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
- `FCPPlayerBuffData`(`Source/CP/Datatables/CPPlayerBuffData.h`) : 플레이어 버프 마스터 데이터의
  DataTable Row Struct. 한 행 = 버프 하나, `UI/CPBuffIconWidget.h`의
  `UCPBuffIconWidget::PlayerBuffDataTable`이 참조하는 테이블의 Row Struct로 쓰임 (아래
  "FCPPlayerBuffData" 참고)
- `FCPCutSceneData`(`Source/CP/Datatables/CPCutSceneData.h`) : 컷신 시퀀스 마스터 데이터의 DataTable
  Row Struct. 한 행 = 컷신 진행 중 영상/텍스트 조합 하나, `UI/CPVideoCutSceneUIWidget.h`의
  `UCPVideoCutSceneUIWidget::CutSceneDataTable`이 참조하는 테이블의 Row Struct로 쓰임 (아래
  "FCPCutSceneData" 참고)

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
- `CoinPointText`(FText) : 이 아이템(코인 포함)이 `ACPDropZone`에 떨어질 때 `UI/CPCoinPointUI.h`의
  `UCPCoinPointUI`가 그 자리에 띄울 안내 문구(예: "+1", "다이아!" 등). 비어있으면
  `UCPCoinPointUI::CoinPointDisplayText`(기본 "+1")로 대체됨 (`Roulette 연동` 등과 무관하게 항상
  `ACPCoinPusher::GetItemDataTable()`에서 조회 - `CoinPusher/README.md`/`UI/README.md` 참고)
- `InventoryIcon`(`TObjectPtr<UTexture2D>`) : 인벤토리 슬롯에 표시할 아이콘 이미지
- `ItemMesh`(`TObjectPtr<UStaticMesh>`) : `ACPItem`이 3D 메시로 표시될 때 쓰는 스태틱 메시.
  `ACPItem::ApplyItemData()`가 `ItemId`로 이 행을 찾아 자신의 `Mesh` 컴포넌트에 적용함(비어있으면
  `Mesh`에 원래 지정된 스태틱 메시를 그대로 둠) - `CoinPusher/README.md`의 "ACPItem" 참고
- `ItemMaterial`(`TObjectPtr<UMaterialInterface>`) : `ItemMesh`(또는 `Mesh`에 이미 지정된 메시)의
  슬롯 0에 적용할 머티리얼. 비어있으면 원래 지정된 머티리얼을 그대로 둠
- `ItemMaterial2`(`TObjectPtr<UMaterialInterface>`) : `ItemMesh`(또는 `Mesh`에 이미 지정된 메시)의
  슬롯 1에 적용할 머티리얼. 비어있으면 원래 지정된 머티리얼을 그대로 둠
- `ItemImage`(`TObjectPtr<UTexture2D>`) : `ACPItem`이 3D 메시 대신 평면(Billboard, `ImageMesh`)으로
  표시될 때 쓰는 이미지. `ACPItem::BillboardMaterial`로부터 만든 Dynamic Material Instance의
  `ItemTextureParameterName`(기본 `"ItemTexture"`) 텍스처 파라미터에 이 값이 들어감. 비어있으면
  `ImageMesh`를 숨김

### FCPPlayerBuffData
- `BuffID`(FName) : 식별용 버프 ID. DataTable의 **Row Name과 동일한 값으로 등록**해야
  `FindRow<FCPPlayerBuffData>(BuffID, ...)`로 조회 가능 (실제 조회는 Row Name 기준이며 `BuffID`
  필드 자체는 참조용/가독성용)
- `BuffImage`(`TObjectPtr<UTexture2D>`) : 이 버프의 아이콘으로 표시할 이미지.
  `UCPBuffIconWidget::SetBuffCode(BuffCode)`가 `BuffCode`(=Row Name)로 이 행을 찾아
  `BackgroundImage`에 적용함 - `UI/README.md`의 "버프 아이콘" 참고

### FCPCutSceneData
- `CutSceneID`(FName) : 식별용 컷신 ID (참조용/가독성용 - 실제 재생 순서는 아래 `Sequence_Index` 기준이라
  Row Name과 맞출 필요는 없음)
- `Video`(`TObjectPtr<UFileMediaSource>`) : 이 시퀀스에서 재생할 영상. 비어있으면(nullptr) 직전
  시퀀스에서 재생 중이던 영상을 그대로 이어서 보여줌
- `TextFont`(`FSlateFontInfo`) : 이 시퀀스의 `Text`에 적용할 폰트. 유효한 폰트가 지정되지 않았으면
  (`HasValidFont()`가 false) 직전 시퀀스에서 쓰던 폰트를 그대로 유지
- `FontSize`(float) : 이 시퀀스의 `Text`에 적용할 폰트 크기. `TextFont`(폰트 자체)는 그대로 두고
  크기만 바꾸고 싶을 때 사용 - 0 이하면 "지정 안 함"으로 취급해 직전 시퀀스에서 쓰던 크기를 그대로 유지
