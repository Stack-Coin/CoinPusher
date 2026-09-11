# Datatables 관련 C++

## 개요

아이템 마스터 데이터(ID/분류/이름/스폰용 클래스 등)를 DataTable로 관리하기 위한 최소 구성.
예전에 `ACPDispenser`가 참조하던 `UCPItemRegistry`(`UDataAsset`, ItemID → 스폰 클래스 맵)를
대체한다 — DataTable 쪽이 에디터에서 표 형태로 한눈에 보고 편집하기 편하기 때문.

## Class 구조

- `FItemData` : DataTable의 Row Struct (`FTableRowBase` 상속). 한 행 = 아이템(코인 포함) 하나
- `UCPItemDataTableGameInstance` : `ItemDataTable`을 들고 있는 `UGameInstance` — 레벨을 넘어서도
  전역적으로 아이템 마스터 데이터에 접근하고 싶을 때 사용 (현재 `ACPDispenser`는 이 GameInstance를
  거치지 않고 자기 자신의 `ItemDataTable` 참조를 직접 들고 있음 — 아래 "ACPDispenser 연동" 참고)

## 클래스별 상세

### FItemData
- `ID`(FName) : 식별용 아이템 ID. DataTable의 **Row Name과 동일한 값으로 등록**해야 `FindRow<FItemData>(ID, ...)`
  로 조회 가능 (실제 조회는 Row Name 기준이며 `ID` 필드 자체는 참조용/가독성용)
- `Category`(FName) : 아이템 대분류 (예: `Coin`, `Weapon`, `Item` 등 - 프로젝트에서 자유롭게 정의).
  **`"Coin"`이면 특별 취급됨** — `ACPDispenser`가 스폰 직후 `CoinType`을 적용
- `Type`(FName) : `Category` 안에서의 세부 종류 (예: `Category`가 `Weapon`이면 `Sword` 등). 순수 데이터용, 코드에서 분기하지 않음
- `Name`(FText) : 화면에 표시할 이름
- `CoinType`(`ECPCoinType`) : `Category`가 `"Coin"`일 때 적용할 코인 타입 (Normal/Passive/Big/HP/Monster/CoinTower).
  코인이 아니면 무시됨
- `CoinPusherSpawnBPClass`(`TSubclassOf<AActor>`) : `ACPDispenser::DispenseItemByID()`가 스폰할 때 쓰는
  액터 클래스. `ICPCoinPusherItem`을 구현해야 함(코인/DropZone에서 처리되는 아이템)
- `WorldSpawnBPClass`(`TSubclassOf<AActor>`) : 현재 코드에서 소비하는 곳은 없는 예비 필드(과거
  WorldSpawn 기능이 쓰던 필드). DataTable 에셋에 이미 등록된 데이터를 보존하기 위해 필드 자체는
  남겨둠
- `bRoulette`(bool, 기본값 false) : true면 `ACPRoulette`의 추첨 후보가 되는 행
- `RouletteProbability`(float, 기본값 0.0f) : `bRoulette`가 true인 행들의 `RouletteProbability` 합
  (TotalProbability) 대비 이 행의 비율로 룰렛 당첨 확률이 결정됨 (자세한 내용은 `Roulette/README.md` 참고)
- `RouletteSpawnCount`(int32, 기본값 1) : 이 행이 룰렛에 당첨됐을 때 전달할 개수

### UCPItemDataTableGameInstance
- `ItemDataTable`(`TObjectPtr<UDataTable>`, EditDefaultsOnly) + `GetItemDataTable()` getter
- `UCLASS(abstract)` — BP 자식을 만들어 `ItemDataTable`에 실제 DataTable 에셋을 지정한 뒤, Project
  Settings > Maps & Modes > Game Instance Class에 그 BP를 지정해야 사용 가능

## ACPDispenser 연동 (`Source/CP/CoinPusher/CPDispenser.h`)

`ACPDispenser`는 `ItemDataTable`(`TObjectPtr<UDataTable>`, EditAnywhere) 프로퍼티를 직접 들고 있고,
`DispenseItemByID(ItemID, SpawnCount, bLaunch)` / `DispenseCoinByID(ItemID, bLaunch)`가
`FindItemData(ItemID)`(private, `ItemDataTable->FindRow<FItemData>(...)`)로 행을 찾아 스폰한다.
스폰 시 `SpawnFromItemData()`가 `Row.Category == "Coin"`이면 스폰된 액터가 실제로 `ACPCoin`일 때만
`SetCoinType(Row.CoinType)`을 호출해준다. 여러 Dispenser(천장 Dispenser 등)가 같은 DataTable 에셋을
공유해서 지정할 수 있다. 자세한 내용은 `CoinPusher/README.md`의 `ACPDispenser` 섹션 참고.

## 에디터에서 준비해야 할 것

1. **DataTable 에셋 생성**: Content 브라우저 우클릭 → Miscellaneous → Data Table → Row Structure로
   `Item Data`(`FItemData`) 선택. 행마다 `ID`/`Category`/`Type`/`Name`/`CoinType`/
   `CoinPusherSpawnBPClass`를 입력하고, **Row Name을 `ID`와 동일한 값으로** 지정
   (조회는 Row Name 기준). 룰렛에서 뽑히길 원하는 행이면 `bRoulette`를 true로, `RouletteProbability`/
   `RouletteSpawnCount`도 함께 설정 (`Roulette/README.md` 참고)
2. ItemID로 스폰해야 하는 모든 `ACPDispenser`(천장 Dispenser 등)의 `ItemDataTable`에 1번에서 만든
   DataTable을 연결
3. (선택) 레벨을 넘어 전역적으로 아이템 마스터 데이터를 조회하고 싶으면
   `UCPItemDataTableGameInstance`를 상속하는 BP를 만들어 `ItemDataTable`을 지정하고, Project Settings
   > Maps & Modes > Game Instance Class에 지정
