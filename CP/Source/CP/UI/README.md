# UI 관련 C++

## 개요

이 폴더의 UI 클래스들은 전부 **델리게이트 방식**으로 데이터 소스와 디커플링되어 있다. 각 위젯/
컴포넌트는 인터페이스 구현을 요구하지 않고, "값이 바뀌면 호출해줄" 진입점 함수 하나만 제공한다.
그 함수를 실제 값을 들고 있는 쪽(GameMode, Actor 등)의 `BlueprintAssignable` 델리게이트에
BP의 Bind Event(또는 C++의 `AddDynamic`)로 연결해두면, 이후로는 값이 바뀔 때마다 Broadcast만
해도 화면이 자동으로 갱신된다. 폴링(Tick)도 없다.

> **한글 리터럴 관련 주의**: 이 프로젝트의 다른 소스 파일 중 일부(`CoinPusher/CPCoinPusher.h`
> 등)는 한글 코드페이지(CP949)로 저장돼 있고, 이 폴더의 새 파일들은 UTF-8로 저장돼 있다.
> 컴파일러의 소스 인코딩 처리 방식에 따라 **문자열 리터럴(코드에 컴파일되는 `"..."`/`TEXT(...)`)
> 안의 한글은 깨질 수 있어**, 아래 클래스들은 실제 표시 문구를 C++ 코드에 하드코딩하지 않고
> Widget Blueprint의 Class Defaults(에디터에서 입력, `.uasset`에 저장되므로 인코딩 문제 없음)
> 에서 설정하도록 만들었다. 코드에는 영문 자리표시자 기본값만 들어있다.

## Class 구조

### 게이지 바 (체력바 등 범용)

- `UCPHorizonGuageBarWidget` : 가로로 채워지는 범용 게이지 바 WBP의 베이스(체력/경험치/콤보 등
  0~1 비율로 표현 가능한 값이면 무엇에든 재사용). `BackgroundImage`(배경 틀) + `FillImage`(실제
  값을 나타내는 앞쪽 이미지, 둘 다 `BindWidgetOptional`)로 구성. `Update(Current, Max)`를
  호출(또는 델리게이트 바인딩)하면 Percent를 계산해 `SetPercent`를 호출하는데, 기본 구현이
  `FillImage->SetRenderScale(FVector2D(Percent, 1.0f))`로 가로 폭을 자동으로 줄여주므로 **WBP에서
  별도 그래프 작업 없이도 바로 동작**한다. `NativeConstruct`에서 `FillImage`의 Render Transform
  Pivot을 코드로 직접 `(0.0, 0.5)`(왼쪽 기준)로 맞춰주기 때문에 **왼쪽 끝은 고정된 채 오른쪽에서
  왼쪽으로 줄어드는 방향**으로 동작한다 - WBP 디자이너에서 Pivot을 따로 설정할 필요 없음.
  색상 변화 등 커스텀 연출이 필요하면 `SetPercent`가 `BlueprintNativeEvent`라 WBP에서
  오버라이드해 추가할 수 있다(Super 호출 여부는 자유). `ValueText`(`UTextBlock`,
  `BindWidgetOptional`)를 배치해두면 `SetValues(Current, Max)`의 기본 구현(이것도
  `BlueprintNativeEvent`)이 `DisplayFormat`(기본 `"{0} / {1}"`)으로 포맷해서 자동으로 채워준다 -
  WBP에서 오버라이드하면 다른 표시 방식(퍼센트 등)으로 바꿀 수 있다. 월드 스페이스/뷰포트 스페이스
  어느 쪽에서도 동일하게 재사용

  > **WBP 배치**: `BackgroundImage`를 뒤에, `FillImage`를 그 앞에 겹쳐 배치(Overlay 또는 Canvas
  > Panel에서 순서로 조절)하면 된다. `FillImage`의 Anchor/Alignment는 왼쪽 정렬(Horizontal
  > Alignment: Left)로 둬야 줄어들 때 오른쪽 끝만 안쪽으로 들어오는 모양이 된다
- `UCPHealthBarComponent` : `UWidgetComponent` 기반. 아무 Actor의 BP에나 Add Component로 붙이면
  그 위에 월드 스페이스 체력바가 뜬다 (적/보스 머리 위 등). `WidgetClass`에는
  `UCPHorizonGuageBarWidget`을 상속하는 WBP를 지정하고, `UpdateHealth(Current, Max)`는 내부에서
  그 위젯의 `Update`를 호출해준다
- `UCPViewportHealthBarComponent` : `UActorComponent` 기반. BeginPlay에 `UCPHorizonGuageBarWidget`
  WBP를 생성해 화면(뷰포트 또는 로컬 플레이어 화면)에 고정 표시한다 (플레이어 자신의 HUD 체력바 등).
  마찬가지로 `UpdateHealth(Current, Max)`가 내부에서 `Update`를 호출해준다

### 캐릭터 정보 / 코인 콤보 / 인게임 통합 HUD

- `UCPCharacterInfoWidget`("CharacterInfoUI") : 캐릭터(플레이어, 보스 등) 정보를 한 번에 보여주는
  UI. `HealthGaugeWidget`/`ExpGaugeWidget`(둘 다 `UCPHorizonGuageBarWidget`, `BindWidgetOptional`),
  `NameText`(`UTextBlock`), `LevelText`(`UTextBlock` - `SetLevel(Level)`이 `LevelDisplayFormat`
  기본 `"Lv.{0}"`으로 포맷해서 채워줌), `PortraitImage`(`UImage`)로 구성. `UpdateHealth`/
  `UpdateExp`/`SetCharacterName`/`SetLevel`/`SetPortrait`로 각각 독립적으로 갱신하며, 대응하는
  컴포넌트가 WBP에 배치되지 않았으면(바인딩되지 않았으면) 해당 함수는 조용히 아무 동작도 하지
  않는다(=표시하지 않음)
- `UCPCoinComboWidget`("CoinComboUI") : 코인 콤보 수(`ComboCountText`, `UCPTicketCountWidget`과
  동일한 `DisplayFormat` 패턴)와 콤보 상태를 나타내는 게이지(`ComboGaugeWidget`,
  `UCPHorizonGuageBarWidget`)를 함께 보여주는 UI. `SetComboCount(Count)`/
  `UpdateComboGauge(Current, Max)`로 각각 갱신하며, 둘 다 `BindWidgetOptional`이라 배치하지 않은
  쪽은 조용히 무시된다
