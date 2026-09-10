# CoinPusher 관련 C++

##개요

CoinPusher 기계를 구성하는 Actor들의 C++ 구현. 모든 클래스는 `UCLASS(abstract)`로 선언되어 있으며,
실제 메시/이펙트 등 콘텐츠 참조는 이 클래스들을 상속한 Blueprint에서 채워 넣는 방식(프로젝트 기존 관례)을 따른다.

## Class 구조

- `ACPCoinPusher`
    - `Floor` (`UBoxComponent`, RootComponent) : 코인이 놓이는 바닥이자 실제 충돌의 기준이 되는 루트
    - `Body` : 몸체 StaticMeshComponent (`Floor`에 부착, 콜리전 없음 — 순수 비주얼)
    - `LeftWall` / `RightWall` / `BackWall` / `FrontWall` (`UBoxComponent`) : `Floor`와 함께 실제 충돌을 담당하는 박스 콜리전. 코인을 기계 안에 물리적으로 가둠 (`FrontWall`은 게임 시작 후 `FrontWallRemovalDelay`초 뒤 자동으로 콜리전/비주얼이 꺼짐)
    - `ExtraBoxMesh` (`UStaticMeshComponent`, `Floor`에 부착, 콜리전 없음) : 추가 비주얼 메시. 구체적인 용도는 아직 정해지지 않음 — BP에서 자유롭게 확장
    - `ViewCaptureBoom` (`USpringArmComponent`, `Floor`에 부착) + `ViewCaptureComponent` (`UCPCoinPusherViewCaptureComponent`, `SceneCaptureComponent2D` 서브클래스, Boom 소켓에 부착) : 이 CoinPusher를 비추는 카메라. 화면 Picture-in-Picture 표시에 쓰임 (아래 "화면 캡처(PIP) 시스템" 참고)
    - `PusherComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — `ACPPusher`를 소유
    - `DispenserComponentA` / `DispenserComponentB` (`UChildActorComponent`, 2개) : **컴포넌트를 통한 Has-a** — Input과 연동되어 앞으로 코인을 던지는 `ACPDispenser`를 소유
    - `CeilingDispenserComponents` (`UChildActorComponent`, 5개) : **컴포넌트를 통한 Has-a** — 천장에서 물건을 뿌리는 `ACPDispenser`를 소유. 게임 시작 시 각각 코인을 드롭
    - `DropZoneComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — `ACPDropZone`을 소유
    - `PassiveCoinConvertAreaComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — 영역 안 코인을 Passive/HP로 전환시키는 `ACPPassiveCoinConvertArea`를 소유. `GetPassiveCoinConvertArea()`로 실제 스폰된 인스턴스 접근
    - `MonsterCoinConvertAreaComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — 위와 별개의 `ACPPassiveCoinConvertArea` 인스턴스를 소유, 영역 안 Normal 코인을 Monster로 전환하는 전용 트리거. `GetMonsterCoinConvertArea()`로 실제 스폰된 인스턴스 접근
    - `CoinThrowAreaComponents` (`UChildActorComponent`, 5개) : **컴포넌트를 통한 Has-a** — 코인을 날려보내는 `ACPCoinThrowArea` 5개를 소유. `GetCoinThrowArea(Index)`로 실제 스폰된 인스턴스 접근, `ActiveWaveThrow()`가 이 5개를 순차적으로 활성화
    - `CoinTowerSpawnerComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — 원형 코인 타워를 스폰/상승시키는 `ACPCoinTowerSpawner`를 소유. `GetCoinTowerSpawner()`로 실제 스폰된 인스턴스 접근. `PostInitializeComponents()`가 같은 CoinPusher의 `GetPusher()`(=`PusherComponent`가 스폰한 `ACPPusher`)를 `SetTargetPusher()`로 자동 연결해줌
    - 체력(Health) 보유, 적(Enemy 태그)과 접촉 시 피해를 입음
- `ACPPusher` : 앞뒤로 왕복 운동하며 코인을 밀어내는 Actor
- `ACPDispenser` : 설정된 `ICPCoinPusherItem` 오브젝트(코인, 아이템 등)를 생성해 앞으로 던지는 Actor. **Has-a** `ACPInput`
- `ACPInput` : 플레이어와 상호작용 가능한 Actor (`ICPInteractable` 구현)
- `ACPDropZone` : `ICPCoinPusherItem`이 떨어지면 수거하는 트리거 Actor
- `ACPCoin` : 물리 시뮬레이션을 받는 코인 Actor. `ICPCoinPusherItem` 구현 — DropZone에 떨어지면 코인 개수 증가
- `ACPPassiveCoinConvertArea` : 겹친 Normal 코인 중 Num개를 랜덤하게 골라 Passive/HP/Monster로 전환시키는 트리거 볼륨 (아래 "ACPPassiveCoinConvertArea" 참고)
- `ACPCoinThrowArea` : 겹쳐 있는 모든 타입의 코인을 월드 X(앞)/Z(위) 방향으로 날려보내는 트리거 볼륨 (아래 "ACPCoinThrowArea" 참고)
- `ACPCoinTowerSpawner` : `SpawnTower(N)`으로 원형 코인 타워를 스폰하고 목표 지점까지 상승시키는 연출용 Actor (아래 "ACPCoinTowerSpawner" 참고)
- `ACPItem` : 물리 시뮬레이션을 받는 프라이즈/아이템 Actor. `ICPCoinPusherItem` 구현 — DropZone에 떨어지면 `ItemCode` 기록
- `ICPCoinPusherItem` : Dispenser가 생성하고 DropZone이 수거할 수 있는 오브젝트를 위한 인터페이스 (`OnDroppedInZone(ACPDropZone*)`)
- `ICPDroppedItemReceiver` : `ACPDropZone`에 떨어진 아이템(코인 포함) 정보를 GameMode로 전달하기 위한 인터페이스 (`ReceiveDroppedItem(ItemID, Count, CoinType)`)
- `ICPInteractable` : 상호작용 인터페이스 (`Interact(AActor* Interactor)`)
- `FItemData`(`Source/CP/Datatables/CPItemData.h`) : 아이템 마스터 데이터 DataTable의 Row Struct.
  `ACPDispenser::ItemDataTable`(`UDataTable*`)에 Row Struct가 `FItemData`인 DataTable 에셋을 지정해
  사용 — 예전의 `UCPItemRegistry`(`UDataAsset`) 데이터 에셋을 대체함 (자세한 내용은 `Datatables/README.md` 참고)

## 클래스별 상세

### ICPCoinPusherItem
- `OnDroppedInZone(ACPDropZone* DropZone)` 하나만 가진 인터페이스. `ACPDropZone`이 무엇이 떨어졌는지 구체 타입을 몰라도 처리할 수 있게 해준다 (디커플링)
- 구현체가 스스로 `DropZone`의 공개 API(`AddCollectedCoins`, `RecordCollectedItem`)를 호출해서 자신이 어떤 종류인지 알림 → 파괴/정리까지 스스로 담당

### ACPCoin
- `Mesh`(StaticMeshComponent, RootComponent)에서 물리 시뮬레이션(SimulatePhysics)을 켜서 중력/충돌의 영향을 받음
- `CoinThrowAreaComponent`(`UChildActorComponent`, `Mesh`에 부착) : **컴포넌트를 통한 Has-a** — 스케일 연출이 최대 크기에 도달했을 때 활성화할 `ACPCoinThrowArea`를 소유. 실제 사용할 BP 서브클래스는 이 컴포넌트의 Child Actor Class에 지정(`BP_CPCoinThrowArea` 등). `GetCoinThrowArea()`로 실제 스폰된 인스턴스 접근
- `ItemID`(FName, EditAnywhere) : `ItemDataTable`(`FItemData`)/Dispenser가 쓰는 ItemID와 동일한 개념의 식별자. `OnDroppedInZone()`이 자신의 `ItemID`/`CoinType`을 `AddCollectedCoins(1, ItemID, CoinType)`에 실어서 호출함
- `ICPCoinPusherItem` 구현
- `Launch(LaunchVelocity)` : `Mesh`에 물리 속도를 부여해 날림 (`ACPCoinThrowArea::ActiveThrow()`가 호출). 이미 `bIsLaunched`가 true(=날아가고 있는 중)면 아무것도 하지 않고 무시 — 여러 CoinThrowArea가 짧은 시간 안에 같은 코인을 중복으로 발사해 속도가 비정상적으로 누적되는 것을 방지. `LaunchCooldown`(EditAnywhere, 기본 1초) 경과 후 자동으로 `bIsLaunched`가 다시 false로 돌아와 재발사 가능해짐 (참고: `ACPDispenser::SpawnItemClass()`의 초기 스폰 발사는 이 함수를 거치지 않고 RootComponent에 직접 속도를 부여하므로 이 쿨다운의 영향을 받지 않음)
- `Collect()` : `BP_OnCollected` 이벤트 후 자신을 Destroy
- `OnDroppedInZone(DropZone)` : `DropZone->AddCollectedCoins(1)` 호출 후 `Collect()`
- `CoinType`(`ECPCoinType`: Normal/Passive/Big/HP/Monster) : `SetCoinType(NewType)`으로 변경. 실제로 값이 바뀔 때만 아래 동작들을 트리거하고 `BP_OnCoinTypeChanged(OldType, NewType)` BP 이벤트를 호출
  - Passive/HP/Monster로 전환 시 동일한 스케일 연출 재생(공용 로직): 원본 스케일 → `ScaleAnimMinScale`로 축소 → `ScaleAnimMaxScale`로 확대 → 원본 스케일로 복귀 (속도는 `ScaleAnimSpeed`, 세 값 모두 EditAnywhere, `Coin|ScaleAnim` 카테고리). 다른 타입으로 다시 바뀌면 진행 중이던 연출은 즉시 취소되고 원본 스케일로 복귀
  - 최대 크기(`ScaleAnimMaxScale`)에 도달하는 순간 `CoinThrowAreaComponent`가 스폰한 `ACPCoinThrowArea`(`GetCoinThrowArea()`)를 이 코인 위치 기준 월드 X방향으로 `CoinThrowAreaOffsetX`만큼 떨어뜨려 옮기고 `ActiveThrow()`를 호출 — "커졌을 때 주변 코인을 날려보내는" 연출. **Monster로 전환된 경우는 예외** — 스케일 연출 자체는 Passive/HP와 동일하게 재생되지만 최대 크기에 도달해도 `ActivateCoinThrowArea()`를 호출하지 않음(`SetCoinType`에서 분기)
  - Big으로 전환 시에는 애니메이션 없이 즉시 `GetActorScale3D() * BigScaleMultiplier`(EditAnywhere, `Coin|Big` 카테고리, 기본 3배)로 스케일을 키우고 원상복구되지 않음
  - `CoinTypeVisuals`(`TMap<ECPCoinType, FCPCoinTypeVisual>`, EditAnywhere) : 타입별로 지정한 `Mesh`/`Material`/`PhysicsMaterial`이 있으면 전환 시점에 `Mesh` 컴포넌트에 각각 `SetStaticMesh()`/`SetMaterial(0, ...)`/`SetPhysMaterialOverride(...)`로 적용. 비워둔(nullptr) 필드나 맵에 없는 타입은 바꾸지 않음 — 예: Monster 전용 메시/머티리얼/물리 머티리얼(마찰·반발 등)을 쓰고 싶으면 `CoinTypeVisuals`에 `Monster` 항목만 채워두면 됨 (이 매핑은 CoinType과 무관하게 이미 범용으로 동작하므로, 새 타입을 추가할 때 이 함수 자체는 손댈 필요가 없었음)
- `OwningCoinPusher`(`TObjectPtr<ACPCoinPusher>`) : 이 코인을 스폰한 CoinPusher. `ACPCoinPusher::SpawnBigCoin()`이 스폰 직후 `SetOwningCoinPusher(this)`로 직접 설정해준다 — ChildActorComponent가 스폰한 Dispenser/Coin에는 `Owner`가 채워지지 않아 `GetOwner()` 체인을 타고 올라가는 방식은 쓸 수 없었음(항상 nullptr을 반환하는 버그가 있었음)
- `CoinType == Big`이고 `bBigWaveThrowArmed`가 true(스폰 후 `BigWaveThrowArmDelay`, 기본 0.2초 경과)이며 아직 `bHasTriggeredBigWaveThrow`가 false인 상태에서 `Mesh`가 무엇과든(`OnComponentHit` → `HandleMeshHit`, Floor/Wall뿐 아니라 다른 코인 등 어떤 대상이든) 처음 부딪히면, `OwningCoinPusher->ActiveWaveThrow()`를 호출하고 `bHasTriggeredBigWaveThrow`를 true로 설정해 한 번만 실행되도록 함 — "Big 코인이 스폰되고 어떤 충돌이든 한 번 부딪히면 WaveThrow가 한 번(스폰 0.2초 뒤부터 유효) 터진다" 연출. `BeginPlay()`에서 `BigWaveThrowArmTimerHandle`을 통해 `BigWaveThrowArmDelay` 뒤 `bBigWaveThrowArmed`를 true로 설정 — 스폰 직후 SpawnPoint/Dispenser와의 초기 접촉으로 곧바로 오발동하는 것을 방지
- `SetTowerLocked(bool)` : `ACPCoinTowerSpawner`가 타워를 스폰/상승시키는 동안 코인을 물리적으로 격리할 때 사용. true면 `Mesh->SetSimulatePhysics(false)`로 Kinematic화해서(중력 영향 없음, 스윕 없는 이동에는 Floor/Wall/Pusher 같은 정적/비-Simulate 콜리전에 막히지 않음) `Launch()`/`SetCoinType()`도 조기 반환되어 무시되게 만든다. 콜리전 프로파일 자체(`BlockAllDynamic`)는 그대로 유지되므로 여전히 Simulate 중인 다른 코인과는 밀어내는 물리 상호작용이 발생 — "Coin을 제외하고는 물리충돌을 하지 않는다"가 별도 콜리전 채널 없이 자연스럽게 만족됨. false로 되돌리면 `SimulatePhysics(true)`로 복구되어 중력/물리충돌/`Launch`/`SetCoinType`이 전부 정상으로 돌아옴

### ACPPassiveCoinConvertArea
- `ConvertVolume`(UBoxComponent, RootComponent, Overlap 전용 — `OverlapAllDynamic`) : 코인과 겹쳤는지만 감지, 물리적으로 막지 않음
- `ConvertActive(ItemID, Num)` / `HPConvertActive(ItemID, Num)` / `MonsterConvertActive(ItemID, Num)` : 셋 다 `GatherCandidates()`(private)로 `ConvertVolume`과 겹친 `ACPCoin` 중 `CoinType == Normal`인 것만 후보로 모은 뒤, `ConvertRandomCandidates()`(private, static)로 Num개(가능한 만큼)를 중복 없이 랜덤하게 골라 각각 `SetCoinType(Passive/HP/Monster)`를 호출 — 대상 타입만 다르고 나머지 로직은 동일. `ItemID`는 이 함수 자체는 사용하지 않고(대상 타입이 이미 고정돼 있음), 호출부인 `ACPCoinPusher`의 동명 랩퍼 함수가 CoinType 검증에만 사용
- 같은 클래스의 인스턴스 2개가 `ACPCoinPusher`에 각각 다른 용도로 존재: `PassiveCoinConvertAreaComponent`(레벨 디자인상 Passive/HP 겸용)와 `MonsterCoinConvertAreaComponent`(Monster 전용) — 어느 함수를 호출하느냐로 용도가 갈릴 뿐 클래스 자체는 같음
- 보통 이 클래스를 직접 호출하지 않고 `ACPCoinPusher::ConvertActive()`/`HPConvertActive()`/`MonsterConvertActive()` 랩퍼를 통해서만 호출된다 (아래 `ACPCoinPusher` 섹션 참고)

### ACPCoinThrowArea
- `ThrowVolume`(UBoxComponent, RootComponent, Overlap 전용 — `OverlapAllDynamic`) : 충돌하지 않고 겹침만 감지. 크기는 BP/디테일 패널에서 자유롭게 조정
- `ActiveThrow()` : `ThrowVolume`과 겹쳐 있는 모든 타입의 `ACPCoin`에게 `Launch()`로 속도를 부여해 날림. 위(월드 Z)/앞(월드 X) 방향은 액터 회전과 무관하게 항상 고정
- `MaxUpPower`/`MinUpPower`, `MaxForwardPower`/`MinForwardPower`(모두 EditAnywhere) : `bIsRandomize`가 true면 매 `ActiveThrow()`마다 Min~Max 사이에서 랜덤한 위/앞 힘을 사용, false면 항상 Max 값을 그대로 사용
- 두 곳에서 각자 별도의 `ACPCoinThrowArea` 인스턴스를 **컴포넌트를 통한 Has-a**로 소유함 — 서로 다른 용도이므로 공유하지 않음
  - `ACPCoinPusher`의 `CoinThrowAreaComponents`(ChildActorComponent 5개) : `ActiveWaveThrow()`가 순차적으로 활성화
  - `ACPCoin`의 `CoinThrowAreaComponent`(ChildActorComponent 1개) : 자신의 스케일 연출이 최대 크기에 도달하면 활성화 (위 "ACPCoin" 참고)

### ACPCoinTowerSpawner
- `SpawnerRoot`(USceneComponent, RootComponent) : 고정 루트 — `BackPosition`/`CoinTowerPosition`은 전부 이 액터 기준 상대 위치
- `TowerRoot`(USceneComponent, `SpawnerRoot`에 부착) : 타워를 구성하는 코인들이 부착되는 기준점. 스폰 시점엔 상대 위치 0(=`SpawnerRoot`와 같은 자리)에 있다가, 상승 애니메이션 동안 `CoinTowerPosition`까지 이동 — 이 컴포넌트가 움직이면 부착된 코인들이 한 덩어리로 함께 움직임
- `CoinClass`(`TSubclassOf<ACPCoin>`, EditAnywhere) : `SpawnActor`로 스폰할 Coin 클래스
- `FloorHeight`(기본 50) / `TowerRadius`(기본 60) (모두 EditAnywhere) : 층 사이 수직 간격 / 원형 배치 반지름. 한 층에 배치하는 코인 개수는 `CoinsPerFloor`(고정 5, `CeilingDispenserComponents`처럼 코드에 고정된 상수). 홀수 층은 반 칸(`2π/CoinsPerFloor`의 절반)만큼 회전시켜 배치해서, 바로 아랫층 코인들의 틈 사이사이에 윗층 코인이 놓이도록 함(벽돌쌓기 패턴)
- `CoinTowerPosition`(FVector, EditAnywhere, 기본 `(0,0,300)`) : 타워가 다 스폰된 뒤 상승해서 도달할 목표 지점(이 액터 기준 상대 위치). `UpTime`(기본 2초, EditAnywhere) 동안 `TowerRoot`가 상대 위치 0에서 이 값까지 `FMath::Lerp`로 선형 이동
- `TargetPusher`(`TObjectPtr<ACPPusher>`, VisibleInstanceOnly/BlueprintReadOnly) : 스폰~상승 동안 멈추고 옮겨둘 Pusher. 액터 레퍼런스라 BP에서 직접 할당할 수 없어(`ACPPusher`도 ChildActorComponent로 스폰되는 인스턴스) `SetTargetPusher()`로만 설정 가능 — 같은 CoinPusher가 소유한 경우 `ACPCoinPusher::PostInitializeComponents()`가 자동으로 연결해줌
- `BackPosition`(FVector, EditAnywhere, `TargetPusher` 기준 상대 위치, 기본 `(-200,0,0)`) : 스폰~상승 동안 `TargetPusher`가 이동해갈 목표 위치
- `BackMoveTime`(float, EditAnywhere, 기본 0.5초) : `TargetPusher`가 현재 위치에서 `BackPosition`까지 이동하는 데 걸리는 시간 — 순간이동이 아니라 `Tick`에서 `FMath::Lerp`로 선형 보간되는 애니메이션
- `PusherReturnTime`(float, EditAnywhere, 기본 1초) : 상승이 끝난 뒤 `TargetPusher`가 `BackPosition`에서 원래 위치(뒤로 밀리기 전 위치)까지 되돌아오는 데 걸리는 시간. 이 복귀가 다 끝나야 왕복 운동이 재개됨
- `SpawnTower(FName ItemID, int32 N)` : `ItemID`는 이 함수 자체는 사용하지 않고, 호출부인
  `ACPCoinPusher::SpawnTower()` 랩퍼가 CoinType이 `CoinTower`인지 검증하는 데만 사용한다. 이미 진행
  중인 타워가 있으면(`bIsTowerActive`) 무시. 아니면:
  1. `TowerRoot`를 상대 위치 0으로 리셋
  2. `TargetPusher`가 있으면 `SetPusherPaused(true)`로 멈추고, 현재 위치(`PusherMoveStartLocation`으로 기억)→`BackPosition`(그 시점 `TargetPusher` 기준 상대 위치를 World로 변환한 값) 후퇴 애니메이션을 시작(`bIsMovingPusherBack = true`) — 실제 이동은 `Tick`에서 `BackMoveTime` 동안 진행
  3. `SpawnTowerCoins(N)`으로 N개 층 × `CoinsPerFloor`(5)개를 원형으로 `SpawnActor` 스폰 — 스폰된 각 코인은 `SetTowerLocked(true)`로 잠근 뒤 `TowerRoot`에 `AttachToComponent(KeepWorldTransform)`으로 부착
  4. 다음 `Tick`부터 상승 애니메이션 시작(`bIsRising = true`) — Pusher 후퇴 애니메이션과 동시에 진행됨(서로 독립적)
- `Tick()` : 매 틱 세 애니메이션을 각자 독립적으로 처리
  - `bIsMovingPusherBack`이면 `PusherMoveElapsedTime`을 누적해 `BackMoveTime` 동안 `TargetPusher`를 시작 위치→`BackPosition`으로 보간 이동. 도달하면 `bIsMovingPusherBack = false`
  - `bIsReturningPusher`이면 `PusherReturnElapsedTime`을 누적해 `PusherReturnTime` 동안 `TargetPusher`를 `BackPosition`→`PusherMoveStartLocation`(뒤로 밀리기 전 원래 위치)으로 보간 이동. 도달하면 `bIsReturningPusher = false`로 끄고 `SetPusherPaused(false)`로 왕복 운동을 재개한 뒤, 마지막으로 `bIsTowerActive = false`로 되돌려 다음 `SpawnTower()` 호출을 허용
  - `bIsRising`이면 `RiseElapsedTime`을 누적해 `UpTime` 동안 `TowerRoot`의 상대 위치를 0 → `CoinTowerPosition`으로 보간. 도달하면 `CompleteRise()` 호출
- `CompleteRise()` : 스폰된 모든 코인에 `SetTowerLocked(false)` + `DetachFromActor(FDetachmentTransformRules::KeepWorldTransform)`(독립 액터화) 호출 → `TargetPusher`가 있으면(후퇴 애니메이션이 아직 안 끝났다면 `BackPosition`으로 즉시 완료시킨 뒤) `PusherReturnTime` 동안 원래 위치까지 되돌아오는 복귀 애니메이션을 시작(`bIsReturningPusher = true`) — 왕복 운동 재개와 `bIsTowerActive` 해제는 그 복귀가 다 끝났을 때 `Tick`에서 처리 (즉, 코인 Detach + Pusher가 원래 위치로 복귀해 왕복 운동을 재개하기 전까지는 `SpawnTower()` 재호출이 막힘). `TargetPusher`가 없으면 기다릴 대상이 없으므로 이 단계에서 바로 `bIsTowerActive = false`

### ACPItem
- `CollisionSphere`(USphereComponent, RootComponent, 물리 시뮬레이션) — 구 형태라 자연스럽게 굴러감. `ACPCoin`과 동일하게 Dispenser가 `UPrimitiveComponent` 루트로 인식해 발사 가능
- `Mesh`(StaticMeshComponent, `CollisionSphere`에 부착, 콜리전 없음) — 구 메시를 assign하는 순수 비주얼 파츠
- `ItemCode`(FName, EditAnywhere)로 어떤 아이템인지 식별
- `ICPCoinPusherItem` 구현
- `OnDroppedInZone(DropZone)` : `DropZone->RecordCollectedItem(ItemCode)` 호출 후 `BP_OnCollected` 이벤트 + Destroy

### FItemData
- `Source/CP/Datatables/CPItemData.h`에 정의. `FItemData`는 `FTableRowBase`를 상속하는 DataTable
  Row Struct로, `ID`/`Category`/`Type`/`Name`/`CoinType`/`CoinPusherSpawnBPClass`/`bRoulette`/
  `RouletteProbability`/`RouletteSpawnCount` 필드를 가짐 — 예전에 `ACPDispenser`가 참조하던
  `UCPItemRegistry`(`UDataAsset`)를 대체
- 자세한 필드 설명은 `Datatables/README.md` 참고

### ICPDroppedItemReceiver
- `ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType = Normal)` 하나만 가진 인터페이스.
  `ACPDropZone`에 아이템(코인 포함)이 떨어졌을 때 그 정보를 GameMode로 전달하기 위한 것으로,
  `ACPDropZone`은 `GetAuthGameMode()`가 이 인터페이스를 구현하는지만 확인하고 캐스팅해서 호출하므로
  실제 게임의 GameMode든 테스트용 GameMode든 이 인터페이스만 구현하면 드랍 정보를 받을 수 있다
  (구현 안 하면 그냥 아무 일도 일어나지 않음). `CoinPusher/Test/ACPCoinPusherItemSpawnTestGameMode`가
  구현체 예시 — 받은 정보를 `UE_LOG(LogTemp, Warning, ...)`로 표시해 DropZone이 실제로 보냈는지 확인 가능

### ACPDropZone
- `CollectionVolume`(UBoxComponent)에 `ICPCoinPusherItem`을 구현하는 오브젝트가 겹치면(OnComponentBeginOverlap) 감지해 `Item->OnDroppedInZone(this)` 호출 — Coin/Item 등 구체 타입은 전혀 모름
- `AddCollectedCoins(int32 Amount = 1, FName ItemID = NAME_None, ECPCoinType CoinType = Normal)` :
  `CollectedCoinCount` 증가 + `OnCoinCollected` 브로드캐스트 (`ACPCoin`이 자신의 `ItemID`/`CoinType`을
  실어서 호출). `ExperiencePerCoin * Amount`만큼 팀 경험치를 지급하고, `CollectedCoinCount`가
  `CoinsPerTicket`(기본 10)의 배수가 될 때마다 팀 티켓을 1개 지급. `ItemID`가 비어있지 않으면
  `OnDropped`를 브로드캐스트하고, `GetAuthGameMode()`를 `ICPDroppedItemReceiver`로 캐스팅해
  `ReceiveDroppedItem(ItemID, Amount, CoinType)`도 호출
- `RecordCollectedItem(FName ItemCode)` : `CollectedItemCodes` 배열에 추가 + `OnItemCollected`/`OnDropped` 브로드캐스트 (`ACPItem`이 호출). `ItemRespawnDispenser`가 설정되어 있으면 `DispenseItemByID(ItemCode, 1)`을 호출해 같은 아이템을 다시 생성 요청하고, `GetAuthGameMode()`가 `ICPDroppedItemReceiver`를 구현하면 `ReceiveDroppedItem(ItemCode, 1)`도 호출(코인이 아니므로 `CoinType`은 기본값 Normal)
- `OnDropped`(`FOnCPDropZoneDropped`, `ItemID` 하나만 매개변수) : 코인이든 아이템이든 무언가 떨어질
  때마다(=`AddCollectedCoins`/`RecordCollectedItem`이 호출될 때마다, `ItemID`가 있을 때) 종류 구분
  없이 Broadcast하는 범용 알림용 델리게이트 — `OnCoinCollected`(코인 누적 개수만 전달)/`OnItemCollected`
  (Item 전용)와 달리 "무엇(ItemID)이 떨어졌는지"만 통합해서 알려줌. `ACPCoinPusher::GetDropZoneDroppedDelegate()`
  로 DropZone을 직접 거치지 않고도 참조할 수 있음
- `ItemRespawnDispenser`(`TObjectPtr<ACPDispenser>`, VisibleInstanceOnly) : 위 재생성을 맡을 Dispenser. **DropZone은 `ACPCoinPusher`의 ChildActorComponent로 스폰되는 인스턴스라 레벨에서 직접 편집할 수 없으므로**, 에디터에서 직접 설정하지 않고 `SetItemRespawnDispenser()`를 통해서만 설정됨 (소유자인 `ACPCoinPusher`가 `PostInitializeComponents`에서 호출)
- `ItemRespawnDispenser`(`TObjectPtr<ACPDispenser>`, VisibleInstanceOnly) : 위 재생성을 맡을 Dispenser. **DropZone은 `ACPCoinPusher`의 ChildActorComponent로 스폰되는 인스턴스라 레벨에서 직접 편집할 수 없으므로**, 에디터에서 직접 설정하지 않고 `SetItemRespawnDispenser()`를 통해서만 설정됨 (소유자인 `ACPCoinPusher`가 `PostInitializeComponents`에서 호출)

### ACPPusher
- `PushPlate`(StaticMeshComponent)를 물리 시뮬레이션 없이(Kinematic) `Movable`로 두고 `Tick`에서 위치만 이동
- `StartRelativeLocation` : 왕복 운동의 기준 위치. `BeginPlay` 시점에 `PushPlate`의 상대 위치를 한 번만 기록해두고, 이후로는 절대 건드리지 않음
- `Tick`에서 sine 파형으로 `Alpha`(0~1)를 구하고, 매 틱 `PushPlate->SetRelativeLocation(StartRelativeLocation + FVector(PushDistance * Alpha, 0, 0))`로 **절대 위치**를 다시 계산해서 적용 — `StartRelativeLocation`을 기준으로 0~`PushDistance` 사이를 `CycleSpeed` 속도로 왕복하며 코인을 밀어냄. (이전에는 `AddLocalOffset`으로 매 틱 상대 이동량만 누적하는 방식이었는데, 외부에서 액터가 다른 곳으로 옮겨진 뒤 재개되면 그 옮겨진 위치를 기준으로 계속 오실레이션하게 되는 문제가 있어서 절대 위치 계산 방식으로 바꿈)
- 물리 시뮬레이션 코인(ACPCoin)과는 충돌 블로킹으로 밀어내는 상호작용이 발생
- `SetPusherPaused(bool)` : true면 `Tick`에서 왕복 운동 로직(ElapsedTime 누적 포함)을 완전히 건너뜀. 재개(false) 시 위 절대 위치 계산 덕분에, 액터가 일시정지 중 다른 곳으로 옮겨져 있었더라도 재개하는 순간 자동으로 `StartRelativeLocation` 기준의 올바른 왕복 지점을 찾아 들어가며 이어서 진행됨 — 재개 전에 액터를 원래 위치로 따로 되돌려 놓을 필요가 없음(`ACPCoinTowerSpawner`가 이 패턴으로 사용)

### ACPInput
- `Mesh`를 통해 플레이어의 상호작용 트레이스에 맞을 수 있도록 충돌 설정
- `ICPInteractable::Interact(AActor* Interactor)` 구현 → `OnInteracted` 델리게이트 브로드캐스트 + `BP_OnInteracted` BP 이벤트 호출
- 자신이 직접 Dispenser를 알 필요 없이, 상호작용 사실만 델리게이트로 알림 (디커플링)

### ACPDispenser
- `SpawnPoint`(SceneComponent, Root) + `Body`(StaticMeshComponent, `SpawnPoint`에 부착)로 물건이 튀어나갈 위치/방향 지정 — `SpawnPoint`가 Root라서 이 액터(BP)를 배치/정렬할 때의 기준점이 곧 발사 위치/방향과 일치함
- `ItemClass`(`TSubclassOf<AActor>`, EditAnywhere) : 생성할 오브젝트 클래스. `ICPCoinPusherItem`을 구현해야 하며(`ACPCoin`, `ACPItem` 등), 아니면 `DispenseItem()`이 아무것도 하지 않음 — Dispenser는 자신이 무엇을 생성하는지 몰라도 됨
- `LinkedInput`(`TObjectPtr<ACPInput>`, VisibleInstanceOnly)으로 **Has-a Input** 관계를 표현. Dispenser가 이제 `ACPCoinPusher`의 ChildActorComponent로 스폰되기 때문에 에디터에서 직접 편집하지 않고, `SetLinkedInput()`을 통해서만 설정한다. Input 없이 코드로만 동작하는 천장 Dispenser는 이 값을 비워둠
- `SetLinkedInput(NewLinkedInput)` : 기존 Input의 `OnInteracted` 바인딩을 해제하고 새 Input에 다시 바인딩. 소유자인 `ACPCoinPusher`가 `PostInitializeComponents`에서 호출
- `BeginPlay`에서도 `BindToLinkedInput()`을 호출해 이미 `LinkedInput`이 설정된 경우를 한 번 더 보장 (`AddUniqueDynamic`이라 중복 바인딩되지 않음)
- `DispenseItem()` : `ItemClass`가 `ICPCoinPusherItem`을 구현하는지 확인 후 `SpawnPoint` 위치/회전으로 스폰. 스폰된 액터의 RootComponent가 `UPrimitiveComponent`(물리 시뮬레이션 중)라면 전방+상방 속도(`LaunchForwardSpeed`, `LaunchUpwardSpeed`)를 부여 — `ACPCoin`/`ACPItem` 어느 쪽이든 동일하게 동작하며 Dispenser는 구체 타입을 캐스팅하지 않음
- `DispenseItems(Count)` : `DispenseItem()`을 Count번 반복 호출하는 편의 함수 (천장 Dispenser가 게임 시작 시 여러 개를 한 번에 드롭할 때 사용)
- Input이 상호작용되면 `HandleInputInteracted` → `DispenseItem()`이 호출되어 "Input과 상호작용 시 연결된 Dispenser가 코인을 발사" 요구사항을 만족
- `ItemDataTable`(`TObjectPtr<UDataTable>`, EditAnywhere) : ItemID(RowName) → `FItemData` 행 조회에
  쓰는 데이터 테이블 참조. Row Struct가 `FItemData`인 DataTable이어야 하며, 여러 Dispenser가 같은
  테이블을 공유해서 지정할 수 있음 (예전 `UCPItemRegistry` 에셋 참조를 대체)
- `FindItemData(ItemID)`(protected) : `ItemDataTable->FindRow<FItemData>(ItemID, ...)`로 행을 찾아
  반환 (없으면 nullptr). `DispenseItemByID()`/`DispenseCoinByID()`가 공유하는 조회 로직
- `DispenseItemByID(ItemID, SpawnCount, bLaunch = true)` : `FindItemData(ItemID)`로 찾은 행의
  `CoinPusherSpawnBPClass`가 `ICPCoinPusherItem`을 구현하는지 확인한 뒤 `SpawnCount`개 생성.
  `bLaunch=false`면 발사 속도를 부여하지 않고 그 자리에 둠 — Roulette/DropZone처럼 특정 ItemID를
  지정해서 만들어야 하는 경우에 사용 (`DispenseItem()`/`DispenseItems()`는 여전히 단일 `ItemClass`를
  쓰는 기존 경로)
- `DispenseCoinByID(ItemID, bLaunch = true)` : `DispenseItemByID`와 같은 방식으로 행의
  `CoinPusherSpawnBPClass` 1개를 스폰하지만, 스폰된 액터를 `ACPCoin*`으로 캐스팅해 반환(실패 시
  nullptr)한다는 점이 다름 — 스폰 직후 스폰된 코인에 접근해야 하는 호출부(예:
  `ACPCoinPusher::SpawnBigCoin()`가 `SetCoinType(Big)`을 호출하기 위해 사용)를 위한 함수
- `SpawnFromItemData(Row, ClassToSpawn, bLaunch)`(protected) : `SpawnItemClass()`로 스폰한 뒤, `Row.Category`가
  `"Coin"`이면 스폰된 액터가 실제로 `ACPCoin`일 때만 `SetCoinType(Row.CoinType)`을 호출 —
  `DispenseItemByID()`/`DispenseCoinByID()`가 공유하는 "FItemData 행 기준 스폰" 로직
- `SpawnItemClass(ClassToSpawn, bLaunch)`(protected) : 실제 스폰 + (옵션) 발사를 수행하고 스폰된 액터(실패 시 nullptr)를 반환하는 공용 헬퍼. `DispenseItem()`/`SpawnFromItemData()`가 함께 사용

### ACPCoinPusher
- `Floor`(`UBoxComponent`, RootComponent) : 액터의 루트. `BlockAllDynamic` 프로파일로 실제 충돌 기준이 됨. `BoxExtent`로 직접 크기 지정 (예: `150,150,10`)
- `Body`(StaticMeshComponent, `Floor`에 부착) : `SetCollisionEnabled(NoCollision)`으로 콜리전을 꺼서 순수 비주얼 메시로만 사용
- `LeftWall` / `RightWall` / `BackWall` / `FrontWall`(`UBoxComponent`, `Floor`에 부착) : `Floor`와 함께 전부 `BlockAllDynamic` 프로파일 — 코인(WorldDynamic, 물리 시뮬레이션)을 막아 기계 안에 가두고, Pawn도 막아 실제 벽처럼 동작. 상대 위치/크기는 BP에서 실제 메시에 맞게 조정. `FrontWall`은 `BeginPlay`에서 타이머를 걸어 `FrontWallRemovalDelay`초 후 `RemoveFrontWall()`로 콜리전/비주얼을 꺼서 코인이 앞으로 빠질 수 있게 함
- `ExtraBoxMesh`(`UStaticMeshComponent`, `Floor`에 부착, 콜리전 없음) : 추가 비주얼 메시. 아직 특정 용도는 없고 확장을 위한 자리
- `PusherComponent` / `DispenserComponentA` / `DispenserComponentB` / `CeilingDispenserComponents`(5개) / `DropZoneComponent` / `PassiveCoinConvertAreaComponent` / `MonsterCoinConvertAreaComponent` / `CoinThrowAreaComponents`(5개) / `CoinTowerSpawnerComponent` 모두 `UChildActorComponent`로 각각 `ACPPusher`, `ACPDispenser`, `ACPDropZone`, `ACPPassiveCoinConvertArea`(2개 — 용도별로 별개 인스턴스), `ACPCoinThrowArea`, `ACPCoinTowerSpawner`를 소유 — 전부 Actor이지만 **컴포넌트로 감싸서 Has-a 관계**를 구현 (실제 사용할 BP 서브클래스는 각 컴포넌트의 `Child Actor Class`에 지정). `MonsterCoinConvertAreaComponent`는 `PassiveCoinConvertAreaComponent`와 완전히 별개의 `ACPPassiveCoinConvertArea` 인스턴스로, Normal 코인을 Monster로 전환하는 용도로만 쓰임(`GetMonsterCoinConvertArea()`로 접근)
- `InputA` / `InputB`(`TObjectPtr<ACPInput>`, EditInstanceOnly) : 레벨에 배치한 `ACPInput`을 CoinPusher에서 직접 연결 (앞으로 던지는 `DispenserComponentA`/`B`용)
- `LinkedRoulette`(`TObjectPtr<ACPRoulette>`, EditInstanceOnly) : 이 CoinPusher와 연동할 Roulette.
  레벨에서 직접 연결해야 하며(`InputA`/`InputB`와 동일한 방식의 수동 연결), `BeginPlay()`에서 자동으로
  `LinkedRoulette->OnPickedUp.AddDynamic(this, &ACPCoinPusher::HandleRoulettePickedUp)`으로 구독해 룰렛에서
  아이템이 뽑힐 때마다(`bRouletteToCoinPusher`인 경우에만) 이 CoinPusher의 천장 Dispenser에서 그 아이템이
  나오게 한다 (자세한 내용은 `Roulette/README.md`의 "CoinPusher 연동" 참고)
- `ItemDataTable`(`TObjectPtr<UDataTable>`, EditAnywhere) : ItemID(RowName) → `FItemData` 행 조회에 쓰는
  데이터 테이블 참조. `HandleRoulettePickedUp()`의 `bRouletteToCoinPusher` 조회와 `ValidateItemCoinType()`의
  `CoinType` 검증에 쓰이며, 천장 Dispenser들의 `ItemDataTable`과 같은 에셋을 공유해서 지정하면 됨
- `ItemRespawnDispenser`(`TObjectPtr<ACPDispenser>`, EditInstanceOnly) : DropZone에 아이템이 떨어졌을 때 재생성을 맡을 Dispenser. 레벨에서 이 CoinPusher 인스턴스의 자식 액터로 스폰된 Dispenser 중 하나를(보통 천장 Dispenser) 피커로 선택해서 지정
- `InitialCoinDropCount`(int32, EditAnywhere, 기본값 10) : 게임 시작 시 천장 Dispenser 하나당 드롭할 코인 개수
- `PostInitializeComponents()` : Dispenser 자식 액터가 스폰된 직후(=`BeginPlay` 이전) `GetDispenserA()->SetLinkedInput(InputA)`, `GetDispenserB()->SetLinkedInput(InputB)`를 호출해 Dispenser의 `LinkedInput`을 CoinPusher가 대신 설정하고, `GetDropZone()->SetItemRespawnDispenser(ItemRespawnDispenser)`도 함께 호출. `GetCoinTowerSpawner()->SetTargetPusher(GetPusher())`도 호출해 같은 CoinPusher가 소유한 Pusher를 CoinTowerSpawner에 연결. Dispenser/DropZone/CoinTowerSpawner/Pusher 모두 ChildActorComponent로 스폰되는 인스턴스라(레벨에 직접 배치된 액터가 아니므로) 서로를 액터 레퍼런스 UPROPERTY로 BP에서 직접 할당할 방법이 없어서, 대신 CoinPusher가 스폰 직후 코드로 대신 전달해준다
- `BeginPlay()` : `CeilingDispenserComponents` 5개를 순회하며 각각 스폰된 `ACPDispenser`의 `DispenseItems(InitialCoinDropCount)`를 호출 — "게임 시작 시 5개의 Dispenser에서 코인을 10개씩 드롭" 요구사항을 만족. 실제로 코인이 나오려면 5개 천장 Dispenser BP 인스턴스의 `ItemClass`를 코인 클래스(`BP_CPCoin`)로 지정해야 함
- `GetPusher()` / `GetDispenserA()` / `GetDispenserB()` / `GetCeilingDispenser(Index)` / `GetDropZone()` / `GetPassiveCoinConvertArea()` / `GetMonsterCoinConvertArea()` / `GetCoinTowerSpawner()` : 해당 ChildActorComponent가 실제로 스폰한 액터 인스턴스를 캐스팅해 반환 (`Child Actor Class`가 지정되어야 유효)
- `GetDropZoneDroppedDelegate()` : `GetDropZone()->OnDropped`에 대한 포인터 반환(DropZone이 아직
  스폰되지 않았으면 `nullptr`) — DropZone을 직접 거치지 않고 CoinPusher만으로 바로 바인딩하고 싶은
  C++ 코드를 위한 편의 함수. 델리게이트 타입은 반환값으로 BP에 노출할 수 없어 `BlueprintCallable`이
  아닌 순수 C++ 함수 — BP에서 바인딩하려면 `GetDropZone()`으로 얻은 액터의 `OnDropped` 핀에 직접
  Bind Event를 걸면 됨
- 체력 시스템: `MaxHealth` / `CurrentHealth`, `ApplyDamage(Damage, DamageCauser)`로 감소
- `TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser)` 오버라이드 : 표준 엔진 데미지 경로. `Super::TakeDamage(...)`를 호출해 실제 데미지 값을 구한 뒤 `ApplyDamage(ActualDamage, DamageCauser)`로 위임 — 적(또는 다른 무엇이든)이 `UGameplayStatics::ApplyDamage` / `ApplyPointDamage` / `ApplyRadialDamage`를 호출하면 이 경로를 통해 체력이 깎임. 기존에 있던 오버랩 태그 기반 피격 판정(`OnActorOverlapBegin`, `EnemyActorTag`, `EnemyContactDamage`)은 제거하고 이 방식으로 대체함
- 체력이 0 이하가 되면 `HandleDestroyed()` → `OnCoinPusherDestroyed` 브로드캐스트 + `BP_OnDestroyed` BP 이벤트 호출
- `ValidateItemCoinType(ItemID, ExpectedType)`(private) : `ItemDataTable->FindRow<FItemData>(ItemID, ...)`로
  찾은 행의 `CoinType`이 `ExpectedType`과 일치하는지 검사(행이 없거나 `ItemDataTable`이 비어있으면
  `false`). `SpawnBigCoin()`/`SpawnMonsterCoin()`/`ConvertActive()`/`HPConvertActive()`/
  `MonsterConvertActive()`/`SpawnTower()`가 실제 동작(스폰/전환/타워 스폰) 전에 공통으로 호출하는 검증
  로직 — 예: `SpawnBigCoin()`은 `ItemID`로 조회한 `CoinType`이 `Big`이 아니면 아무것도 하지 않음
- `HandleRoulettePickedUp(ItemID, SpawnCount)` : `LinkedRoulette->OnPickedUp`에 자동으로 등록되는
  핸들러. `ItemDataTable`에서 `ItemID`로 찾은 행의 `bRouletteToCoinPusher`가 `true`인 경우에만
  `ItemSpawn(ItemID, SpawnCount)`을 호출 — 룰렛에서 당첨된 아이템이라도 실제로 CoinPusher에 스폰돼야
  하는지는 데이터 테이블 설정에 따름
- `ItemSpawn(ItemID, SpawnCount)` / `SpawnBigCoin(ItemID, Count = 1)` / `SpawnMonsterCoin(ItemID, Num = 1)` : 셋 다 `PickRandomValidCeilingDispenser()`(private)로 `CeilingDispenserComponents` 5개 중 실제로 스폰된 `ACPDispenser`를 랜덤하게 하나 고른다는 공통 로직을 공유
  - `ItemSpawn(ItemID, SpawnCount)` : 고른 Dispenser의 `DispenseItemByID(ItemID, SpawnCount)`를 호출. 코인 여부/`CoinType` 적용은 Dispenser가 자신의 `ItemDataTable`에서 `FItemData::Category`/`CoinType`을 조회해 내부적으로 처리하므로 `ItemSpawn` 자체는 코인 타입을 전혀 몰라도 됨 — `HandleRoulettePickedUp`이 검증을 마친 뒤 그대로 호출할 만큼 단순한 진입점 (`FName ItemID, int32 Count` 시그니처가 `FOnCPRoulettePickedUp`과 정확히 일치)
  - `SpawnBigCoin(ItemID, Count)` : `ValidateItemCoinType(ItemID, Big)`을 통과해야 진행. Count번 반복해서(매번 Dispenser를 새로 고름) 고른 Dispenser의 `DispenseCoinByID(ItemID)`로 코인 1개씩을 스폰하고, 성공하면 `SetOwningCoinPusher(this)`로 이 CoinPusher를 알려준 뒤 `SetCoinType(Big)`을 호출
  - `SpawnMonsterCoin(ItemID, Num)` : `ValidateItemCoinType(ItemID, Monster)`을 통과해야 진행. `SpawnBigCoin`과 동일한 방식으로 Num번 반복해서 `DispenseCoinByID(ItemID)`로 코인을 스폰하고 `SetCoinType(Monster)`를 호출 — Big과 달리 `SetOwningCoinPusher()`는 호출하지 않음(Monster는 WaveThrow 트리거와 무관)
- `ConvertActive(ItemID, SpawnCount)` / `HPConvertActive(ItemID, SpawnCount)` / `MonsterConvertActive(ItemID, SpawnCount)` / `SpawnTower(ItemID, SpawnCount)` : 각각 `ValidateItemCoinType(ItemID, Passive/HP/Monster/CoinTower)`를 통과해야 `GetPassiveCoinConvertArea()->ConvertActive()`/`HPConvertActive()`, `GetMonsterCoinConvertArea()->MonsterConvertActive()`, `GetCoinTowerSpawner()->SpawnTower()`에 각각 위임하는 랩퍼 — `ACPPassiveCoinConvertArea`/`ACPCoinTowerSpawner`를 직접 호출하는 대신 이 랩퍼들을 거치면 CoinType 검증이 자동으로 이루어짐
- `ActiveWaveThrow()` : `WaveThrowInterval`(기본 0.15초) 간격으로 `CoinThrowAreaComponents` 5개를 인덱스 순서대로 하나씩 `ActiveThrow()` — 첫 번째는 즉시 호출되고 이후 매 인터벌마다 다음 것을 활성화, 5개를 모두 돌면 타이머를 정지. `GetCoinThrowArea(Index)`로 각 ChildActorComponent가 실제로 스폰한 `ACPCoinThrowArea` 인스턴스에 접근. Big 코인이 스폰 0.2초 뒤부터 무엇과든 처음 부딪히면(`ACPCoin::HandleMeshHit`) 이 함수가 자동으로 한 번 호출됨

## 화면 캡처(Picture-in-Picture) 시스템

각 `ACPCoinPusher`는 `ViewCaptureBoom`(SpringArm) + `ViewCaptureComponent`(SceneCaptureComponent2D)로 자기 자신을 비추는 카메라를 갖고 있고, 이 화면을 게임 화면 왼쪽 30%에 항상 띄울 수 있다.

- `UCPCoinPusherViewportClient`(`CoinPusher/CPCoinPusherViewportClient.h`) : 프로젝트 전역 `GameViewportClient` (`DefaultEngine.ini`의 `[/Script/Engine.Engine] GameViewportClientClassName`으로 지정됨). `LayoutPlayers()`에서 1P 로컬 플레이어의 카메라 뷰포트를 `CaptureWidthRatio`(기본 0.3)만큼 왼쪽을 제외한 오른쪽 영역으로 축소한다 — Player 카메라와 PIP가 같은 픽셀을 두고 겹쳐그리기 경쟁을 하지 않도록, Player 뷰 자체를 줄이는 방식
- `UCPCoinPusherViewCaptureComponent`(`CoinPusher/CPCoinPusherViewCaptureComponent.h`) : `SceneCaptureComponent2D` 서브클래스. `TextureTarget`을 BP/디테일 패널에서 직접 Render Target 에셋으로 지정할 수 있고, 비워두면 `BeginPlay`에서 게임 뷰포트 실제 픽셀 크기(`CaptureWidthRatio` 반영) + `SupersampleFactor`에 맞춰 자동 생성한다(확대로 인한 pixel화 방지). `bAlwaysPersistRenderingState = true`와 캡처 자체의 Lumen GI/Reflection Off, Auto Exposure Min/MaxBrightness 고정 등 이 프로젝트(Lumen/RayTracing 사용)에서 캡처가 검게 나오지 않도록 하는 설정이 생성자에 들어있다. `bCaptureEveryFrame`의 엔진 자동 캡처가 런타임 생성 RenderTarget과 잘 맞지 않아, 대신 `TickComponent`에서 매 틱 직접 `CaptureScene()`을 호출한다
- `UCPCoinPusherCaptureWidget`(`CoinPusher/CPCoinPusherCaptureWidget.h`) : WBP 없이 `RebuildWidget()`에서 Slate(`SConstraintCanvas` + `SImage`)로 직접 화면 왼쪽 `CaptureWidthRatio`(기본 0.3) 영역에 이미지를 앵커링하는 위젯. `SetCaptureTexture(RenderTarget)`으로 표시할 텍스처를 지정. 이 클래스 자체를 그대로 써도 되고(별도 WBP 불필요), 커스텀 스타일이 필요하면 WBP로 상속해서 오버라이드 가능
- 실제로 화면에 띄우려면 PlayerController가 `UCPCoinPusherCaptureWidget`을 만들어 `AddToViewport()`하고 CoinPusher(또는 캡처를 가진 액터)의 `ViewCaptureComponent->GetViewRenderTarget()`을 `SetCaptureTexture()`로 넘겨줘야 한다 — 예시는 `CoinPusher/Test/CPCoinPusherCaptureTestPlayerController` 참고

## 테스트용 Actor/GameMode/PlayerController (`CoinPusher/Test`)

- **PIP 캡처 테스트**: `ACPCoinPusherCaptureTestActor`(비주얼 메시 + `ViewCaptureBoom`/`ViewCaptureComponent`를 직접 가진 독립 액터, 실제 `ACPCoinPusher` 전체 설정 없이 캡처 기능만 테스트) / `ACPCoinPusherCaptureTestPawn`(`ADefaultPawn` 상속, 자유비행 카메라) / `ACPCoinPusherCaptureTestGameMode`(위 Pawn을 DefaultPawnClass로) / `ACPCoinPusherCaptureTestPlayerController`(레벨의 `ACPCoinPusherCaptureTestActor` 또는 `ACPCoinPusher`를 찾아 `UCPCoinPusherCaptureWidget`을 만들고 RenderTarget을 연결)
- **ItemSpawn / PassiveCoinConvertArea / CoinThrowArea / Roulette / DropZone / InGamePause·Ending UI 테스트**: `ACPCoinPusherItemSpawnTestPawn`(`ADefaultPawn` 상속, `TargetCoinPusher`/`TargetRoulette`를 직접 할당하거나 레벨에서 자동으로 찾음 — ConvertArea/ThrowArea 모두 CoinPusher가 ChildActorComponent로 소유하므로 이 Pawn은 CoinPusher/Roulette 둘만 참조).
  - Space바: `TargetCoinPusher->ItemSpawn(CoinItemID, 1)` 호출
  - R키: `TargetRoulette->Roll()` 호출
  - P키: `TargetCoinPusher->ConvertActive(PassiveConvertItemID, PassiveConvertCount)` 호출
  - O키: `TargetCoinPusher->HPConvertActive(HPConvertItemID, HPConvertCount)` 호출 (기본 5개, Normal 코인만 대상)
  - M키: `TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, MonsterConvertCount)` 호출 (기본 5개, Normal 코인만 대상)
  - N키: `TargetCoinPusher->ActiveWaveThrow()` 호출 (Monster 관련 기능 추가로 M키를 MonsterConvertActive에 내주면서 이쪽으로 옮김)
  - I키: `TargetCoinPusher->SpawnBigCoin(BigCoinItemID)` 호출
  - U키: `TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, MonsterCoinSpawnCount)` 호출 (기본 10개)
  - 1/2/3/4/5/6키: `TargetCoinPusher->SpawnTower(CoinTowerItemID, N)` 호출 (N = 5/10/15/20/25/30층)
  - P/O/M/I/U/1-6 모두 각 랩퍼가 대응하는 ItemID(`PassiveConvertItemID`/`HPConvertItemID`/
    `MonsterConvertItemID`/`BigCoinItemID`/`MonsterCoinItemID`/`CoinTowerItemID`, 모두 EditAnywhere)로
    `TargetCoinPusher->ItemDataTable`의 CoinType을 검증하므로, 값이 맞지 않으면 아무 일도 일어나지 않음
  - Z키: possessing `ACPTopDownPlayerController::ShowEndingResult(true)` 호출 — Ending 위젯에 Clear 결과 표시
  - X키: 같은 방식으로 `ShowEndingResult(false)` 호출 — Ending 위젯에 Lose 결과 표시
  - `ACPCoinPusherItemSpawnTestGameMode`(위 Pawn을 DefaultPawnClass로) — `ICPDroppedItemReceiver`를 구현해 DropZone의 드랍 정보 전달을 로그로 확인 가능. `PlayerControllerClass`는 `ACPCoinPusherItemSpawnTestPlayerController`(`ACPTopDownPlayerController` 상속)로 지정되어 있어, 실제 게임과 동일한 `PauseAction`(게임패드 Menu/키보드 Escape)으로 InGamePause 메뉴도 함께 테스트할 수 있다 — Input Action/위젯 클래스는 BP에서 채워야 하므로 자세한 준비 절차는 `UI/README.md`의 에디터 체크리스트 9-10번 참고

## Roulette 연동

`ACPRoulette`(`Source/CP/Roulette`)는 `ItemDataTable`에서 `bRoulette`가 true인 행들을 확률 가중치로
돌려 당첨된 행을 결정하고, `OnPickedUp(ItemID, Count)`을 Broadcast하는 것으로 자신의 역할을 끝낸다 —
결과를 누가 받는지 전혀 모름. `ACPCoinPusher`가 `LinkedRoulette`로 연결한 뒤 `BeginPlay()`에서
`OnPickedUp`에 자신의 `HandleRoulettePickedUp`을 구독해 "룰렛에서 당첨된 아이템 중
`FItemData::bRouletteToCoinPusher`가 true인 것만 CoinPusher의 천장 Dispenser 중 하나에서 나온다"는
흐름을 만든다. 자세한 내용은 `Roulette/README.md` 참고.

## 플레이어 상호작용 (ACPCharacter 수정)

`ACPInput`을 실제로 사용할 수 있도록 기본 플레이어 캐릭터(`ACPCharacter`)에 상호작용 기능을 추가:

- `InteractAction`(UInputAction), `InteractionDistance` 프로퍼티 추가
- `DoInteract()` : 캐릭터 전방으로 Sphere Sweep을 수행해 `ICPInteractable`을 구현한 Actor를 찾고, 있으면 `Interact(this)` 호출

## 에디터에서 준비해야 할 것

C++ 클래스들은 모두 abstract이므로 실제 배치를 위해서는 각 클래스를 상속하는 Blueprint를 만들어야 함:

1. `BP_CPCoin`, `BP_CPItem`, `BP_CPPusher`, `BP_CPInput`, `BP_CPDispenser`, `BP_CPDropZone`, `BP_CPCoinPusher` 생성 후 메시/충돌 크기 등 콘텐츠 설정 (`BP_CPItem`은 `Mesh`에 구 모양 스태틱 메시를 assign하고, `CollisionSphere`의 반지름을 그 메시 크기에 맞게 조정, `ItemCode`도 지정)
2. `BP_CPCoinPusher`에서 `PusherComponent`의 `Child Actor Class`를 `BP_CPPusher`로, `DispenserComponentA`/`DispenserComponentB`/`CeilingDispenserComponents`(5개 전부)의 `Child Actor Class`를 `BP_CPDispenser`로, `DropZoneComponent`의 `Child Actor Class`를 `BP_CPDropZone`으로 지정 (모두 CoinPusher가 소유하므로 레벨에 별도 배치 불필요)
3. `BP_CPCoinPusher` 인스턴스를 레벨에 배치하면 Pusher/Dispenser 7개(앞 2 + 천장 5)/DropZone이 자식 액터로 함께 스폰됨. 필요하면 각 컴포넌트의 상대 위치를 조정해 배치 (천장 5개는 천장 근처로, 서로 겹치지 않게)
4. 각 Dispenser 자식 액터를 선택해 `ItemClass`를 지정: 앞의 2개(`DispenserComponentA`/`B`)와 천장 5개 전부 기본은 `BP_CPCoin`, 아이템을 뿌리고 싶은 Dispenser만 `BP_CPItem`으로 변경
5. `ACPInput`(`BP_CPInput`) 2개를 레벨에 배치하고, `BP_CPCoinPusher` 인스턴스의 `InputA` / `InputB`에 각각 연결 (Dispenser의 `LinkedInput`은 여기서 직접 건드리지 않아도 `PostInitializeComponents`가 자동으로 설정)
6. 적(또는 데미지를 주는 무엇이든)이 `UGameplayStatics::ApplyDamage(CoinPusher, Damage, Instigator, Causer, DamageType)` 등을 호출하면 `ACPCoinPusher::TakeDamage`를 통해 자동으로 체력이 깎임 (별도의 태그/오버랩 설정 불필요)
7. `ACPCharacter`를 상속하는 캐릭터 BP에 `InteractAction` Input Action 에셋을 연결
8. Row Struct가 `FItemData`인 DataTable 에셋을 하나 만들고(예: `DT_ItemData`), 행마다 `ID`(=RowName과
   일치시키는 것을 권장)/`Category`/`Type`/`Name`/`CoinType`/`CoinPusherSpawnBPClass`를 등록 (룰렛에서
   뽑히길 원하는 행이면 `bRoulette`/`RouletteProbability`/`RouletteSpawnCount`도 설정)
9. ItemID로 스폰해야 하는 모든 Dispenser(천장 Dispenser 등)의 `ItemDataTable`에 위에서 만든 DataTable을 연결.
   `BP_CPCoinPusher` 인스턴스 자신의 `ItemDataTable`에도 같은(또는 호환되는) DataTable을 연결해야
   `HandleRoulettePickedUp()`의 `bRouletteToCoinPusher` 조회와 `ConvertActive()`/`HPConvertActive()`/
   `MonsterConvertActive()`/`SpawnBigCoin()`/`SpawnMonsterCoin()`/`SpawnTower()` 랩퍼의 CoinType 검증이 동작함
10. `BP_CPCoinPusher` 인스턴스의 `ItemRespawnDispenser`에 (보통 천장 Dispenser 자식 액터 중 하나를 피커로 선택해) 연결 — DropZone에 아이템이 떨어졌을 때 이 Dispenser가 재생성을 담당
11. 룰렛을 연동하려면 `BP_CPRoulette`(`ACPRoulette` 상속) 인스턴스를 레벨에 배치하고, 그 `ItemDataTable`에
    위와 동일한(또는 다른) `FItemData` DataTable을 연결한 뒤, `BP_CPCoinPusher` 인스턴스의
    `LinkedRoulette`에 그 `BP_CPRoulette`를 연결 (자세한 내용은 `Roulette/README.md` 참고)
