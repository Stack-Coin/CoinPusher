# Roulette 관련 C++

## 개요

원형 돌림판 형태의 룰렛 기능. `Roll()`을 호출하면 `ItemDataTable`(Row Struct는 `FItemData`,
`Source/CP/Datatables/CPItemData.h`)에서 `bRoulette`가 true인 행들 중 하나가 각 행의
`RouletteProbability` 가중치에 비례해 당첨되고, 화면 중앙 위쪽에서 아래로 등장하는 UI가 그 칸에서
멈추는 연출을 보여준 뒤, 당첨된 행의 `ItemID`(=Row Name)/`RouletteSpawnCount`를 `OnPickedUp`으로
Broadcast한다. `ACPRoulette`는 그 결과를 누가 어떻게 쓰는지 전혀 모른다 — 외부 시스템(예:
`ACPCoinPusher::LinkedRoulette`)이 `OnPickedUp`에 직접 바인딩해서 원하는 대로 처리한다. `ACPCoinPusher`와
마찬가지로 C++ 클래스는 `UCLASS(abstract)`로 선언되어 있으며, 실제 메시/UMG 비주얼은 이를 상속한
Blueprint에서 채워 넣는다.

## Class 구조

- `ACPRoulette` : 룰렛 로직을 갖는 Actor. 로컬 스플릿 스크린으로 두 플레이어가 하나의 룰렛 Actor를 공유한다.
  룰렛 UI 위젯은 이 Actor가 직접 만들지 않고, 각 로컬 플레이어의 "InGameUI"
  (`ACPTopDownPlayerController::GetInGameWidget()`, `UI/CPInGameWidget.h`)가 이미 갖고 있는
  `RouletteWidget` 컴포넌트를 그대로 재사용한다 (아래 "InGameUI 연동" 참고)
    - `SpawnPoint` (`USceneComponent`, RootComponent) : 룰렛 액터의 기준 위치/방향
    - `ItemDataTable` (`TObjectPtr<UDataTable>`, `EditAnywhere`) : 룰렛이 아이템을 뽑을 때 사용할
      데이터 테이블 (Row Struct는 `FItemData`여야 함). `bRoulette`가 true인 행만 후보가 되며,
      `RouletteProbability` 가중치로 추첨해 `RouletteSpawnCount`개를 전달한다. 자세한 필드 설명은
      `Datatables/README.md` 참고
    - `bIsRolling` : 스핀이 시작되어 결과가 결정될 때까지 true. 한 플레이어가 `Roll()`을 호출해 스핀
      중일 때 다른 플레이어가 `Roll()`을 호출해도 무시되도록 막는 잠금 상태 (`IsRolling()`으로 조회 가능)
    - `PendingResultItemID` / `PendingResultSpawnCount` : `Roll()` 시점(추첨 직후)에 확정된 당첨
      ItemID/개수를 캐싱해두는 멤버. 위젯의 스핀 애니메이션이 끝나면 이 값을 그대로 `OnPickedUp`으로
      전달한다 — 스핀 도중 `ItemDataTable`이 바뀌거나 순서가 달라져도 결과가 흔들리지 않도록 하기 위함
    - `OnPickedUp` (`FOnCPRoulettePickedUp`, `ItemID`/`Count` 매개변수) : 행이 당첨(아이템이 뽑힘)될
      때마다 Broadcast. 이 결과를 누가 어떻게 처리할지는 전혀 모르므로, `ACPCoinPusher` 등 외부 시스템이
      여기에 직접 바인딩해서 사용한다 (아래 "CoinPusher 연동" 참고)
    - `Roll()` : 월드의 GameMode가 `ACPGameMode`라면 팀 티켓을 1개 소모해야 하며(부족하면 `false`
      반환), `ACPGameMode`가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다. `ItemDataTable`에서
      `bRoulette`인 행들을 `RouletteProbability` 가중치로 추첨하고, 로컬 스플릿 스크린의 모든 플레이어
      화면에 동일한 룰렛 UI를 동시에 재생한 뒤 당첨 정보를 전달한다
- `UCPRouletteWidget` : 룰렛 UI (UMG). `PlaySpin(ResultIndex, CandidateCount)`가 호출되면
  화면 중앙 위쪽에서 아래로 슬라이드하며 등장한 뒤, `WheelImage`를 여러 바퀴 돌려 `ResultIndex`번째
  칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다. 칸이 결정되면 `OnResultDetermined`를
  브로드캐스트하고, `PostResultHideDelay`(기본 2초) 후 스스로 사라진다. `ResultIndex`/`CandidateCount`는
  이번 `Roll()` 한 번에 한정된 추첨 후보 목록 안에서의 순번일 뿐, 특정 아이템을 가리키는 영구적인
  인덱스가 아니므로 오직 스핀 연출(몇 칸 중 몇 번째에서 멈추는지)에만 쓰인다