- `UCPInGameWidget`("InGameUI") : 인게임 화면의 여러 WBP 구성요소를 한데 모아놓은 최상위 HUD
  위젯. 일시정지 메뉴("InGamePauseUI", `UCPInGamePauseWidget`)와 CoinPusher Picture-in-Picture
  (`UCPCoinPusherCaptureWidget`)는 각각 `ACPTopDownPlayerController`가 별도 인스턴스로 직접
  생성/관리하므로 이 위젯은 포함하지 않는다. `PlayerInfoWidget`/`BossInfoWidget`(둘 다
  `UCPCharacterInfoWidget`), `BackgroundImage`(`UImage`), `TicketCountWidget`
  (`UCPTicketCountWidget`), `InventoryWidget`(`UCPInventoryWidget` - 스스로 플레이어 인벤토리에
  바인딩되므로 이 위젯 쪽에 별도 진입점 없음), `CoinComboWidget`(`UCPCoinComboWidget`),
  `RouletteWidget`(`UCPRouletteWidget`, `Roulette/CPRouletteWidget.h` - `ACPRoulette`가 직접
  만들지 않고 이 인스턴스를 그대로 재사용함, `Roulette/README.md`의 "InGameUI 연동" 참고),
  `CoinPointUI`(`UCPCoinPointUI`, 아래 "코인 포인트 UI" 참고)를 전부 `BindWidgetOptional`로 갖는다.
  플레이어/보스 정보, 티켓 개수, 코인 콤보처럼 "값 하나를 그대로 전달"하는 하위 위젯은
  `UpdatePlayerHealth`/`UpdatePlayerExp`/`SetPlayerName`/`SetPlayerLevel`/`SetPlayerPortrait`
  (보스는 `Boss` 접두사로 동일)/`UpdateTicketCount`/`SetComboCount`/`UpdateComboGauge` 같은
  패스스루 함수로 감싸 노출하고, `RouletteWidget`/`CoinPointUI`처럼 이미 자체적으로 풍부한
  API(스핀 연출, 위치 계산 등)를 가진 위젯은 `GetRouletteWidget()`/`GetCoinPointUI()`로 인스턴스
  자체를 돌려줘서 호출부가 필요한 함수를 직접 호출하게 한다. 8개 컴포넌트 전부
  `SetPlayerInfoVisible`/`SetBossInfoVisible`/`SetBackgroundVisible`/`SetTicketCountVisible`/
  `SetInventoryVisible`/`SetCoinComboVisible`/`SetRouletteVisible`/`SetCoinPointUIVisible`
  (전부 `bool` 하나만 받음, 컴포넌트가 없으면 조용히 무시)로 개별 On/Off가 가능하며,
  `NativeConstruct`에서 `BossInfoWidget`만 기본적으로 꺼진 상태(Collapsed)로 시작한다 - 아직
  보스 시스템이 없어 당장 보여줄 값이 없기 때문(`SetBossInfoVisible(true)`로 다시 켤 수 있음)

### 코인 포인트 UI

코인이 떨어진 위치에 안내 문구(예: "+1")를 잠깐 띄웠다가 지우는 UI. **코인이 실제로 보이는 화면은
플레이어의 메인 게임플레이 카메라가 아니라 `ACPCoinPusher`의 Picture-in-Picture
(`UCPCoinPusherViewCaptureComponent`, `SceneCaptureComponent2D`)가 화면 왼쪽
`CaptureWidthRatio` 영역에 그리는 별도의 뷰**(`CoinPusher/CPCoinPusherViewCaptureComponent.h`/
`CPCoinPusherCaptureWidget.h`/`CPCoinPusherViewportClient.h` 참고 - 플레이어 자신의 카메라
뷰포트는 오른쪽 `1-CaptureWidthRatio` 영역으로 축소되어 있다)이므로, 이 UI는 플레이어
컨트롤러의 카메라가 아니라 그 캡처 컴포넌트의 위치/회전/FOV(또는 Ortho Width)를 직접 이용해
투영한다. PIP 밖으로 떨어진(또는 캡처 카메라 뒤쪽인) 코인도 문구 자체는 항상 PIP 영역 안에서
보이도록 가장자리에 마진을 두고 클램프한다.

- `UCPCoinPointTextWidget` : 낱개로 떴다 사라지는 텍스트 위젯. `PointText`(`UTextBlock`,
  `BindWidgetOptional`)에 `SetPointText(Text)`로 문구를 표시하는 것 말고는 로직이 없다. 언제
  사라질지는 스스로 정하지 않고 `UCPCoinPointUI`가 타이머로 관리하며, `SetPointText` 직후
  `PlayAppearEffect`(`BlueprintImplementableEvent`)가 호출되므로 페이드 인/위로 떠오르는 연출이
  필요하면 WBP에서 오버라이드해 UMG Animation을 재생하면 된다
- `UCPCoinPointUI`("CoinPointUI") : `PointTextCanvas`(`UCanvasPanel`, `BindWidgetOptional`) 위에
  `ShowPointText(Text, WorldLocation)`가 호출될 때마다 `PointTextWidgetClass`
  (`UCPCoinPointTextWidget` 상속 WBP) 인스턴스를 하나 생성해서 계산된 화면 좌표에 배치하고,
  `DisplayDuration`(기본 1초) 후 제거한다 - 동시에 여러 코인이 떨어져도 각자 독립된 인스턴스로
  겹쳐 표시된다. 화면 좌표 계산은 레벨의 `ACPCoinPusher`를 찾아(`GetCaptureComponent()`, 최초
  1회 캐싱) 그 `GetViewCaptureComponent()`의 위치/회전과 `FOVAngle`(Perspective) 또는
  `OrthoWidth`(Orthographic)를 이용해 `WorldLocation`을 캡처 카메라 기준으로 투영한 뒤,
  `CaptureWidthRatio`(기본 0.3, **`UCPCoinPusherCaptureWidget`/`UCPCoinPusherViewportClient`의
  값과 반드시 일치시켜야 함**)로 정해지는 화면 왼쪽 PIP 영역의 픽셀 좌표로 변환한다. 월드 위치가
  PIP 프러스텀 밖이거나 캡처 카메라 뒤쪽이라 정상적으로 투영되지 않으면, PIP 영역 가장자리에서
  `OffscreenMargin`(기본 32px)만큼 안쪽으로 들여온 위치로 클램프해서 항상 PIP 화면 안에 보이게
  한다. `ShowCoinPointText(WorldLocation)`은 `CoinPointDisplayText`(기본 "+1")를 문구로 써서
  `ShowPointText`를 호출하는 얇은 래퍼로, `ACPDropZone::OnCoinDropped(FVector)` 델리게이트와
  시그니처가 동일해 그대로 Bind Event할 수 있다. 레벨에 `ACPCoinPusher`가 없으면(테스트 레벨 등)
  캡처 컴포넌트를 찾지 못해 아무것도 표시하지 않는다
- `ACPDropZone::OnCoinDropped`(`FVector, WorldLocation`) : 코인이 이 DropZone에 떨어져
  `AddCollectedCoins`가 호출될 때마다(코인 액터가 스스로 넘긴 `GetActorLocation()`) 그 월드
  위치와 함께 Broadcast. `UCPCoinPointUI::ShowCoinPointText`와 시그니처가 같으므로 BP에서
  Bind Event 한 번으로 "코인 먹을 때마다 그 자리에 +1 표시"가 끝난다

### 원형 게이지

