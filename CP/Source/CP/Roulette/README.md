# Roulette 관련 C++

## 개요

원형 돌림판 형태의 룰렛 기능. `Roll(PlayerLevel)`을 호출하면 `RouletteDataTable`(Row Struct는
`FCPRouletteDataRow`, `Datatables/CPRouletteDataTypes.h`)의 각 행을 나열 순서대로 인덱스(0..N-1)
삼아 추첨 후보로 삼는다. `PlayerLevel`과 `MustPickLevel`이 같은 행이 있고 그 레벨에서 아직 강제
당첨을 적용한 적이 없으면 그 행(들) 중 균등 확률로 반드시 하나를 당첨시킨다(한 번 적용되면 같은
레벨에서는 다음부터 건너뛰고, 다음 레벨이 되어야 다시 강제 당첨 대상이 된다). 그 외의 경우에는
`RouletteProbabilityDataTable`(Row Struct는
`FCPRouletteProbabilityRow`)에서 `Level`이 `PlayerLevel`과 같은 행을 찾아 그 행의
`Roulette_index0`~`Roulette_index9` 가중치로 후보 인덱스를 추첨한다. 화면 중앙 위쪽에서 아래로
등장하는 UI가 그 칸에서 멈추는 연출을 보여준 뒤, 당첨된 행의 `PickUpImage`를 잠시 보여주는 PickUp
연출까지 끝나야 `ItemID`/`PickEA`를 `OnPickedUp`으로 Broadcast한다. `ACPRoulette`는 그 결과를
누가 어떻게 쓰는지 전혀 모른다 — 외부 시스템(예:
`ACPCoinPusher::LinkedRoulette`)이 `OnPickedUp`에 직접 바인딩해서 원하는 대로 처리한다. 당첨된
`ItemID`의 실제 표시/스폰 정보(아이콘, 스폰 클래스 등)는 `ItemDataTable`(Row Struct는 `FItemData`,
`Datatables/CPItemData.h`)에서 조회한다. `ACPCoinPusher`와 마찬가지로 C++ 클래스는
`UCLASS(abstract)`로 선언되어 있으며, 실제 메시/UMG 비주얼은 이를 상속한 Blueprint에서 채워 넣는다.

## Class 구조

