# Roulette 관련 C++

## 개요

원형 돌림판 형태의 룰렛 기능. `Roll()`을 호출하면 `Slots`에 설정된 칸 중 하나가 각 칸의
`Probability` 가중치에 비례해 당첨되고, 화면 중앙 위쪽에서 아래로 등장하는 UI가 그 칸에서
멈추는 연출을 보여준 뒤, 당첨된 칸의 `RewardTarget`(CoinPusher/GameMode)에 따라 당첨 정보를
전달한다. `ACPCoinPusher`와 마찬가지로 C++ 클래스는 `UCLASS(abstract)`로 선언되어 있으며,
실제 메시/UMG 비주얼은 이를 상속한 Blueprint에서 채워 넣는다.

## Class 구조

- `ACPRoulette` : 룰렛 로직을 갖는 Actor. 로컬 스플릿 스크린으로 두 플레이어가 하나의 룰렛 Actor를 공유한다
    - `SpawnPoint` (`USceneComponent`, RootComponent) : 룰렛 액터의 기준 위치/방향
    - `Slots` (`TArray<FCPRouletteSlotData>`) : 룰렛의 칸. 개수와 각 칸의 내용(확률/아이템 ID/개수/
      당첨 정보를 받을 곳)을 에디터에서 자유롭게 추가/삭제/변경할 수 있다 (칸 개수 고정 아님)
    - `RouletteWidgetClass` (`TSubclassOf<UCPRouletteWidget>`) : `Roll()` 시 화면에 띄울 룰렛 UI
    - `CoinPusher` (`TObjectPtr<ACPCoinPusher>`, `EditInstanceOnly`) : `RewardTarget`이 CoinPusher인
      칸이 당첨됐을 때 실제로 스폰을 맡을 CoinPusher. 레벨에서 수동으로 연결해야 함
      (`InputA`/`InputB`와 동일한 방식)
    - `bIsRolling` : 스핀이 시작되어 결과가 결정될 때까지 true. 한 플레이어가 `Roll()`을 호출해 스핀
      중일 때 다른 플레이어가 `Roll()`을 호출해도 무시되도록 막는 잠금 상태 (`IsRolling()`으로 조회 가능)
    - `Roll()` : 월드의 GameMode가 `ACPGameMode`라면 팀 티켓을 1개 소모해야 하며(부족하면 `false`
      반환), `ACPGameMode`가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다. `Slots`의
      `Probability` 가중치에 따라 칸을 뽑고, 로컬 스플릿 스크린의 모든 플레이어 화면에 동일한 룰렛
      UI를 동시에 재생한 뒤 해당 칸의 당첨 정보를 전달한다
- `UCPRouletteWidget` : 룰렛 UI (UMG). `PlaySpin(ResultIndex, NumSlots)`가 호출되면
  화면 중앙 위쪽에서 아래로 슬라이드하며 등장한 뒤, `WheelImage`를 여러 바퀴 돌려 `ResultIndex`번째
  칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다. 칸이 결정되면 `OnResultDetermined`를
  브로드캐스트하고, `PostResultHideDelay`(기본 2초) 후 스스로 사라진다
- `FCPRouletteSlotData` : 룰렛 한 칸의 데이터. `Probability`(float), `ItemID`(FName),
  `SpawnCount`(int32), `CoinType`(`ECPCoinType`), `RewardTarget`(`ECPRouletteRewardTarget`)
- `ECPRouletteRewardTarget` : 당첨 정보를 받을 곳. `CoinPusher`(실제 스폰) 또는
  `GameMode`(`ICPRouletteRewardReceiver`로 정보만 전달)
- `ICPRouletteRewardReceiver` : `RewardTarget`이 GameMode인 칸이 당첨됐을 때 `ReceiveRouletteReward
  (ItemID, SpawnCount, CoinType)`를 호출받는 인터페이스. `ACPRoulette`은 `GetAuthGameMode()`가 이
  인터페이스를 구현하는지만 확인하므로, 실제 게임의 `ACPGameMode`든 테스트용 GameMode든 이 인터페이스만
  구현하면 룰렛 당첨 정보를 받을 수 있다 (현재는 `ACPCoinPusherItemSpawnTestGameMode`만 구현 중 —
  실제 `ACPGameMode`가 룰렛 보상을 받아야 한다면 별도로 이 인터페이스 구현이 필요함)

