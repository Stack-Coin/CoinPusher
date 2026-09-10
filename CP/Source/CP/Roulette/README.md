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

- `ACPRoulette` : 룰렛 로직을 갖는 Actor. 로컬 스플릿 스크린으로 두 플레이어가 하나의 룰렛 Actor를 공유한다
    - `SpawnPoint` (`USceneComponent`, RootComponent) : 룰렛 액터의 기준 위치/방향
    - `ItemDataTable` (`TObjectPtr<UDataTable>`, `EditAnywhere`) : 룰렛이 아이템을 뽑을 때 사용할
      데이터 테이블 (Row Struct는 `FItemData`여야 함). `bRoulette`가 true인 행만 후보가 되며,
      `RouletteProbability` 가중치로 추첨해 `RouletteSpawnCount`개를 전달한다. 자세한 필드 설명은
      `Datatables/README.md` 참고
    - `RouletteWidgetClass` (`TSubclassOf<UCPRouletteWidget>`) : `Roll()` 시 화면에 띄울 룰렛 UI
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
  아니면(테스트 레벨 등) 티켓 검사를 건너뛴다. 이후 `GetOrCreateRouletteWidgets()`가 반환한
  모든(로컬 스플릿 스크린) 위젯에 대해 `PlaySpin(ResultIndex, CandidateCount)`을 호출해 두 플레이어의
  화면에 동시에 같은 연출이 나오도록 한다. 위젯이 하나도 없으면(클래스 미지정 등) UI 없이 바로
  `HandleRouletteResultDetermined()`를 호출하는 폴백 동작. 성공적으로 스핀을 시작했으면 `true` 반환
- `PickWeightedItem(OutResultIndex, OutCandidateCount)` : `ItemDataTable->GetRowMap()`을 순회해
  `bRoulette`가 true인 행만 모아 `RouletteProbability` 가중치에 비례해 하나를 추첨한다(모든 후보의
  가중치 합이 0 이하면 균등 확률로 대체). 뽑힌 행의 ItemID(=Row Name)/`RouletteSpawnCount`를
  `PendingResultItemID`/`PendingResultSpawnCount`에 저장하고, 위젯 스핀 연출용으로 후보 목록 안에서의
  순번(`OutResultIndex`)과 전체 후보 수(`OutCandidateCount`)를 반환한다. 후보가 하나도 없으면
  (`ItemDataTable` 미지정 포함) `false` 반환
- `GetOrCreateRouletteWidgets()` : 월드의 로컬 `PlayerController`(스플릿 스크린 인원 수만큼)마다
  위젯 인스턴스를 지연 생성해 재사용하고, 각 위젯 생성 시 `OnResultDetermined`에
  `HandleRouletteResultDetermined()`를 바인딩
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

## CoinPusher 연동

`ACPRoulette`는 당첨 결과를 누가 쓰는지 전혀 모르는 순수 pub-sub 구조다. `ACPCoinPusher`가
`LinkedRoulette`(`TObjectPtr<ACPRoulette>`, `EditInstanceOnly`) 프로퍼티로 연동할 룰렛을 직접
참조하고, `BeginPlay()`에서 `LinkedRoulette->OnPickedUp.AddDynamic(this, &ACPCoinPusher::HandleRoulettePickedUp)`
으로 자신의 `HandleRoulettePickedUp(ItemID, SpawnCount)`를 구독한다 — 룰렛에서 아이템이 뽑힐 때마다
CoinPusher가 자신의 `ItemDataTable`에서 그 `ItemID`의 `FItemData::bRouletteToCoinPusher`를 확인해,
true인 경우에만 `ItemSpawn(ItemID, SpawnCount)`으로 천장 Dispenser 중 하나에서 그 아이템을 스폰한다.
자세한 내용은 `CoinPusher/README.md`의 `ACPCoinPusher` 섹션 참고.

## 테스트 (CoinPusher/Test)

- `ACPCoinPusherItemSpawnTestPawn` : R 키를 누르면 `TargetRoulette->Roll()`을 호출 (미지정 시
  `BeginPlay`에서 레벨에 배치된 아무 `ACPRoulette`나 자동으로 찾아 사용, `TargetCoinPusher`와 동일한
  방식). 시작 여부(`bool` 반환값)를 `UE_LOG(LogTemp, Warning, ...)`으로 표시. 당첨 결과 자체는
  `TargetRoulette`의 `LinkedRoulette`로 연결된 `ACPCoinPusher`(있다면)의 `HandleRoulettePickedUp()`으로
  자동 전달되므로 이 Pawn이 직접 결과를 처리하지 않음

## 에디터에서 준비해야 할 것

1. `WBP_CPRoulette`(`UCPRouletteWidget` 상속) 생성: 화면 중앙에 위쪽 화살표 이미지와,
   `WheelImage`라는 이름의 회전판 `Image`를 배치 (이름이 일치해야 `BindWidgetOptional`이 연결됨)
2. `BP_CPRoulette`(`ACPRoulette` 상속) 생성 후 `RouletteWidgetClass`에 `WBP_CPRoulette` 지정,
   `ItemDataTable`에 Row Struct가 `FItemData`인 DataTable 에셋을 연결 (`Datatables/README.md` 참고).
   룰렛에서 뽑히길 원하는 행마다 `bRoulette`를 true로, `RouletteProbability`/`RouletteSpawnCount`를
   원하는 값으로 설정
3. `BP_CPRoulette` 인스턴스를 레벨에 배치한 뒤, 이 결과를 받을 `BP_CPCoinPusher` 인스턴스의
   `LinkedRoulette`에 방금 배치한 `BP_CPRoulette`를 연결 (연결하지 않으면 당첨되어도 아이템이
   스폰되지 않음). 해당 CoinPusher의 천장 Dispenser가 참조하는 `ItemDataTable`에도 룰렛과 동일한
   `ItemID`(RowName)가 등록되어 있어야 실제로 스폰됨
4. 필요한 곳(상호작용, 게임 로직 등)에서 `BP_CPRoulette` 인스턴스의 `Roll()`을 호출