- `ACPRoulette` : 룰렛 로직을 갖는 Actor. 로컬 스플릿 스크린으로 두 플레이어가 하나의 룰렛 Actor를 공유한다.
  룰렛 UI 위젯은 이 Actor가 직접 만들지 않고, 각 로컬 플레이어의 "InGameUI"
  (`ACPTopDownPlayerController::GetInGameWidget()`, `UI/CPInGameWidget.h`)가 이미 갖고 있는
  `RouletteWidget` 컴포넌트를 그대로 재사용한다 (아래 "InGameUI 연동" 참고)
    - `SpawnPoint` (`USceneComponent`, RootComponent) : 룰렛 액터의 기준 위치/방향
    - `ItemDataTable` (`TObjectPtr<UDataTable>`, `EditAnywhere`, Row Struct `FItemData`) : 당첨된
      `ItemID`의 표시/스폰 정보를 조회하는 용도로만 쓰인다 (추첨 후보/확률에는 관여하지 않음). 자세한
      필드 설명은 `Datatables/README.md` 참고
    - `RouletteDataTable` (`TObjectPtr<UDataTable>`, `EditAnywhere`, Row Struct `FCPRouletteDataRow`) :
      추첨 후보 테이블. 각 행이 후보 하나이며, 행의 나열 순서가 곧 추첨 인덱스(0..N-1)다. `ItemID`
      (`ItemDataTable`의 ID와 매칭), `PickEA`(당첨 시 지급 개수), `bRouletteToCoinPusher`(당첨 시
      CoinPusher로 전달되는지), `MustPickLevel`(`PlayerLevel`과 같으면 이 행이 반드시 당첨되는 강제
      레벨), `PickUpImage`(`TObjectPtr<UTexture2D>`, 스핀이 끝난 뒤 `UCPRouletteWidget`의
      `PickUpImage`에 잠시 보여줄 이미지) 필드를 가짐
    - `RouletteProbabilityDataTable` (`TObjectPtr<UDataTable>`, `EditAnywhere`, Row Struct
      `FCPRouletteProbabilityRow`) : 팀 레벨별 확률 가중치 테이블. 각 행의 `Level` 필드로 검색하며,
      `Roulette_index0`~`Roulette_index9`(최대 10개 후보까지 지원)가 `RouletteDataTable`의 후보
      인덱스에 대응하는 가중치. `Level`이 `PlayerLevel`과 일치하는 행이 없으면 테이블에 정의된 순서상
      가장 마지막 행의 가중치를 그대로 사용한다
    - `bIsRolling` : 스핀이 시작되어 결과가 결정될 때까지 true. 한 플레이어가 `Roll()`을 호출해 스핀
      중일 때 다른 플레이어가 `Roll()`을 호출해도 무시되도록 막는 잠금 상태 (`IsRolling()`으로 조회 가능)
    - `PendingResultItemID` / `PendingResultSpawnCount` / `PendingResultPickUpImage` : `Roll()`
      시점(추첨 직후)에 확정된 당첨 ItemID/개수/PickUp 이미지를 캐싱해두는 멤버.
      `PendingResultPickUpImage`는 그대로 위젯의 `PlaySpin()`에 전달되어 PickUp 연출에 쓰이고,
      `PendingResultItemID`/`PendingResultSpawnCount`는 위젯의 스핀+PickUp 연출이 모두 끝나면 그대로
      `OnPickedUp`으로 전달한다 — 스핀 도중 데이터 테이블이 바뀌거나 순서가 달라져도 결과가 흔들리지
      않도록 하기 위함
    - `OnPickedUp` (`FOnCPRoulettePickedUp`, `ItemID`/`Count` 매개변수) : 행이 당첨(아이템이 뽑힘)될
      때마다 Broadcast. 이 결과를 누가 어떻게 처리할지는 전혀 모르므로, `ACPCoinPusher` 등 외부 시스템이
      여기에 직접 바인딩해서 사용한다 (아래 "CoinPusher 연동" 참고)
    - `Roll(PlayerLevel)` : 월드의 GameMode가 `ACPGameMode`라면 팀 티켓을 1개 소모해야 하며(부족하면
      `false` 반환), `ACPGameMode`가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다.
      `PlayerLevel`은 `MustPickLevel` 강제 당첨 판정과 `RouletteProbabilityDataTable` 조회에
      쓰이며, 호출자가 팀 레벨 등을 직접 구해서 넘겨줘야 한다. 로컬 스플릿 스크린의 모든 플레이어
      화면에 동일한 룰렛 UI를 동시에 재생한 뒤 당첨 정보를 전달한다
- `UCPRouletteWidget` : 룰렛 UI (UMG). `PlaySpin(ResultIndex, CandidateCount, PickUpTexture)`가
  호출되면 화면 중앙 위쪽에서 아래로 슬라이드하며 등장한 뒤, `WheelImage`를 여러 바퀴 돌려
  `ResultIndex`번째 칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다. 칸이 멈추면 곧바로
  결과를 알리지 않고 `PickUpImage`를 `PickUpTexture`로 채워 `PickUpDisplayDuration`(기본 1.5초)
  동안 보여준 뒤, 그 시간이 지나면 `PickUpImage`가 사라짐과 동시에 `OnResultDetermined`를
  브로드캐스트하고 위젯 자신도 함께 `Collapsed`되어 사라진다. `ResultIndex`/`CandidateCount`는
  이번 `Roll()` 한 번에 한정된 추첨 후보 목록 안에서의 순번일 뿐, 특정 아이템을 가리키는 영구적인
  인덱스가 아니므로 오직 스핀 연출(몇 칸 중 몇 번째에서 멈추는지)에만 쓰인다

## 클래스별 상세

### ACPRoulette
- `Roll(PlayerLevel)` : `bIsRolling`이 true면(다른 플레이어가 이미 돌리는 중이면) 아무 동작도 하지 않고
  `false`를 반환한다. `PickWeightedItem(PlayerLevel, ...)`으로 먼저 당첨 후보를 추첨해보고(뽑을 수
  있는 행이 하나도 없으면 티켓을 소모하지 않고 바로 `false` 반환), 성공하면 그다음 월드의 GameMode가
  `ACPGameMode`인 경우에만 `TrySpendTeamTicket(1)`이 성공해야 진행되며(실패 시 `false` 반환),
  `ACPGameMode`가 아니면(테스트 레벨 등) 티켓 검사를 건너뛴다. 이후 `GetLocalRouletteWidgets()`가
  반환한 모든(로컬 스플릿 스크린) 위젯에 대해 `PlaySpin(ResultIndex, CandidateCount, PendingResultPickUpImage)`를
  호출해 두 플레이어의 화면에 동시에 같은 연출이 나오도록 한다. 위젯이 하나도 없으면(InGameUI 미생성,
  RouletteWidget 미배치 등) UI 없이 바로 `HandleRouletteResultDetermined()`를 호출하는 폴백 동작.
  성공적으로 스핀을 시작했으면 `true` 반환