## 클래스별 상세

### FCPRouletteSlotData / ECPRouletteRewardTarget
- `Probability` : 당첨 확률 가중치. 모든 칸의 `Probability` 합 대비 이 칸의 비율로 당첨 확률이
  결정되며, 합이 반드시 1일 필요는 없음 (예: 10/20/30/40이면 각각 10%/20%/30%/40%). 모든 칸의 합이
  0 이하면(설정 실수 등) `ACPRoulette::PickWeightedSlotIndex()`가 균등 확률로 대체
- `ItemID` : 식별용 아이템 ID
- `SpawnCount` : 당첨 시 스폰(또는 전달)할 개수
- `CoinType` : `ItemID`가 코인일 때 적용할 코인 타입. 코인이 아닌 아이템이면 무시됨
  (`ACPCoinPusher::ItemSpawn()`이 스폰된 액터가 실제로 `ACPCoin`일 때만 `SetCoinType()`을 호출)
- `RewardTarget` : `CoinPusher`면 `ACPRoulette::CoinPusher->ItemSpawn(ItemID, SpawnCount, CoinType)`
  호출, `GameMode`면 `GetAuthGameMode()`를 `ICPRouletteRewardReceiver`로 캐스팅해
  `ReceiveRouletteReward(ItemID, SpawnCount, CoinType)` 호출 (구현하지 않는 GameMode면 아무 일도
  일어나지 않음)

### ACPRoulette
- `Roll()` : `bIsRolling`이 true면(다른 플레이어가 이미 돌리는 중이면) 아무 동작도 하지 않고
  `false`를 반환한다. 월드의 GameMode가 `ACPGameMode`면 `TrySpendTeamTicket(1)`이 성공해야
  진행되며(실패 시 `false` 반환), `ACPGameMode`가 아니면(테스트 레벨 등) 티켓 검사를 건너뛴다.
  이후 `PickWeightedSlotIndex()`로 뽑은 `ResultIndex`로 `GetOrCreateRouletteWidgets()`가 반환한
  모든(로컬 스플릿 스크린) 위젯에 대해 `PlaySpin(ResultIndex, Slots.Num())`을 호출해 두 플레이어의
  화면에 동시에 같은 연출이 나오도록 한다. 위젯이 하나도 없으면(클래스 미지정 등) UI 없이 바로
  `HandleRouletteResultDetermined()`를 호출하는 폴백 동작. 성공적으로 스핀을 시작했으면 `true` 반환
- `PickWeightedSlotIndex()` : `Slots`의 `Probability` 가중치에 비례해 당첨 칸 인덱스를 뽑는다
  (가중 랜덤 추첨)
- `GetOrCreateRouletteWidgets()` : 월드의 로컬 `PlayerController`(스플릿 스크린 인원 수만큼)마다
  위젯 인스턴스를 지연 생성해 재사용하고, 각 위젯 생성 시 `OnResultDetermined`에
  `HandleRouletteResultDetermined()`를 바인딩
- `HandleRouletteResultDetermined(ResultIndex)` : 스플릿 스크린 위젯마다 각자 스핀을 멈추고 칸을
  확정하면 호출되므로, `bIsRolling`이 false면(이미 처리된 결과면) 무시해 당첨 정보가 중복
  전달되지 않도록 한다. 최초 호출에서 `bIsRolling`을 false로 되돌려 잠금을 풀고
  `DeliverSlotReward(Slots[ResultIndex])`를 호출
- `DeliverSlotReward()` : 당첨된 칸의 `ItemID`/`SpawnCount`/`CoinType`/`RewardTarget`을
  `UE_LOG(LogTemp, Warning, ...)`으로 표시한 뒤, `RewardTarget`에 따라 CoinPusher 스폰 또는
  GameMode 전달 중 하나를 수행 (둘 다 `CoinType`을 함께 넘김)