- `UCPRadialGaugeWidget` : 진짜 파이 조각처럼 각도로 채워지는(clock-wipe) 원형 게이지 WBP 베이스.
  `BackgroundImage`(배경 원) + `FillImage`(실제 값을 나타내는 원, 둘 다 `BindWidgetOptional`)로
  구성. UMG의 `Image`/`ProgressBar`는 좌우·상하 방향 채우기만 지원하고 각도(radial) 채우기를
  지원하지 않으므로, **`FillImage`의 Brush에 각도 기반 마스크를 구현하는 Material을 지정**하는
  방식으로 동작한다. `NativeConstruct`에서 그 Material로 Dynamic Material Instance를 만들어
  캐싱해두고, `UpdateGauge(CurrentValue, MaxValue)`를 호출(또는 델리게이트 바인딩)할 때마다
  `SetGaugePercent`가 그 인스턴스의 `PercentParameterName`(기본 `"Percent"`) 스칼라 파라미터를
  갱신해준다. 색상 변화 등 커스텀 연출이 필요하면 `SetGaugePercent`가 `BlueprintNativeEvent`라
  WBP에서 오버라이드할 수 있다. 룰렛 회전 진행도, 대시/스킬 쿨다운, 충전 게이지 등 0~1로 표현
  가능한 어떤 값에도 재사용 가능

  > **Material 준비 필수**: `FillImage`에 그냥 텍스처만 지정하면 파라미터를 갱신할 대상이 없어
  > 아무 효과도 나지 않는다. 아래 순서로 각도 기반 마스크 Material을 하나 만들어 `FillImage`의
  > Brush → Image에 지정해야 한다 (Material Domain은 **User Interface**로 설정):
  > 1. `TextureCoordinate` → `Subtract`로 `(0.5, 0.5)`를 빼서 중심 기준 UV로 이동
  > 2. `ComponentMask`로 R(X)/G(Y) 분리 후 `Atan2(Y, X)` → 각도(라디안, -π~π)
  > 3. `Divide by 2π` 후 `Add 0.5`로 0~1 범위 각도 비율로 정규화 (시작 위치를 12시 방향으로
  >    맞추려면 Atan2에 넣기 전 X/Y를 적절히 회전/부호 반전)
  > 4. `Scalar Parameter`(이름을 `Percent`로) 노드를 추가하고, 2~3에서 만든 각도 비율과 `Step`
  >    (또는 `If`) 노드로 비교해 0/1 마스크 생성
  > 5. 그 마스크를 텍스처 샘플의 Alpha/Opacity에 곱해서 Material의 Opacity 출력에 연결
  >
  > 이렇게 만든 Material을 `FillImage`의 Brush(Image)로 지정하면 준비 끝 - 이후로는 코드가
  > `Percent` 파라미터만 갱신해준다
- `UCPRadialGaugeComponent` : `UCPHealthBarComponent`와 동일한 구조의 `UWidgetComponent` 기반
  컴포넌트 - 아무 Actor에나 Add Component로 붙이면 그 위에 월드 스페이스 원형 게이지가 뜬다.
  `SetGaugeEnabled(bool)`로 게이지 자체를 켜고 끌 수 있음(Off일 때 렌더링/컴포넌트 Tick 모두 중단) -
  항상 보일 필요 없는 게이지(특정 상황에서만 나타나는 충전 게이지 등)에 사용

### 개수/시간 표시

- `UCPCoinCountWidget` : `UpdateCoinCount(int32 Count)` → `DisplayFormat`(EditAnywhere FText,
  WBP Class Defaults에서 실제 문구 지정)으로 포맷해 `CoinCountText`(BindWidgetOptional)에 표시.
  `ACPGameMode::OnTeamCoinCountChanged`에 바인딩해서 사용
- `UCPTicketCountWidget` : 위와 동일한 구조로 티켓 개수 표시. `ACPGameMode::OnTeamTicketCountChanged`
  에 바인딩
- `UCPTimeDisplayWidget` : `UpdateTime(float TimeInSeconds)` → "MM:SS" 형식으로 `TimeText`
  (BindWidgetOptional)에 표시. 제한시간 타이머, 스톱워치 등에 사용

### UI 전환

- `FCPAnyInputProcessor`(`CPAnyInputProcessor.h`, 헤더 온리) : 포커스/히트테스트와 완전히 무관하게
  키보드/마우스/게임패드 입력을 가로채는 전역 `IInputProcessor`. 생성자에 키/마우스 콜백을
  넘기면 그대로 호출해주는 얇은 래퍼
- `UCPPressAnyKeyWidget` : `FCPAnyInputProcessor`를 `NativeConstruct`에서 등록(`NativeDestruct`
  에서 해제)해서 아무 키/마우스/게임패드 입력이든 감지해 `OnAnyKeyPressed`(BlueprintAssignable)
  를 Broadcast. `NextWidgetClass`를 지정해두면 BP 작업 없이도 자동으로 그 위젯으로 전환됨
  (타이틀 화면 "Press Any Button" 등). `SwitchDelay`(EditAnywhere, 기본 0초)를 0보다 크게 주면
  첫 입력 후 그 시간만큼 기다렸다가 전환(이미 대기 중이면 추가 입력이 들어와도 다시 잡지 않음) -
  0이면 기존처럼 즉시 전환. `HandleAnyKeyPressed`는 Broadcast 후 `ScheduleSwitchToNextWidget()`
  (protected, `virtual`)을 호출해 위 딜레이/전환 예약을 수행 - 하위 클래스가 이 함수를 오버라이드해
  전환 전에 자기만의 연출(점멸 등)을 넣고 원하는 타이밍에 직접 `SwitchToNextWidget()`(protected)을
  호출하도록 재정의할 수 있다 (`UCPStartScreenWidget` 참고)

  > **왜 `NativeOnKeyDown`/`SetUserFocus` 대신 `IInputProcessor`인가**: 처음엔
  > `NativeOnKeyDown`/`NativeOnMouseButtonDown` + `SetUserFocus`로 구현했었는데, 실제로
  > 테스트해보니 두 가지 이유로 신뢰할 수 없었다 - (1) 마우스 클릭이 위젯의 히트테스트 가능한
  > 영역을 못 맞히면 클릭 자체가 포커스를 날려버림(`NativeOnFocusLost` Cause=Mouse), (2)
  > 게임패드 입력은 `SetUserFocus`가 쓰는 레거시 `ControllerId` 기반 Slate User와 실제 게임패드
  > 키 이벤트가 라우팅되는 Slate User가 서로 어긋나 포커스가 있어도 이벤트가 전달되지 않음.
  > `IInputProcessor`는 Slate가 포커스/히트테스트로 이벤트를 어디로 보낼지 정하기 이전 단계에서
  > 가로채므로 이 두 문제 모두와 무관하게 항상 동작한다

### 시작 화면 / 게임 설명 화면 (컨트롤러 선택)