- `PickWeightedItem(PlayerLevel, OutResultIndex, OutCandidateCount)` : `RouletteDataTable->GetRowNames()`
  순서대로 `FCPRouletteDataRow` 후보 목록을 만든다(이 순서가 곧 추첨 인덱스). 후보 중 `MustPickLevel`이
  `PlayerLevel`과 같은 행이 하나 이상 있고, `LastMustPickAppliedLevel`(마지막으로 강제 당첨을 적용한
  레벨)이 `PlayerLevel`과 다르면 그 행들 중 균등 확률로 반드시 하나를 당첨시키고
  `LastMustPickAppliedLevel`을 `PlayerLevel`로 갱신한다 - 즉 같은 레벨에서는 한 번만 강제 당첨이
  나가고, 다음 레벨이 될 때까지는 다시 나가지 않는다. 그 외의 경우에는
  `RouletteProbabilityDataTable`에서 `Level`이 `PlayerLevel`과 같은 행을 찾아 그 행의
  `Roulette_index0`~`9` 가중치로 추첨한다 - 일치하는 `Level` 행이 없으면 테이블에 정의된 순서상 가장
  마지막 행의 가중치를 그대로 사용하고, 테이블이 비어있거나 가중치 합이 0 이하면(설정 실수 등) 균등
  확률로 대체한다. 뽑힌 행의 `ItemID`/`PickEA`/`PickUpImage`를
  `PendingResultItemID`/`PendingResultSpawnCount`/`PendingResultPickUpImage`에 저장하고, 위젯 스핀
  연출용으로 후보 목록 안에서의 순번(`OutResultIndex`)과 전체 후보 수(`OutCandidateCount`)를
  반환한다. 후보가 하나도 없으면(`RouletteDataTable` 미지정 포함) `false` 반환
- `GetLocalRouletteWidgets()` : 월드의 로컬 `ACPTopDownPlayerController`(스플릿 스크린 인원 수만큼)
  마다 `GetInGameWidget()->GetRouletteWidget()`을 조회해서 모은다 - 위젯을 새로 만들지 않고 이미
  InGameUI 안에 존재하는 인스턴스를 그대로 재사용하며, 조회할 때마다 `OnResultDetermined`에
  `HandleRouletteResultDetermined()`를 `AddUniqueDynamic`으로 바인딩(중복 바인딩 안전). InGameUI가
  없거나 `RouletteWidget`이 WBP에 배치되지 않은 플레이어는 결과 목록에서 제외된다
- `HandleRouletteResultDetermined(ResultIndex)` : 스플릿 스크린 위젯마다 각자 스핀을 멈추고 칸을
  확정하면 호출되므로, `bIsRolling`이 false면(이미 처리된 결과면) 무시해 당첨 정보가 중복
  전달되지 않도록 한다. 최초 호출에서 `bIsRolling`을 false로 되돌려 잠금을 풀고
  `OnPickedUp.Broadcast(PendingResultItemID, PendingResultSpawnCount)`를 호출

### UCPRouletteWidget
- `WheelImage` (`BindWidgetOptional`) : 칸이 그려진 회전판 이미지. 위쪽 화살표는 고정된 비주얼
  요소이므로 BP에서 `WheelImage` 위에 배치하기만 하면 되고 별도 C++ 바인딩은 필요 없음
- `PickUpImage` (`BindWidgetOptional`) : 스핀이 멈춘 직후 당첨 아이템 이미지를 잠시 보여주는 PickUp
  연출용 이미지. `NativeConstruct`/`PlaySpin` 시작 시점에는 항상 `Collapsed`로 숨겨져 있다가,
  스핀이 끝나면 `PlaySpin`에 전달된 텍스처로 채워져 `PickUpDisplayDuration` 동안만 보인다
- `PlaySpin(ResultIndex, NumSlots, PickUpTexture)` : 매 호출마다 위젯을 `EnterStartOffsetY`(화면
  위쪽)에서 다시 등장시키고, `PickUpImage`를 초기화한 뒤 새 스핀을 시작 (연속 호출 시 이전 연출을
  덮어씀 - `Player/CPItemToastWidget`과 동일한 재시작 방식). `PickUpTexture`는 스핀이 끝난 뒤
  `PickUpImage`에 채울 텍스처로, 널이면 텍스처를 갱신하지 않는다