## 클래스별 상세

### ACPRoulette
- `Roll()` : `bIsRolling`이 true면(다른 플레이어가 이미 돌리는 중이면) 아무 동작도 하지 않고
  `false`를 반환한다. `PickWeightedItem()`으로 먼저 당첨 후보를 추첨해보고(뽑을 수 있는 행이 하나도
  없으면 티켓을 소모하지 않고 바로 `false` 반환), 성공하면 그다음 월드의 GameMode가 `ACPGameMode`인
  경우에만 `TrySpendTeamTicket(1)`이 성공해야 진행되며(실패 시 `false` 반환), `ACPGameMode`가
  아니면(테스트 레벨 등) 티켓 검사를 건너뛴다. 이후 `GetLocalRouletteWidgets()`가 반환한
  모든(로컬 스플릿 스크린) 위젯에 대해 `PlaySpin(ResultIndex, CandidateCount)`을 호출해 두 플레이어의
  화면에 동시에 같은 연출이 나오도록 한다. 위젯이 하나도 없으면(InGameUI 미생성, RouletteWidget
  미배치 등) UI 없이 바로 `HandleRouletteResultDetermined()`를 호출하는 폴백 동작. 성공적으로
  스핀을 시작했으면 `true` 반환
- `PickWeightedItem(OutResultIndex, OutCandidateCount)` : `ItemDataTable->GetRowMap()`을 순회해
  `bRoulette`가 true인 행만 모아 `RouletteProbability` 가중치에 비례해 하나를 추첨한다(모든 후보의
  가중치 합이 0 이하면 균등 확률로 대체). 뽑힌 행의 ItemID(=Row Name)/`RouletteSpawnCount`를
  `PendingResultItemID`/`PendingResultSpawnCount`에 저장하고, 위젯 스핀 연출용으로 후보 목록 안에서의
  순번(`OutResultIndex`)과 전체 후보 수(`OutCandidateCount`)를 반환한다. 후보가 하나도 없으면
  (`ItemDataTable` 미지정 포함) `false` 반환
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
- `PlaySpin(ResultIndex, NumSlots)` : 매 호출마다 위젯을 `EnterStartOffsetY`(화면 위쪽)에서
  다시 등장시키고, 진행 중이던 숨김 타이머를 취소한 뒤 새 스핀을 시작 (연속 호출 시 이전 연출을
  덮어씀 - `Player/CPItemToastWidget`과 동일한 재시작 방식)
- 내부적으로 `Entering`(등장 슬라이드) → `Spinning`(회전) 두 단계를 `NativeTick`에서 `FMath::InterpEaseOut`으로
  보간하며, `Spinning`이 끝나면 `FinishSpin()`이 `OnResultDetermined`를 브로드캐스트하고
  `PostResultHideDelay` 후 `HideRoulette()`으로 스스로 `Collapsed` 처리

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

## 추가 데이터 테이블 (RouletteDataTable / LevelRouletteProbabilityDataTable)

`ItemDataTable`(`FItemData`, `Datatables/CPItemData.h`)은 그대로 두고, 룰렛에서만 의미 있는
부가 규칙을 담기 위해 완전히 별개의 신규 테이블 2개를 `Roulette/CPRouletteDataTypes.h`에 Row
Struct로만 정의해뒀다(둘 다 아직 어떤 추첨 로직에도 반영되어 있지 않음 - 데이터 스키마 정의까지만
우선 준비된 상태). `ACPRoulette`에 직접 슬롯을 두지 않고, `Datatables/CPItemDataTableGameInstance.h`
(`ItemDataTable`을 들고 있는 기존 GameInstance)와 같은 패턴으로 전용 GameInstance 클래스
`Datatables/CPRouletteDataTableGameInstance.h`(`UCPRouletteDataTableGameInstance`)를 새로
만들어 그 안에서 관리한다:

- `UCPRouletteDataTableGameInstance` (`UCPItemDataTableGameInstance` 상속 - 기존 `ItemDataTable`/
  `GetItemDataTable()`은 그대로 물려받아 손대지 않음)
  - `RouletteDataTable`(`TObjectPtr<UDataTable>`, Row Struct `FCPRouletteDataRow`) : `ID`(항목
    식별, `ItemDataTable`의 ID와 매칭), `bRouletteToCoinPusher`(당첨 시 CoinPusher로 전달되는지),
    `MustLevelPick`(이 항목이 후보로 뽑히기 위해 팀이 최소로 도달해 있어야 하는 레벨) -
    `GetRouletteDataTable()`로 조회
  - `LevelRouletteProbabilityDataTable`(`TObjectPtr<UDataTable>`, Row Struct
    `FCPLevelRouletteProbabilityRow`) : `Level`(팀 레벨), `Probability`(그 레벨에 적용할 확률
    가중치) - 레벨 1~9 각각 1행씩 총 9행으로 채워 쓴다 - `GetLevelRouletteProbabilityDataTable()`로 조회
- 실제 적용하려면 기존 `BP_ItemDataTableGameInstance`(또는 그 상속 BP)의 부모 클래스를
  `UCPRouletteDataTableGameInstance`로 재부모(Reparent)하면 된다 - Project Settings의 Game
  Instance Class 자체는 그대로 두고 BP만 재부모하면 되므로 별도 프로젝트 설정 변경은 필요 없다

## 테스트 (CoinPusher/Test)

- `ACPCoinPusherItemSpawnTestPawn` : R 키를 누르면 `TargetRoulette->Roll()`을 호출 (미지정 시
  `BeginPlay`에서 레벨에 배치된 아무 `ACPRoulette`나 자동으로 찾아 사용, `TargetCoinPusher`와 동일한
  방식). 시작 여부(`bool` 반환값)를 `UE_LOG(LogRoulette, Warning, ...)`으로 표시. 당첨 결과 자체는
  `TargetRoulette`의 `LinkedRoulette`로 연결된 `ACPCoinPusher`(있다면)의 `HandleRoulettePickedUp()`으로
  자동 전달되므로 이 Pawn이 직접 결과를 처리하지 않음

## 에디터에서 준비해야 할 것

1. `WBP_CPRoulette`(`UCPRouletteWidget` 상속) 생성: 화면 중앙에 위쪽 화살표 이미지와,
   `WheelImage`라는 이름의 회전판 `Image`를 배치 (이름이 일치해야 `BindWidgetOptional`이 연결됨).
   이 WBP는 `ACPRoulette`가 아니라 `WBP_InGameWidget`(`UI/README.md` 참고)의 `RouletteWidget`
   슬롯에 배치한다
2. `BP_CPRoulette`(`ACPRoulette` 상속) 생성 후 `ItemDataTable`에 Row Struct가 `FItemData`인
   DataTable 에셋을 연결 (`Datatables/README.md` 참고). 룰렛에서 뽑히길 원하는 행마다 `bRoulette`를
   true로, `RouletteProbability`/`RouletteSpawnCount`를 원하는 값으로 설정
3. `BP_CPRoulette` 인스턴스를 레벨에 배치한 뒤, 이 결과를 받을 `BP_CPCoinPusher` 인스턴스의
   `LinkedRoulette`에 방금 배치한 `BP_CPRoulette`를 연결 (연결하지 않으면 당첨되어도 아이템이
   스폰되지 않음). 해당 CoinPusher의 천장 Dispenser가 참조하는 `ItemDataTable`에도 룰렛과 동일한
   `ItemID`(RowName)가 등록되어 있어야 실제로 스폰됨
4. 필요한 곳(상호작용, 게임 로직 등)에서 `BP_CPRoulette` 인스턴스의 `Roll()`을 호출. 이때 화면에
   스핀이 뜨려면 `ACPTopDownPlayerController`를 상속하는 BP의 `InGameWidgetClass`에 (1번에서
   `RouletteWidget`을 배치한) `WBP_InGameWidget`이 지정돼 있어야 한다 - 없으면 `Roll()`은 정상
   동작하지만(폴백) 화면에 스핀 연출 없이 결과만 `OnPickedUp`으로 전달된다
5. (선택) 위 "추가 데이터 테이블" 절의 두 테이블을 실제로 쓰려면, Row Struct를 각각
   `FCPRouletteDataRow`/`FCPLevelRouletteProbabilityRow`로 지정해 DataTable 에셋을 만들고,
   기존 `BP_ItemDataTableGameInstance`(또는 그 상속 BP)의 부모 클래스를
   `UCPRouletteDataTableGameInstance`로 재부모한 뒤 `RouletteDataTable`/
   `LevelRouletteProbabilityDataTable`에 연결한다 - 다만 현재는 어떤 추첨 로직도 이 값을 읽지
   않으므로, 실제로 반영하려면 `ACPRoulette::PickWeightedItem()` 쪽에 조회 로직을 추가하는 별도
   작업이 필요하다