두 화면 모두 `UCPPressAnyKeyWidget`을 상속해 `FCPAnyInputProcessor` 기반 "아무 입력이나 감지" 방식을
그대로 재사용하고, `ECPControllerType`(`GameMode/CPControllerType.h`, `KeyboardMouse`/`GamePad`)과
`UCPControllerTypeSubsystem`(`GameMode/`, `UGameInstanceSubsystem`)으로 "어떤 컨트롤러를 쓰는지"를
화면 전환·레벨 전환 이후에도 들고 다닌다 (`UCPPlayerRegistrySubsystem`과 같은 이유로 GameInstance
서브시스템을 사용 - "몇 번째 플레이어인지"가 아니라 "장치 종류"라는 별개의 관심사라 서브시스템도 분리).

- `UCPStartScreenWidget` : `BackGroundImage`(WBP에 배치만, 별도 바인딩 불필요)와
  `GamePadImage`/`KeyBoardImage`(둘 다 `BindWidgetOptional`), "Press Any Key" 안내 텍스트로
  구성되는 시작 화면. `OnAnyKeyPressed`에 바인딩된 `HandleControllerInputDetected(PressedKey)`가
  (점멸 시작 전까지) `PressedKey.IsGamepadKey()`로 게임패드/키보드·마우스를 구분해 반대쪽 아이콘은
  숨기고(`Collapsed`) 방금 사용한 장치 쪽 아이콘은 다시 보이게(`SelfHitTestInvisible`) 한 뒤, 그
  결과를 `UCPControllerTypeSubsystem::SetSelectedControllerType()`에 매 입력마다 기록한다 - 게임패드를
  눌렀다 키보드를 누르면 반대로 토글되며, 최종적으로 마지막에 사용한 장치의 아이콘만 남는다.
  부모의 `ScheduleSwitchToNextWidget()`을 오버라이드해서, 화면에 남아있는(=선택된) 아이콘을
  `BlinkCount`(EditAnywhere, 기본 3)번 `BlinkInterval`(EditAnywhere, 기본 0.5초) 간격으로 점멸시킨
  뒤에야(꺼짐→켜짐을 `BlinkCount`번, 즉 토글 `BlinkCount*2`번 - 항상 켜진 채로 끝남) `SwitchToNextWidget()`
  을 호출해 다음 화면으로 전환한다 - 부모의 `SwitchDelay`는 이 위젯에서는 쓰이지 않음(오버라이드가
  자체 타이밍으로 직접 전환을 호출). 점멸 시퀀스가 한 번 시작되면(`bHasStartedBlink`) 이후 입력은
  `HandleControllerInputDetected`에서 무시되어 선택이 다시 바뀌지 않는다. WBP Class Defaults에서는
  `NextWidgetClass`에 `UCPGameExplanationWidget` 상속 WBP만 지정하면 됨
- `UCPGameExplanationWidget` : 시작 화면에서 선택된 컨트롤러에 따라 `GamePadImage` 또는
  `KeyBoardImage`(둘 다 `BindWidgetOptional`) 중 하나만 보이도록 `NativeConstruct`에서
  `UCPControllerTypeSubsystem::GetSelectedControllerType()`을 조회해 반영하고, "Press Any Key to
  Start" 안내 텍스트는 WBP에 고정 배치. `OnAnyKeyPressed`에 바인딩된
  `HandleSelectedControllerKeyPressed(PressedKey)`가 `PressedKey.IsGamepadKey()`를 선택된 컨트롤러
  종류와 비교해서, **일치하지 않는 입력은 무시**하고(예: 게임패드를 선택했는데 키보드를 누르면
  아무 반응 없음) 일치할 때만 `NextLevelName`(EditAnywhere)으로 `UGameplayStatics::OpenLevel()`을
  호출 - `UCPControllerTypeSubsystem`이 GameInstance에 붙어 있어 레벨이 바뀌어도 선택 정보가 남으므로
  별도 파라미터 전달 없이 다음 레벨에서 그대로 조회 가능. 부모의 `NextWidgetClass`는 설정하지 않음
  (다음 화면이 아니라 다음 레벨로 이동하므로)

### 게임 종료 화면

- `UCPGameOverWidget` : `RestartLevel()` — 현재 레벨을 다시 로드 (재시작 버튼 등에서 호출)
- `UCPGameClearWidget` : `NextLevelName`(EditAnywhere) + `GoToNextLevel()` — 지정한 레벨로 이동
  (다음 스테이지/타이틀로 버튼 등에서 호출)

  > 위 둘은 만들어만 두고 아직 어디서도 생성/표시하지 않는 초기 스캐폴딩이다(각각 재시작/다음
  > 레벨 이동 함수 하나씩만 가짐). 실제 일시정지 메뉴/Clear·Lose 엔딩 화면은 아래 "인게임
  > 일시정지 / 엔딩 화면" 절의 `UCPInGamePauseWidget`/`UCPEndingWidget`을 사용한다 - 서로 다른
  > 스펙(배경/버튼 2개/컨트롤러별 분기 여부)이라 겹치지 않고 별도로 남겨뒀다

### 인게임 일시정지 / 엔딩 화면

게임패드는 Menu 버튼, 키보드/마우스는 ESC를 누르면 게임이 일시정지되고 메뉴 UI가 뜨며, 같은 버튼을
다시 누르면 닫히고 게임이 재개된다. 이 메뉴(InGamePause)와, 특정 조건에 Clear/Lose 결과를 보여주는
엔딩 화면(Ending)은 반투명 배경 + "게임 종료"/"타이틀로 돌아가기" 두 버튼이라는 같은 뼈대를 공유하므로
`UCPInGamePauseWidget`(뼈대) → `UCPEndingWidget`(그 위에 Clear/Lose 이미지만 추가)로 상속 관계를 이룬다.
실제 입력 처리(일시정지 토글, 게임패드 탐색/확인)는 위젯이 아니라 `ACPTopDownPlayerController`
(`CP/Player/`)가 맡는다 - 위젯은 순수하게 "선택/실행" 상태만 갖고, 어떤 입력 장치가 그 상태를
바꾸는지는 몰라도 된다.

