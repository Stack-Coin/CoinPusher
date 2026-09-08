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

### 체력바

- `UCPHealthBarWidget` : 체력바 WBP의 베이스. `BackgroundImage`(배경 틀) + `FillImage`(실제 체력을
  나타내는 앞쪽 이미지, 둘 다 `BindWidgetOptional`)로 구성. `UpdateHealth(Current, Max)`를
  호출(또는 델리게이트 바인딩)하면 Percent를 계산해 `SetHealthPercent`를 호출하는데, 기본 구현이
  `FillImage->SetRenderScale(FVector2D(Percent, 1.0f))`로 가로 폭을 자동으로 줄여주므로 **WBP에서
  별도 그래프 작업 없이도 바로 동작**한다. `NativeConstruct`에서 `FillImage`의 Render Transform
  Pivot을 코드로 직접 `(0.0, 0.5)`(왼쪽 기준)로 맞춰주기 때문에 **왼쪽 끝은 고정된 채 오른쪽에서
  왼쪽으로 줄어드는 방향**으로 동작한다 - WBP 디자이너에서 Pivot을 따로 설정할 필요 없음.
  색상 변화 등 커스텀 연출이 필요하면 `SetHealthPercent`가 `BlueprintNativeEvent`라 WBP에서
  오버라이드해 추가할 수 있다(Super 호출 여부는 자유). `SetHealthValues(Current, Max)`(BP 이벤트)
  는 텍스트 표시 등에 값 그대로 필요할 때 사용. 월드 스페이스/뷰포트 스페이스 어느 쪽에서도 동일하게
  재사용

  > **WBP 배치**: `BackgroundImage`를 뒤에, `FillImage`를 그 앞에 겹쳐 배치(Overlay 또는 Canvas
  > Panel에서 순서로 조절)하면 된다. `FillImage`의 Anchor/Alignment는 왼쪽 정렬(Horizontal
  > Alignment: Left)로 둬야 줄어들 때 오른쪽 끝만 안쪽으로 들어오는 모양이 된다
- `UCPHealthBarComponent` : `UWidgetComponent` 기반. 아무 Actor의 BP에나 Add Component로 붙이면
  그 위에 월드 스페이스 체력바가 뜬다 (적/보스 머리 위 등)
- `UCPViewportHealthBarComponent` : `UActorComponent` 기반. BeginPlay에 `UCPHealthBarWidget`
  WBP를 생성해 화면(뷰포트 또는 로컬 플레이어 화면)에 고정 표시한다 (플레이어 자신의 HUD 체력바 등)

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
  (타이틀 화면 "Press Any Button" 등)

  > **왜 `NativeOnKeyDown`/`SetUserFocus` 대신 `IInputProcessor`인가**: 처음엔
  > `NativeOnKeyDown`/`NativeOnMouseButtonDown` + `SetUserFocus`로 구현했었는데, 실제로
  > 테스트해보니 두 가지 이유로 신뢰할 수 없었다 - (1) 마우스 클릭이 위젯의 히트테스트 가능한
  > 영역을 못 맞히면 클릭 자체가 포커스를 날려버림(`NativeOnFocusLost` Cause=Mouse), (2)
  > 게임패드 입력은 `SetUserFocus`가 쓰는 레거시 `ControllerId` 기반 Slate User와 실제 게임패드
  > 키 이벤트가 라우팅되는 Slate User가 서로 어긋나 포커스가 있어도 이벤트가 전달되지 않음.
  > `IInputProcessor`는 Slate가 포커스/히트테스트로 이벤트를 어디로 보낼지 정하기 이전 단계에서
  > 가로채므로 이 두 문제 모두와 무관하게 항상 동작한다

### 게임 종료 화면

- `UCPGameOverWidget` : `RestartLevel()` — 현재 레벨을 다시 로드 (재시작 버튼 등에서 호출)
- `UCPGameClearWidget` : `NextLevelName`(EditAnywhere) + `GoToNextLevel()` — 지정한 레벨로 이동
  (다음 스테이지/타이틀로 버튼 등에서 호출)

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

## 에디터에서 준비해야 할 것

모든 C++ 클래스는 `UCLASS(abstract)`라 실제 사용하려면 Blueprint가 필요하다:

1. 각 Widget 클래스(`UCPHealthBarWidget`, `UCPRadialGaugeWidget`, `UCPCoinCountWidget`,
   `UCPTicketCountWidget`, `UCPTimeDisplayWidget`, `UCPPressAnyKeyWidget`, `UCPPlayerJoinWidget`)
   를 부모로 하는 WBP를 만들고 비주얼(ProgressBar/Image/TextBlock 등, `BindWidgetOptional`
   변수와 이름을 맞춰서 배치)과 표시 문구(`DisplayFormat` 등)를 채운다
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