### UCPRouletteWidget
- `WheelImage` (`BindWidgetOptional`) : 칸이 그려진 회전판 이미지. 위쪽 화살표는 고정된 비주얼
  요소이므로 BP에서 `WheelImage` 위에 배치하기만 하면 되고 별도 C++ 바인딩은 필요 없음
- `PlaySpin(ResultIndex, NumSlots)` : 매 호출마다 위젯을 `EnterStartOffsetY`(화면 위쪽)에서
  다시 등장시키고, 진행 중이던 숨김 타이머를 취소한 뒤 새 스핀을 시작 (연속 호출 시 이전 연출을
  덮어씀 - `Player/CPItemToastWidget`과 동일한 재시작 방식)
- 내부적으로 `Entering`(등장 슬라이드) → `Spinning`(회전) 두 단계를 `NativeTick`에서 `FMath::InterpEaseOut`으로
  보간하며, `Spinning`이 끝나면 `FinishSpin()`이 `OnResultDetermined`를 브로드캐스트하고
  `PostResultHideDelay` 후 `HideRoulette()`으로 스스로 `Collapsed` 처리

## 테스트 (CoinPusher/Test)

- `ACPCoinPusherItemSpawnTestPawn` : R 키를 누르면 `TargetRoulette->Roll()`을 호출 (미지정 시
  `BeginPlay`에서 레벨에 배치된 아무 `ACPRoulette`나 자동으로 찾아 사용, `TargetCoinPusher`와 동일한
  방식). 시작 여부(`bool` 반환값)를 `UE_LOG(LogTemp, Warning, ...)`으로 표시
- `ACPCoinPusherItemSpawnTestGameMode` : `ICPRouletteRewardReceiver`를 직접 구현(무거운
  `ACPGameMode`를 상속하지 않고, 다른 Test GameMode들처럼 최소 구성 유지)해 `ReceiveRouletteReward()`가
  호출되면 `UE_LOG(LogTemp, Warning, ...)`으로 수신 여부를 확인할 수 있다. `Slots` 중 `RewardTarget`이
  GameMode인 칸을 당첨시켜(확률을 100으로 설정하는 등) R 키로 확인 가능

## 에디터에서 준비해야 할 것

1. `WBP_CPRoulette`(`UCPRouletteWidget` 상속) 생성: 화면 중앙에 위쪽 화살표 이미지와,
   `WheelImage`라는 이름의 회전판 `Image`를 배치 (이름이 일치해야 `BindWidgetOptional`이 연결됨)
2. `BP_CPRoulette`(`ACPRoulette` 상속) 생성 후 `RouletteWidgetClass`에 `WBP_CPRoulette` 지정,
   `Slots`에 원하는 개수만큼 칸을 추가해 각각 `Probability` / `ItemID` / `SpawnCount` / `CoinType` /
   `RewardTarget` 입력 (`ItemID`가 코인이 아니면 `CoinType`은 무시되므로 기본값 `Normal` 그대로 둬도 됨)
3. `RewardTarget`이 CoinPusher인 칸이 있다면, 레벨에 배치한 `BP_CPRoulette` 인스턴스의
   `CoinPusher`에 같은 레벨의 `BP_CPCoinPusher` 인스턴스를 연결 (연결하지 않으면 당첨되어도
   아이템이 스폰되지 않음). 해당 CoinPusher의 천장 Dispenser가 참조하는 `UCPItemRegistry`에 룰렛
   `Slots`와 동일한 `ItemID`가 등록되어 있어야 실제로 스폰됨
4. `RewardTarget`이 GameMode인 칸이 있다면, 레벨의 GameMode가 `ICPRouletteRewardReceiver`를
   구현하는지 확인 — 실제 `ACPGameMode`는 기본적으로 구현하지 않으므로(테스트용 GameMode만 구현),
   실제 게임에서 GameMode가 룰렛 보상을 받아야 한다면 `ACPGameMode`(또는 그 BP)에 별도로 구현 필요
5. 필요한 곳(상호작용, 게임 로직 등)에서 `BP_CPRoulette` 인스턴스의 `Roll()`을 호출