- `UCPInGamePauseWidget`(`UI/CPInGamePauseWidget.h`) : `KeyboardMouseBackgroundImage`/
  `GamePadBackgroundImage`(둘 다 `BindWidgetOptional`) 중 현재 `UCPControllerTypeSubsystem`에 기록된
  컨트롤러 종류에 맞는 쪽만 보이도록 `RefreshBackgroundForControllerType()`이 전환한다(시작
  화면과 동일한 `SelfHitTestInvisible`/`Collapsed` 토글 방식). `EndGameButton`/`ReturnToTitleButton`
  (둘 다 `BindWidgetOptional` `UButton`)과 그에 대응하는 `EndGameButtonOutline`/
  `ReturnToTitleButtonOutline`(둘 다 `BindWidgetOptional` `UWidget` - Border/Image 등 자유롭게 사용)로
  구성. 두 버튼을 `NavigableButtons`/`ButtonOutlines` 배열로 모아두고 `SelectedButtonIndex` 하나로
  "지금 윤곽선이 표시된 버튼"을 관리한다 - 키보드/마우스는 포인터가 버튼 위로 올라오면(`OnHovered`)
  그 버튼이 선택되고 클릭(`OnClicked`)하면 즉시 실행되며, 게임패드는 `ACPTopDownPlayerController`가
  L-Stick 입력을 `MoveSelection(Delta)`(선택 인덱스를 순환 이동)로, A버튼 입력을
  `ConfirmSelection()`(현재 선택된 버튼 실행)으로 전달해 조작한다 - 결국 마우스 호버든 게임패드
  탐색이든 같은 `SelectedButtonIndex`/`UpdateSelectionVisuals()` 경로로 합쳐지므로 윤곽선 표시 로직은
  하나뿐이다. `EndGameButton`은 `EndGame()`(`UKismetSystemLibrary::QuitGame`)을,
  `ReturnToTitleButton`은 `ReturnToTitle()`(`TitleLevelName`(EditAnywhere)로 `OpenLevel`)을 실행 -
  둘 다 `virtual`이라 필요하면 하위 클래스나 WBP에서 오버라이드 가능. `RefreshForDisplay()`
  (`BlueprintCallable`)는 이 위젯이 (재사용되는 인스턴스로) 다시 표시될 때마다 배경 이미지와 선택을
  초기화하기 위한 진입점 - `ACPTopDownPlayerController`가 `SetVisibility(Visible)` 직전에 호출한다
- `UCPEndingWidget`(`UI/CPEndingWidget.h`) : `UCPInGamePauseWidget`을 상속해 배경/버튼/선택 로직을
  그대로 재사용하고, `ClearImage`/`LoseImage`(둘 다 `BindWidgetOptional` `UImage`)만 추가한다.
  `ShowResult(bool bIsClear)`(`BlueprintCallable`)가 둘 중 하나만 보이도록 전환 - `bIsClear`가
  true면 `ClearImage`, false면 `LoseImage`
- `ACPTopDownPlayerController`(`CP/Player/`)에 추가된 Pause/Ending 관련 멤버:
  - `PauseAction`(`UInputAction*`) : 일시정지 메뉴를 열고 닫는 입력. Input Mapping Context에서
    게임패드 Menu 버튼과 키보드 Escape를 **같은 액션**에 매핑해두면 둘 다 토글로 동작한다
  - `MenuNavigateAction`(`UInputAction*`, Axis2D) : 메뉴가 열려 있는 동안 게임패드 L-Stick의 **X축**
    (좌우)으로 버튼 사이를 이동 — `EndGameButton`/`ReturnToTitleButton`이 가로로 배치되므로 X축을
    읽는다(왼쪽=이전 버튼, 오른쪽=다음 버튼). `MenuNavigateDeadZone`(EditAnywhere, 기본 0.5) 미만인
    축 값은 무시되며, 한 번 민 입력은 스틱이 중립으로 돌아올 때까지 한 번만 처리된다(디바운스,
    `bHasProcessedMenuNavigateThisHold`) - 이 디바운스 상태는 입력을 실제로 받는 컨트롤러
    인스턴스(=자기 자신) 기준으로 관리된다
  - `MenuConfirmAction`(`UInputAction*`) : 메뉴가 열려 있는 동안 게임패드 A버튼으로 현재 선택된
    버튼의 기능을 실행
  - `InGamePauseWidgetClass`/`EndingWidgetClass`(`TSubclassOf`, `EditDefaultsOnly`) : 각각
    `UCPInGamePauseWidget`/`UCPEndingWidget` 상속 WBP를 지정
  - `TogglePauseMenu()`(`BlueprintCallable`, 파라미터 없는 public 버전) : `UGameplayStatics::SetGamePaused()`
    로 월드 전체를 일시정지/재개시키고, `GetMenuOwnerController()`의 `InGamePause` 위젯을 표시/숨김.
    Ending 위젯이 떠 있는 동안은 무시(Ending에는 재개 개념이 없음). `PauseAction`(Enhanced Input)에
    바인딩된 `HandleTogglePauseAction(const FInputActionValue&)`는 이 함수를 그대로 호출하는 얇은
    래퍼일 뿐이므로(이름을 다르게 둔 이유는 `BindAction`이 같은 이름의 오버로드 2개 중 어느
    쪽 주소인지 템플릿에서 구분하지 못해 컴파일 에러가 나기 때문), `PauseAction`/Input Mapping
    Context가 아직 없는 곳(테스트 스캐폴딩 등)에서도
    이 파라미터 없는 버전을 직접 호출해 일시정지 메뉴를 켤 수 있다 - 단, `InGamePauseWidgetClass`
    (WBP 참조)는 C++에 하드코딩할 수 없으므로 여전히 BP에서 지정해야 한다 (아래 "테스트" 절 참고)
  - `ShowEndingResult(bool bIsClear)`(`BlueprintCallable`) : 특정 조건(레벨 클리어, 체력 0 등)이
    만족됐을 때 호출 - 게임을 일시정지하고 `GetMenuOwnerController()`의 Ending 위젯에 Clear/Lose
    결과를 표시. 로컬 스플릿 스크린의 어느 플레이어 컨트롤러에서 호출해도 항상 하나의 공용 위젯으로
    합쳐진다(아래 참고)
  - `GetMenuOwnerController()` : `PauseWidgetInstance`/`EndingWidgetInstance`는 화면 전체(양쪽
    스플릿 스크린 절반 모두)를 덮는 `AddToViewport()` 오버레이라, `ACPRoulette`처럼 플레이어마다
    따로 만들 필요가 없다 - 항상 월드의 첫 번째 로컬 `PlayerController`(`GetFirstPlayerController()`)
    하나에만 생성/캐싱되며, 어느 플레이어의 입력이 `TogglePauseMenu`/`ShowEndingResult`/
    `HandleMenuNavigate`/`HandleMenuConfirm`을 호출했든 전부 이 함수로 그 하나의 인스턴스를 찾아
    대신 조작한다

> **일시정지 중에도 입력이 들어오게 하려면 (엔진 차원의 필수 설정)**: `UGameplayStatics::SetGamePaused(true)`
> 로 일시정지하면 `APlayerController::TickActor()`가 기본적으로 `PlayerTick()`(Enhanced Input 액션 평가
> 경로 포함)을 건너뛴다. `ACPTopDownPlayerController`는 생성자에서 `bShouldPerformFullTickWhenPaused = true`
> 를 설정해 이 문제를 해결해뒀지만, 그것만으로는 부족하다 - Enhanced Input은 액션 단위로 한 번 더
> "일시정지 중에도 트리거할지"를 검사하므로, **`PauseAction`/`MenuNavigateAction`/`MenuConfirmAction`으로
> 쓰는 각 `UInputAction` 에셋마다 `Action Trigger` 카테고리의 `Trigger When Paused`를 체크**해야
> 실제로 일시정지 중 재개/메뉴 탐색/확인이 동작한다. 둘 중 하나라도 빠지면 "일시정지는 되는데 다시
> 못 풀거나(ESC/Menu 재입력 무시) A버튼/L-Stick이 먹통"인 증상으로 나타난다