- `bItalic`(bool) : 이 시퀀스의 `Text`를 이탤릭(기울임)으로 표시할지 여부. bool이라 "지정 안 함"
  상태가 없어 다른 필드와 달리 이전 값을 유지하지 않고 매 시퀀스마다 그 값 그대로(true/false) 적용됨
  - `UI/CPVideoCutSceneUIWidget.h`의 `ItalicSkewAmount`(EditAnywhere)가 실제 기울기 강도를 결정
- `Text`(FText) : 이 시퀀스에서 보여줄 대사/설명 텍스트. 비어있으면 직전 시퀀스의 텍스트를 그대로 유지.
  줄바꿈하고 싶은 위치에 `\n`을 입력하면 `UI/CPVideoCutSceneUIWidget.h`의 `UCPVideoCutSceneUIWidget`이
  실제 개행 문자로 바꿔서 적용해줌
- `ScriptBoxImage`(`TObjectPtr<UTexture2D>`) : 대사/설명 텍스트 뒤에 깔리는 스크립트창 배경 이미지.
  비어있으면(nullptr) 직전 시퀀스에서 쓰던 스크립트창 이미지를 그대로 유지
- `Sequence_Index`(int32) : 재생 순서. `UI/CPVideoCutSceneUIWidget.h`의 `UCPVideoCutSceneUIWidget`이
  DataTable의 모든 행을 이 값 오름차순으로 정렬해 순서대로 재생함(Row Name이나 DataTable 등록 순서와
  무관) - `UI/README.md`의 "영상 컷신" 참고

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
   `RouletteSpawnCount`도 함께 설정 (`Roulette/README.md` 참고). `ACPItem`으로 스폰되는 행이면
   3D 메시로 표시할지(`ItemMesh`/`ItemMaterial`/`ItemMaterial2`) 평면 이미지로 표시할지(`ItemImage`)에 맞춰 해당
   필드도 채움 (`CoinPusher/README.md`의 "ACPItem" 참고)
2. ItemID로 스폰해야 하는 모든 `ACPDispenser`(천장 Dispenser 등)와 `ACPItem`(BP 인스턴스)의
   `ItemDataTable`에 1번에서 만든 DataTable을 연결. `ItemImage`를 쓰는 `ACPItem`이라면
   `BillboardMaterial`에 텍스처 파라미터(기본 이름 `ItemTexture`)를 갖는 머티리얼도 지정해야
   `ImageMesh`가 실제로 그 이미지를 보여줌 - 이때 `Mesh`에 지정된 StaticMesh 에셋에 `ImagePoint`
   라는 이름의 소켓이 정의돼 있어야 `ImageMesh`(사각 플레인)가 그 위치에 붙는다(스태틱 메시
   에디터의 Socket Manager에서 추가). 소켓이 없으면 `ImageMesh`는 `Mesh`의 원점에 그대로 붙음
3. (선택) 레벨을 넘어 전역적으로 아이템 마스터 데이터를 조회하고 싶으면
   `UCPItemDataTableGameInstance`를 상속하는 BP를 만들어 `ItemDataTable`을 지정하고, Project Settings
   > Maps & Modes > Game Instance Class에 지정
4. 플레이어 버프 아이콘을 쓰려면 Row Structure로 `Cp Player Buff Data`(`FCPPlayerBuffData`)를
   선택해 별도 DataTable 에셋을 만들고, 행마다 `BuffID`(=Row Name과 동일하게)/`BuffImage`를 등록한
   뒤, `UCPBuffIconWidget`을 상속하는 WBP의 `PlayerBuffDataTable`에 그 DataTable을 연결
   (`UI/README.md`의 "버프 아이콘" 참고)
5. 영상 컷신을 쓰려면 Row Structure로 `Cp Cut Scene Data`(`FCPCutSceneData`)를 선택해 별도 DataTable
   에셋을 만들고, 행마다 `Sequence_Index`(재생 순서)와 그 시퀀스에서 바뀌어야 할 `Video`/`TextFont`/
   `FontSize`/`Text`/`ScriptBoxImage`만 채움(안 바뀌는 필드는 비워두면 직전 값 유지 - `FontSize`는
   0 이하가 "안 바뀜"). `bItalic`은 다른 필드와 달리 유지되지 않으므로 이탤릭을 켠 행 다음에
   끄고 싶은 행이 있으면 반드시 false로 명시. Video는 File Media Source 에셋(재생할
   영상 파일을 Content 브라우저로 임포트하면 자동 생성됨)을 지정. `UCPVideoCutSceneUIWidget`을
   상속하는 WBP의 `CutSceneDataTable`에 그 DataTable을 연결하고, `MediaPlayer`에는 Content 브라우저
   에서 미리 만들어둔 Media Player 에셋을 지정 (`UI/README.md`의 "영상 컷신" 참고). 실제 영상 디코딩을
   위해 Project Settings > Plugins에서 Electra Player가 켜져 있어야 함(`CP.uproject`에 이미 등록)
