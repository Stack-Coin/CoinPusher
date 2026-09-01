# Roulette 관련 C++

## 개요

원형 돌림판 형태의 룰렛 기능. `Roll()`을 호출하면 8칸 중 하나가 균등한 확률로 당첨되고,
화면 중앙 위쪽에서 아래로 등장하는 UI가 그 칸에서 멈추는 연출을 보여준 뒤, 당첨된 칸에
설정된 아이템을 스폰한다. `ACPCoinPusher`와 마찬가지로 C++ 클래스는 `UCLASS(abstract)` /
`UCLASS(abstract)` 로 선언되어 있으며, 실제 메시/UMG 비주얼은 이를 상속한 Blueprint에서 채워 넣는다.

## Class 구조

- `ACPRoulette` : 룰렛 로직을 갖는 Actor
    - `SpawnPoint` (`USceneComponent`, RootComponent) : 당첨된 아이템이 스폰되는 위치/방향
    - `Slots` (`TArray<FCPRouletteSlotData>`, 8개 고정) : 각 칸의 아이템 ID / 스폰 클래스 / 스폰 개수
    - `RouletteWidgetClass` (`TSubclassOf<UCPRouletteWidget>`) : `Roll()` 시 화면에 띄울 룰렛 UI
    - `Roll()` : 8칸 중 하나를 균등 확률로 뽑고, UI에 결과를 보여준 뒤 해당 칸의 아이템을 스폰
- `UCPRouletteWidget` : 룰렛 UI (UMG). `PlaySpin(ResultIndex, NumSlots)`가 호출되면
  화면 중앙 위쪽에서 아래로 슬라이드하며 등장한 뒤, `WheelImage`를 여러 바퀴 돌려 `ResultIndex`번째
  칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다. 칸이 결정되면 `OnResultDetermined`를
  브로드캐스트하고, `PostResultHideDelay`(기본 2초) 후 스스로 사라진다
- `FCPRouletteSlotData` : 룰렛 한 칸의 데이터. `ItemID`(FName), `ItemClass`(`ICPCoinPusherItem`을
  구현하는 Actor 클래스), `SpawnCount`(int32)

## 클래스별 상세

### FCPRouletteSlotData
- `ItemID` : 식별용 아이템 ID
- `ItemClass` : 당첨 시 스폰할 액터 클래스. `ICPCoinPusherItem`을 구현해야 하며(`ACPCoin`, `ACPItem` 등),
  아니면 `ACPRoulette::SpawnSlotItems()`가 아무것도 하지 않음 (`CoinPusher/ACPDispenser`와 동일한 방식)
- `SpawnCount` : 당첨 시 스폰할 개수

### ACPRoulette
- `Slots`는 `meta = (EditFixedSize)`로 선언되어 에디터에서 8개로 고정되고, 각 칸의 값만 편집 가능
- `Roll()` : `Slots.Num()` 범위에서 균등 난수로 `ResultIndex`를 뽑은 뒤, `RouletteWidgetClass`로
  생성한 위젯의 `PlaySpin(ResultIndex, Slots.Num())`을 호출. 위젯이 없으면(클래스 미지정 등)
  UI 없이 바로 `HandleRouletteResultDetermined()`를 호출하는 폴백 동작
- `GetOrCreateRouletteWidget()` : 위젯 인스턴스를 지연 생성해 재사용하고, 최초 생성 시
  `OnResultDetermined`에 `HandleRouletteResultDetermined()`를 바인딩
- `HandleRouletteResultDetermined(ResultIndex)` : 위젯이 스핀을 멈추고 칸을 확정하면 호출되어
  `Slots[ResultIndex]`의 아이템을 스폰
- `SpawnSlotItems()` : `ACPDispenser::DispenseItem()`과 동일한 방식으로 `SpawnPoint` 위치/회전에
  `ItemClass`를 `SpawnCount`번 스폰하고, 스폰된 액터의 RootComponent가 물리 시뮬레이션 중인
  프리미티브라면 `LaunchForwardSpeed`/`LaunchUpwardSpeed`로 발사 속도를 부여

### UCPRouletteWidget
- `WheelImage` (`BindWidgetOptional`) : 8칸이 그려진 회전판 이미지. 위쪽 화살표는 고정된 비주얼
  요소이므로 BP에서 `WheelImage` 위에 배치하기만 하면 되고 별도 C++ 바인딩은 필요 없음
- `PlaySpin(ResultIndex, NumSlots)` : 매 호출마다 위젯을 `EnterStartOffsetY`(화면 위쪽)에서
  다시 등장시키고, 진행 중이던 숨김 타이머를 취소한 뒤 새 스핀을 시작 (연속 호출 시 이전 연출을
  덮어씀 - `Player/CPItemToastWidget`과 동일한 재시작 방식)
- 내부적으로 `Entering`(등장 슬라이드) → `Spinning`(회전) 두 단계를 `NativeTick`에서 `FMath::InterpEaseOut`으로
  보간하며, `Spinning`이 끝나면 `FinishSpin()`이 `OnResultDetermined`를 브로드캐스트하고
  `PostResultHideDelay` 후 `HideRoulette()`으로 스스로 `Collapsed` 처리

## 에디터에서 준비해야 할 것

1. `WBP_CPRoulette`(`UCPRouletteWidget` 상속) 생성: 화면 중앙에 위쪽 화살표 이미지와,
   `WheelImage`라는 이름의 8칸짜리 회전판 `Image`를 배치 (이름이 일치해야 `BindWidgetOptional`이 연결됨)
2. `BP_CPRoulette`(`ACPRoulette` 상속) 생성 후 `RouletteWidgetClass`에 `WBP_CPRoulette` 지정,
   `Slots` 8칸에 각각 `ItemID` / `ItemClass`(`BP_CPCoin`, `BP_CPItem` 등) / `SpawnCount` 입력
3. 필요한 곳(상호작용, 게임 로직 등)에서 `BP_CPRoulette` 인스턴스의 `Roll()`을 호출