> **왜 위젯 인스턴스가 플레이어당 하나가 아니라 전체에 하나뿐인가**: `ACPRoulette`의 룰렛 스핀
> UI(`Roulette/README.md` 참고)는 플레이어마다 별개 위젯을 만들어 각자 화면에 `AddToViewport()`하는데,
> 이는 "두 화면에 똑같은 내용을 동시에 띄운다"는 결과만 같을 뿐 두 인스턴스가 각자 독립적으로 존재한다.
> 일시정지/엔딩 메뉴는 상태(선택된 버튼, 열려있는지 여부)까지 두 플레이어가 완전히 공유해야
> 하므로(한 플레이어가 연 메뉴를 다른 플레이어가 게임패드로 조작해 닫을 수도 있어야 함), 인스턴스
> 자체를 하나만 두고 항상 그 하나를 찾아가는 방식(`GetMenuOwnerController`)을 택했다

### 로컬 2인 플레이 참가

- `ACPLobbyGameMode`(`CP/GameMode/`) : `UCPPlayerJoinWidget`이 뜬 뒤 처음 입력을 발생시킨 장치
  (키보드/마우스든 게임패드든)를 1P(PlayerIndex 0)로, 그 다음 처음 보는 장치를 2P(PlayerIndex 1)로
  "누른 순서" 그대로 배정하는 GameMode - 이미 존재하는 Player 0 컨트롤러를 재사용하는 게 아니라,
  실제로 가장 먼저 입력한 장치가 PlayerIndex 0을 차지한다. `RegisterPlayerInput(DeviceId)`가
  호출될 때마다 처리하며, 이 단계에서는 새 로컬 플레이어를 만들거나 장치를 리매핑하지 않고
  "이 장치가 몇 번째 플레이어인지" 정보만 `UCPPlayerRegistrySubsystem`에 등록해 다음 레벨로
  넘긴다(실제 로컬 플레이어 생성/장치 리매핑은 그 정보를 바탕으로 다음 레벨에서 처리). 지정한
  인원수(`NumberOfPlayersToJoin`)가 모두 배정되면 `OnAllPlayersJoined`를 Broadcast하고
  `LevelLoadDelay`초 후 `NextLevelName`을 연다. 생성자에서 `PlayerControllerClass`를
  `ACPLobbyPlayerController`로 지정하고, `BeginPlay`에서 `StartWidgetClass`(보통
  `UCPPressAnyKeyWidget` 상속 WBP)를 자동으로 `CreateWidget` + `AddToViewport`해준다 - 레벨
  블루프린트 등에서 위젯을 따로 만들어 띄울 필요가 없다
- `ACPLobbyPlayerController`(`CP/GameMode/`) : 로비 화면 전용 최소 구성 PlayerController.
  `ACPTopDownPlayerController`처럼 Input Mapping Context를 추가하지 않는다 - 이 화면의 입력은
  `UCPPressAnyKeyWidget`/`UCPPlayerJoinWidget`이 각자 `FCPAnyInputProcessor`로 직접 가로채므로
  PlayerController가 입력 모드/포커스를 따로 관리해줄 필요가 없다
- `UCPPlayerJoinWidget` : "아무 버튼이나 눌러 참가하세요" 화면에 놓는 위젯. `FCPAnyInputProcessor`
  로 감지한 입력 장치를 `ACPLobbyGameMode::RegisterPlayerInput`으로 전달하고, `OnPlayerJoined`를
  구독해 배정 결과를 `OnPlayerSlotAssigned(PlayerIndex)`(`BlueprintNativeEvent`)로 알려준다.
  기본 구현이 `Player1Square`/`Player2Square`(둘 다 `BindWidgetOptional` `Image`)를 각각
  `Player1Color`(기본 빨강)/`Player2Color`(기본 파랑)로 칠해준다 - 먼저 입력한 사람은 빨간
  네모, 나중에 입력한 사람은 파란 네모로 표시됨. WBP에서 오버라이드해서 다른 연출(애니메이션,
  텍스트 등)을 추가할 수도 있음
- `UCPPlayerRegistrySubsystem`(`CP/GameMode/`, `UGameInstanceSubsystem`) : GameInstance에 붙어
  있어 `OpenLevel`로 레벨이 바뀌어도 살아남는다. 로비에서 배정된 "PlayerIndex(0=1P, 1=2P, ...) ↔
  입력 장치(FInputDeviceId)" 순서만 들고 있다가, 다음(실제 게임플레이) 레벨에서
  `GetPlayerIndexForInputDevice(DeviceId)` / `GetInputDeviceForPlayerIndex(PlayerIndex)`로
  "이 입력 장치가 몇 번째 플레이어인지"를 질의할 수 있게 해준다. 로컬 플레이어 생성이나 장치
  리매핑은 이 정보를 바탕으로 다음 레벨에서 직접 처리해야 한다. Project Settings에 등록할 필요
  없이 자동으로 생성됨

#### 게임패드/마우스 대응 관련 히스토리 (왜 IInputProcessor 방식인가)

`UCPPressAnyKeyWidget`/`UCPPlayerJoinWidget`은 처음엔 `NativeOnKeyDown`/`NativeOnMouseButtonDown`
+ `SetInputMode(FInputModeUIOnly)` + `SetUserFocus`로 구현했었다. 실제 테스트(로그로 확인)에서
두 가지 문제가 드러나 지금의 `FCPAnyInputProcessor` 방식으로 바꿨다:

1. **마우스**: 클릭이 위젯의 히트테스트 가능한 영역을 못 맞히면(위젯이 화면 전체를 덮지 않거나
   Hit Test Invisible인 배경 등), 그 클릭 자체가 위젯의 포커스를 날려버린다
   (`NativeOnFocusLost` `Cause=Mouse`) - 포커스가 없으니 당연히 `NativeOnKeyDown`도 안 불림
2. **게임패드**: `SetUserFocus`는 내부적으로 `ULocalPlayer::GetControllerId()`(레거시 컨트롤러
   ID)로 Slate User를 결정하는데, 실제 게임패드 키 이벤트는 최신 `FPlatformUserId`/
   `FInputDeviceId` 체계로 라우팅된다. 이 둘이 어긋나면 포커스는 있는 것처럼 보여도
   (`HasKeyboardFocus`가 true) 게임패드 이벤트는 그 포커스와 무관한 경로로 흘러가버려
   `NativeOnKeyDown`이 끝내 호출되지 않는다