- 내부적으로 `Entering`(등장 슬라이드) → `Spinning`(회전) → `ShowingPickUp`(`PickUpImage` 노출) 세
  단계를 `NativeTick`에서 진행한다. `Entering`/`Spinning`은 `FMath::InterpEaseOut`으로 보간하며,
  `Spinning`이 끝나면 `ShowPickUp()`이 `PickUpImage`를 채워 보이게 하고(`PickUpImage`가 없으면
  대기 없이 바로 `FinishSpin()`으로 넘어감), `PickUpDisplayDuration`이 지나면 `FinishSpin()`이
  `PickUpImage`를 다시 숨기고 `OnResultDetermined`를 브로드캐스트한 뒤 위젯 자신도 곧바로
  `SetVisibility(Collapsed)`로 함께 비활성화한다 - PickUp 이미지가 사라지는 시점과 룰렛 UI가
  꺼지는 시점이 항상 같음

## InGameUI 연동

`ACPRoulette`는 더 이상 자기만의 룰렛 위젯을 생성/관리하지 않는다(과거의 `RouletteWidgetClass`/
`RouletteWidgetInstances`는 제거됨). 대신 `UI/CPInGameWidget.h`(`UCPInGameWidget`, "InGameUI")가
`RouletteWidget`(`UCPRouletteWidget` 상속 WBP)을 `BindWidgetOptional` 컴포넌트로 갖고 있고,
`ACPTopDownPlayerController`가 이 InGameUI를 로컬 플레이어당 하나씩 `BeginPlay`에서 생성/캐싱한다
(`InGameWidgetClass`, `GetInGameWidget()`). `ACPRoulette::GetLocalRouletteWidgets()`가 매
`Roll()`마다 각 로컬 플레이어의 `GetInGameWidget()->GetRouletteWidget()`을 조회해서 그 인스턴스에
직접 `PlaySpin()`을 호출하는 구조 - 룰렛 전용 WBP를 별도로 관리할 필요 없이, `WBP_InGameWidget`
안에 `RouletteWidget`이라는 이름으로 `UCPRouletteWidget` 상속 위젯을 하나 배치해두기만 하면 된다.

## CoinPusher 연동

`ACPRoulette`는 당첨 결과를 누가 쓰는지 전혀 모르는 순수 pub-sub 구조다. `ACPCoinPusher`가
`LinkedRoulette`(`TObjectPtr<ACPRoulette>`, `EditInstanceOnly`) 프로퍼티로 연동할 룰렛을 직접
참조하고, `BeginPlay()`에서 `LinkedRoulette->OnPickedUp.AddDynamic(this, &ACPCoinPusher::HandleRoulettePickedUp)`
으로 자신의 `HandleRoulettePickedUp(ItemID, SpawnCount)`를 구독한다 — 룰렛에서 아이템이 뽑힐 때마다
CoinPusher가 자신의 `ItemDataTable`에서 그 `ItemID`의 `FItemData::bRouletteToCoinPusher`를 확인해,
true인 경우에만 `ItemSpawn(ItemID, SpawnCount)`으로 천장 Dispenser 중 하나에서 그 아이템을 스폰한다.
자세한 내용은 `CoinPusher/README.md`의 `ACPCoinPusher` 섹션 참고.

**당첨된 아이템이 스폰되지 않을 때 진단 방법**: `ACPRoulette::HandleRouletteResultDetermined()`가
당첨된 `ItemID`/`SpawnCount`를 `LogRoulette`(Warning)로 항상 남기므로, 이 로그가 찍히는지부터
확인한다. 그 다음 이어지는 파이프라인(`LinkedRoulette` 미지정 → `ItemDataTable` 미지정/Row 없음/
`bRouletteToCoinPusher=false` → 유효한 천장 Dispenser 없음 → Dispenser의 `ItemDataTable`에 Row
없음/`CoinPusherSpawnBPClass` 없음/`ICPCoinPusherItem` 미구현 → `SpawnActor` 실패)의 각 단계가
전부 `LogCoinPusher`(Warning)로 로그를 남기므로, `LogRoulette`/`LogCoinPusher` 로그를 순서대로
따라가면 어느 단계에서 끊겼는지 바로 확인할 수 있다 (자세한 각 로그는 `CoinPusher/README.md`의
`ACPCoinPusher`/`ACPDispenser` 섹션 참고).