`IInputProcessor`는 Slate가 포커스/히트테스트를 기준으로 이벤트를 어디로 보낼지 정하기
"이전" 단계에서 이벤트를 가로채므로 위 두 문제 모두와 무관하게 항상 동작한다. 그래서 두
위젯 모두 포커스/입력 모드를 전혀 건드리지 않고, `FCPAnyInputProcessor`만으로 입력을 감지한다.

## 테스트용 GameMode/PlayerController (`UI/Test`)

시작 화면 → 게임 설명 화면 흐름만 따로 떼어서(레벨 블루프린트나 실제 게임플레이 GameMode 없이)
확인할 수 있는 최소 구성 세트. `CoinPusher/Test`의 GameMode들과 같은 패턴 - 실제 프로젝트 GameMode
(`ACPGameMode` 등)를 상속하지 않고 가벼운 `AGameModeBase`로 둬서 다른 시스템(팀 리소스, 로컬
멀티플레이어 생성 등)의 초기화와 무관하게 UI 흐름만 독립적으로 켜볼 수 있다.

- `ACPStartScreenTestGameMode` : `ACPLobbyGameMode`와 동일한 패턴 - `BeginPlay`에서
  `StartWidgetClass`(보통 `UCPStartScreenWidget` 상속 WBP)를 자동으로 `CreateWidget` + `AddToViewport`
  해준다. `DefaultPawnClass`는 비워두고(순수 UI 테스트라 Pawn 불필요), `PlayerControllerClass`는
  생성자에서 `ACPStartScreenTestPlayerController`로 자동 지정
- `ACPStartScreenTestPlayerController` : `ACPLobbyPlayerController`와 동일한 이유로 Input Mapping
  Context를 추가하지 않는 최소 구성 PlayerController(`UCPStartScreenWidget`/`UCPGameExplanationWidget`
  이 각자 `FCPAnyInputProcessor`로 입력을 직접 가로채므로 PlayerController가 입력 모드/포커스를
  따로 관리할 필요가 없음). 디버그용으로 `EKeys::AnyKey`를 걸어, 입력이 감지될 때마다
  `UCPControllerTypeSubsystem::GetSelectedControllerType()`을 로그로 찍어줘서 이미지 토글이
  눈으로 확인하기 어려운 환경(원격 데스크톱 등)에서도 Output Log만으로 동작을 확인할 수 있다

## 에디터에서 준비해야 할 것

모든 C++ 클래스는 `UCLASS(abstract)`라 실제 사용하려면 Blueprint가 필요하다:

1. 각 Widget 클래스(`UCPHorizonGuageBarWidget`, `UCPRadialGaugeWidget`, `UCPCoinCountWidget`,
   `UCPTicketCountWidget`, `UCPTimeDisplayWidget`, `UCPPressAnyKeyWidget`, `UCPPlayerJoinWidget`,
   `UCPStartScreenWidget`, `UCPGameExplanationWidget`, `UCPInGamePauseWidget`, `UCPEndingWidget`,
   `UCPCharacterInfoWidget`, `UCPCoinComboWidget`, `UCPInGameWidget`, `UCPCoinPointTextWidget`,
   `UCPCoinPointUI`)를 부모로 하는 WBP를 만들고 비주얼
   (ProgressBar/Image/TextBlock 등, `BindWidgetOptional` 변수와 이름을 맞춰서 배치)과 표시 문구
   (`DisplayFormat`, "Press Any Key" 안내 텍스트 등)를 채운다
2. 값을 표시하고 싶은 곳(적/캐릭터 BP, HUD, GameMode 등)에 해당 컴포넌트를 Add Component로
   붙이거나 위젯을 `CreateWidget` + `AddToViewport`
3. 실제 값을 들고 있는 쪽의 `BlueprintAssignable` 델리게이트(`ACPGameMode::OnTeamCoinCountChanged`
   / `OnTeamTicketCountChanged`, 직접 만든 체력 변경 델리게이트 등)를 BeginPlay에서 위 위젯/
   컴포넌트의 `Update*` 함수에 Bind Event로 연결
4. 로컬 2인 참가 화면(StartLevel 등)은 `ACPLobbyGameMode`를 상속하는 BP GameMode를 만들어
   `StartWidgetClass`에 `UCPPressAnyKeyWidget` 상속 WBP를, `NextLevelName`에 실제 게임플레이
   레벨을 지정한 뒤, 로비 레벨의 World Settings → GameMode Override에 그 BP를 지정한다.
   `UCPPressAnyKeyWidget` 상속 WBP의 `NextWidgetClass`에는 `UCPPlayerJoinWidget` 상속 WBP를
   지정 - 그러면 레벨 블루프린트 등에서 위젯을 직접 만들 필요 없이, 레벨을 열면 자동으로
   Press Any Key 화면이 뜨고, 아무 키나 누르면 참가 화면으로 전환되고, 첫 입력이 1P, 다음
   입력이 2P로 배정되면서 자동으로 다음 레벨이 열린다. `PlayerControllerClass`는
   `ACPLobbyGameMode` 생성자가 `ACPLobbyPlayerController`로 자동 지정해주므로 별도 설정 불필요
5. 게임플레이 레벨에서 "이 입력 장치가 1P/2P 중 무엇인지" 알고 싶으면
   `GetGameInstance()->GetSubsystem<UCPPlayerRegistrySubsystem>()`으로 가져와
   `GetPlayerIndexForInputDevice(DeviceId)`를 호출 (`GetInputDeviceForPlayerIndex(0/1)`로
   반대 방향 조회도 가능). 이 레벨에서 실제로 몇 명분의 로컬 플레이어/컨트롤러를 만들지, 각
   장치를 어느 로컬 플레이어에 리매핑할지는 이 정보를 바탕으로 직접 구현해야 한다
6. 게임 오버/클리어는 `UCPGameOverWidget`/`UCPGameClearWidget`을 부모로 WBP를 만들고, 게임
   종료 조건이 발생하는 지점(GameMode, 체력 0 등)에서 `CreateWidget` + `AddToViewport`로 띄운다.
   `UCPGameClearWidget`의 `NextLevelName`은 WBP Class Defaults에서 지정
7. 시작 화면 → 게임 설명 화면 흐름: `UCPStartScreenWidget` 상속 WBP를 만들어 `BackGroundImage`/
   `GamePadImage`/`KeyBoardImage`/안내 텍스트를 배치하고, `NextWidgetClass`에 `UCPGameExplanationWidget`
   상속 WBP를 지정한다(전환 타이밍은 `SwitchDelay`가 아니라 `BlinkCount`/`BlinkInterval`로 제어됨 -
   기본값 그대로 두면 선택된 아이콘이 0.5초 간격으로 3번 점멸한 뒤 전환). `UCPGameExplanationWidget` 상속 WBP에도
   `GamePadImage`/`KeyBoardImage`/"Press Any Key to Start" 텍스트를 배치하고 `NextLevelName`에 이동할
   레벨을 지정한다. 이 WBP를 (`ACPLobbyGameMode`처럼) 레벨 시작 시 자동으로 띄워주는 GameMode의
   `StartWidgetClass` 등에 지정하면, 레벨을 열자마자 시작 화면 → 게임 설명 화면 → 다음 레벨까지
   레벨 블루프린트 작업 없이 자동으로 이어진다
8. 위 흐름만 독립적으로 테스트하고 싶으면 `ACPStartScreenTestGameMode`를 상속하는 BP GameMode를
   만들어 `StartWidgetClass`에 7번에서 만든 `UCPStartScreenWidget` 상속 WBP를 지정한 뒤, 테스트용
   레벨의 World Settings → GameMode Override에 그 BP를 지정한다. `PlayerControllerClass`는 생성자가
   `ACPStartScreenTestPlayerController`로 자동 지정해주므로 별도 설정 불필요 - 레벨을 PIE로 열면
   바로 시작 화면부터 테스트할 수 있고, `EKeys::AnyKey` 입력마다 현재 선택된 컨트롤러 종류가 Output
   Log에 찍힌다
9. 인게임 일시정지/엔딩 화면: `UCPInGamePauseWidget` 상속 WBP를 만들어
   `KeyboardMouseBackgroundImage`/`GamePadBackgroundImage`(반투명 배경, 장치별로 다른 이미지)와
   `EndGameButton`/`ReturnToTitleButton` + 각각의 `EndGameButtonOutline`/`ReturnToTitleButtonOutline`
   (선택 시 보일 윤곽선 - Border든 테두리만 그려진 Image든 자유)을 배치하고, `TitleLevelName`에
   타이틀 레벨을 지정한다. `UCPEndingWidget` 상속 WBP도 동일하게 만들되 `ClearImage`/`LoseImage`를
   추가로 배치한다. 그다음 `ACPTopDownPlayerController`를 상속하는 실제 게임의 BP PlayerController
   Class Defaults에서: `PauseAction`에 게임패드 Menu 버튼과 키보드 Escape를 **같은 Input Action**에
   매핑한 Input Mapping Context 항목을 만들어 지정, `MenuNavigateAction`에 게임패드 L-Stick을 매핑한
   Axis2D Input Action을 지정, `MenuConfirmAction`에 게임패드 A버튼을 매핑한 Input Action을 지정
   (셋 다 `DefaultMappingContexts`에 실제로 추가돼 있어야 함), `InGamePauseWidgetClass`/
   `EndingWidgetClass`에 위에서 만든 두 WBP를 지정한다. **`PauseAction`/`MenuNavigateAction`/
   `MenuConfirmAction`으로 쓰는 세 Input Action 에셋 각각에서 `Action Trigger` 카테고리의
   `Trigger When Paused`를 체크** — 이걸 빼먹으면 일시정지 메뉴가 열리기만 하고 재개/탐색/확인이
   전혀 동작하지 않는다(자세한 이유는 위 "일시정지 중에도 입력이 들어오게 하려면" 참고). 키보드/
   마우스의 클릭·호버는 UMG 버튼이 기본으로 처리하므로 별도 입력 설정이 필요 없다. 게임을 끝내는
   조건이 생기는 지점(GameMode, 체력 0, 레벨 클리어 등)에서
   `ACPTopDownPlayerController::ShowEndingResult(bIsClear)`를 호출하면 Ending 화면이 뜬다. 같은
   BP Class Defaults의 `InGameWidgetClass`에 `UCPInGameWidget` 상속 WBP(위 "캐릭터 정보 / 코인
   콤보 / 인게임 통합 HUD" 절 참고)를 지정하면 InGameUI도 `BeginPlay`에서 함께 생성된다(비워두면
   InGameUI 없이 Pause/Ending만 동작) - `ACPTopDownPlayerController::GetInGameWidget()`으로 어디서든
   조회 가능하며, `ACPRoulette`도 이 인스턴스의 `RouletteWidget`을 그대로 사용한다(아래
   `Roulette/README.md`의 "InGameUI 연동" 참고)
10. 위 InGamePause/Ending UI와 "InGameUI" 통합 HUD를 독립적으로 테스트하려면 `CoinPusher/Test`의
    `ACPCoinPusherItemSpawnTestPlayerController`(`ACPTopDownPlayerController` 상속 - 자체 로직은
    없고 BP에서 값을 채워 넣는 지점일 뿐)를 상속하는 BP를 만들어 9번의 Input Action/Mapping
    Context/위젯 클래스(`InGamePauseWidgetClass`/`EndingWidgetClass`/`InGameWidgetClass` 전부)를
    지정한다. `InGameWidgetClass`로 지정하는 WBP에는 하위 `PlayerInfoWidget`/`BossInfoWidget`/
    `TicketCountWidget`/`CoinComboWidget`/`RouletteWidget`/`CoinPointUI` 등을 배치해둔다.
    `ACPCoinPusherItemSpawnTestGameMode`를 상속하는 BP를 만들어 `PlayerControllerClass`를 그 BP로
    덮어쓴 뒤 테스트 레벨의 GameMode Override로 지정한다. PIE로 열면 Menu 버튼/Escape(Enhanced
    Input, `PauseAction`이 설정돼 있으면)로, 또는 **`ACPCoinPusherItemSpawnTestPawn`의 Escape
    키(레거시 키 바인딩, `ACPTopDownPlayerController::TogglePauseMenu()`를 직접 호출 - `PauseAction`/
    Input Mapping Context가 아직 없어도 동작함)**로 일시정지 메뉴를 열고 닫을 수 있고(단
    `InGamePauseWidgetClass`는 어느 경로든 BP에서 지정해야 함), Z/X 키로 각각 Clear/Lose 엔딩
    화면을 띄워볼 수 있으며, H/J/K/L 키로 플레이어/보스의 테스트용 체력·경험치 게이지가, G 키로
    티켓 개수가, V/C 키로 코인 콤보 수와 게이지가 오르내리는 것을 InGameUI에서 확인할 수 있다.
    **F2-F9는 InGameUI 하위 컴포넌트 8개를 각각 켜고 끈다**(F2 PlayerInfoWidget, F3 BossInfoWidget
    - 기본이 꺼짐, F4 BackgroundImage, F5 TicketCountWidget, F6 InventoryWidget, F7 CoinComboWidget,
    F8 RouletteWidget, F9 CoinPointUI). 전부 `ACPCoinPusherItemSpawnTestPawn`이 들고 있는 가짜
    값/상태를 갱신하는 것으로, 실제 게임플레이 스탯 시스템과는 무관하다(자세한 키 목록은 그
    클래스의 헤더 주석 참고). InGameUI는 `ACPTopDownPlayerController::SetupInGameWidget()`에서
    Z-order 1로 추가되어 - 한 틱 뒤 Z-order 0으로 추가되는 `CoinPusherCaptureWidget`(PIP)보다
    생성 순서와 무관하게 항상 위에 그려진다(Pause/Ending의 Z-order 20보다는 낮음). 자세한 내용은
    `CoinPusher/README.md`의 테스트 절도 참고