## 테스트 (CoinPusher/Test)

- `ACPCoinPusherItemSpawnTestPawn` : R 키를 누르면 `TargetRoulette->Roll(PlayerLevel)`을 호출
  (미지정 시 `BeginPlay`에서 레벨에 배치된 아무 `ACPRoulette`나 자동으로 찾아 사용,
  `TargetCoinPusher`와 동일한 방식). `PlayerLevel`은 `ACPGameMode::GetTeamLevel()`에서 조회하고,
  월드의 GameMode가 `ACPGameMode`가 아니면 1로 대체한다. 시작 여부(`bool` 반환값)를
  `UE_LOG(LogRoulette, Warning, ...)`으로 표시. 당첨 결과 자체는 `TargetRoulette`의
  `LinkedRoulette`로 연결된 `ACPCoinPusher`(있다면)의 `HandleRoulettePickedUp()`으로 자동
  전달되므로 이 Pawn이 직접 결과를 처리하지 않음

## 에디터에서 준비해야 할 것

1. `WBP_CPRoulette`(`UCPRouletteWidget` 상속) 생성: 화면 중앙에 위쪽 화살표 이미지와,
   `WheelImage`라는 이름의 회전판 `Image`를 배치 (이름이 일치해야 `BindWidgetOptional`이 연결됨).
   PickUp 연출을 쓰려면 `PickUpImage`라는 이름의 `Image`도 함께 배치한다(선택 사항 - 없으면 PickUp
   연출 없이 `PickUpDisplayDuration`만큼 대기 후 바로 결과가 처리됨). 이 WBP는 `ACPRoulette`가
   아니라 `WBP_InGameWidget`(`UI/README.md` 참고)의 `RouletteWidget` 슬롯에 배치한다
2. `BP_CPRoulette`(`ACPRoulette` 상속) 생성 후 다음 3개의 DataTable 에셋을 연결
   (`Datatables/README.md` 참고):
   - `ItemDataTable` : Row Struct `FItemData`. 당첨된 `ItemID`의 표시/스폰 정보 조회용
   - `RouletteDataTable` : Row Struct `FCPRouletteDataRow`. 추첨 후보 하나당 한 행 - `ItemID`
     (`ItemDataTable`의 ID와 일치해야 함), `PickEA`, `bRouletteToCoinPusher`, `MustPickLevel`,
     `PickUpImage`(스핀이 끝난 뒤 `WBP_CPRoulette`의 `PickUpImage`에 보여줄 텍스처)를 원하는 값으로
     설정. 행을 만든 순서가 곧 추첨 인덱스이므로 `RouletteProbabilityDataTable`의 `Roulette_indexN`
     컬럼과 순서를 맞춰야 한다 (최대 10개 후보까지 지원)
   - `RouletteProbabilityDataTable` : Row Struct `FCPRouletteProbabilityRow`. 팀 레벨 하나당 한
     행 - `Level`에 해당 레벨 값을, `Roulette_index0`~`9`에 `RouletteDataTable`의 인덱스별 확률
     가중치를 설정 (합이 1일 필요는 없음, 상대 비율로만 반영됨)
3. `BP_CPRoulette` 인스턴스를 레벨에 배치한 뒤, 이 결과를 받을 `BP_CPCoinPusher` 인스턴스의
   `LinkedRoulette`에 방금 배치한 `BP_CPRoulette`를 연결 (연결하지 않으면 당첨되어도 아이템이
   스폰되지 않음). 해당 CoinPusher의 천장 Dispenser가 참조하는 `ItemDataTable`에도 룰렛과 동일한
   `ItemID`(RowName)가 등록되어 있어야 실제로 스폰됨
4. 필요한 곳(상호작용, 게임 로직 등)에서 `BP_CPRoulette` 인스턴스의 `Roll(PlayerLevel)`을 호출
   (`PlayerLevel`은 호출자가 `ACPGameMode::GetTeamLevel()` 등으로 직접 구해서 넘겨줘야 함). 이때
   화면에 스핀이 뜨려면 `ACPTopDownPlayerController`를 상속하는 BP의 `InGameWidgetClass`에
   (1번에서 `RouletteWidget`을 배치한) `WBP_InGameWidget`이 지정돼 있어야 한다 - 없으면 `Roll()`은
   정상 동작하지만(폴백) 화면에 스핀 연출 없이 결과만 `OnPickedUp`으로 전달된다
