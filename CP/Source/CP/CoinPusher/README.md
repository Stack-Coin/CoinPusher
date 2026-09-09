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
    - `CoinThrowAreaComponents` (`UChildActorComponent`, 5개) : **컴포넌트를 통한 Has-a** — 코인을 날려보내는 `ACPCoinThrowArea` 5개를 소유. `GetCoinThrowArea(Index)`로 실제 스폰된 인스턴스 접근, `ActiveWaveThrow()`가 이 5개를 순차적으로 활성화
    - 체력(Health) 보유, 적(Enemy 태그)과 접촉 시 피해를 입음
- `ACPPusher` : 앞뒤로 왕복 운동하며 코인을 밀어내는 Actor
- `ACPDispenser` : 설정된 `ICPCoinPusherItem` 오브젝트(코인, 아이템 등)를 생성해 앞으로 던지는 Actor. **Has-a** `ACPInput`
- `ACPInput` : 플레이어와 상호작용 가능한 Actor (`ICPInteractable` 구현)
- `ACPDropZone` : `ICPCoinPusherItem`이 떨어지면 수거하는 트리거 Actor
- `ACPCoin` : 물리 시뮬레이션을 받는 코인 Actor. `ICPCoinPusherItem` 구현 — DropZone에 떨어지면 코인 개수 증가
- `ACPCoinThrowArea` : 겹쳐 있는 모든 타입의 코인을 월드 X(앞)/Z(위) 방향으로 날려보내는 트리거 볼륨 (아래 "ACPCoinThrowArea" 참고)
- `ACPItem` : 물리 시뮬레이션을 받는 프라이즈/아이템 Actor. `ICPCoinPusherItem` 구현 — DropZone에 떨어지면 `ItemCode` 기록
- `ICPCoinPusherItem` : Dispenser가 생성하고 DropZone이 수거할 수 있는 오브젝트를 위한 인터페이스 (`OnDroppedInZone(ACPDropZone*)`)
- `ICPInteractable` : 상호작용 인터페이스 (`Interact(AActor* Interactor)`)
- `UCPItemRegistry` : ItemID + 스폰 방식(CoinPusherItem/WorldItem)을 주면 생성할 클래스를 반환하는 데이터 에셋. `ACPDispenser`를 비롯해 아이템을 스폰하는 어떤 클래스든 참조해서 재사용 가능

## 클래스별 상세

### ICPCoinPusherItem
- `OnDroppedInZone(ACPDropZone* DropZone)` 하나만 가진 인터페이스. `ACPDropZone`이 무엇이 떨어졌는지 구체 타입을 몰라도 처리할 수 있게 해준다 (디커플링)
- 구현체가 스스로 `DropZone`의 공개 API(`AddCollectedCoins`, `RecordCollectedItem`)를 호출해서 자신이 어떤 종류인지 알림 → 파괴/정리까지 스스로 담당

### ACPCoin
- `Mesh`(StaticMeshComponent, RootComponent)에서 물리 시뮬레이션(SimulatePhysics)을 켜서 중력/충돌의 영향을 받음
- `CoinThrowAreaComponent`(`UChildActorComponent`, `Mesh`에 부착) : **컴포넌트를 통한 Has-a** — 스케일 연출이 최대 크기에 도달했을 때 활성화할 `ACPCoinThrowArea`를 소유. 실제 사용할 BP 서브클래스는 이 컴포넌트의 Child Actor Class에 지정(`BP_CPCoinThrowArea` 등). `GetCoinThrowArea()`로 실제 스폰된 인스턴스 접근
- `ItemID`(FName, EditAnywhere) : `UCPItemRegistry`/Dispenser가 쓰는 ItemID와 동일한 개념의 식별자. 다만 아직 `OnDroppedInZone`에서 사용하지 않음 — `AddCollectedCoins(1)`만 호출하고 `ItemID`는 참조하지 않는 상태 (필요하면 `ACPItem`처럼 `RecordCollectedItem`/재생성 로직과 연결 가능)
- `ICPCoinPusherItem` 구현
- `Launch(LaunchVelocity)` : `Mesh`에 물리 속도를 부여해 날림 (`ACPCoinThrowArea::ActiveThrow()`가 호출). 이미 `bIsLaunched`가 true(=날아가고 있는 중)면 아무것도 하지 않고 무시 — 여러 CoinThrowArea가 짧은 시간 안에 같은 코인을 중복으로 발사해 속도가 비정상적으로 누적되는 것을 방지. `LaunchCooldown`(EditAnywhere, 기본 1초) 경과 후 자동으로 `bIsLaunched`가 다시 false로 돌아와 재발사 가능해짐 (참고: `ACPDispenser::SpawnItemClass()`의 초기 스폰 발사는 이 함수를 거치지 않고 RootComponent에 직접 속도를 부여하므로 이 쿨다운의 영향을 받지 않음)
- `Collect()` : `BP_OnCollected` 이벤트 후 자신을 Destroy
- `OnDroppedInZone(DropZone)` : `DropZone->AddCollectedCoins(1)` 호출 후 `Collect()`
- `CoinType`(`ECPCoinType`: Normal/Passive/Giant/HP) : `SetCoinType(NewType)`으로 변경. 실제로 값이 바뀔 때만 아래 동작들을 트리거하고 `BP_OnCoinTypeChanged(OldType, NewType)` BP 이벤트를 호출
  - Passive 또는 HP로 전환 시 동일한 스케일 연출 재생(공용 로직): 원본 스케일 → `ScaleAnimMinScale`로 축소 → `ScaleAnimMaxScale`로 확대 → 원본 스케일로 복귀 (속도는 `ScaleAnimSpeed`, 세 값 모두 EditAnywhere, `Coin|ScaleAnim` 카테고리). 다른 타입으로 다시 바뀌면 진행 중이던 연출은 즉시 취소되고 원본 스케일로 복귀
  - 최대 크기(`ScaleAnimMaxScale`)에 도달하는 순간 `CoinThrowAreaComponent`가 스폰한 `ACPCoinThrowArea`(`GetCoinThrowArea()`)를 이 코인 위치 기준 월드 X방향으로 `CoinThrowAreaOffsetX`만큼 떨어뜨려 옮기고 `ActiveThrow()`를 호출 — "커졌을 때 주변 코인을 날려보내는" 연출
  - `CoinTypeVisuals`(`TMap<ECPCoinType, FCPCoinTypeVisual>`, EditAnywhere) : 타입별로 지정한 `Mesh`/`Material`이 있으면 전환 시점에 `Mesh` 컴포넌트에 `SetStaticMesh()`/`SetMaterial(0, ...)`로 적용. 둘 중 비워둔(nullptr) 필드나 맵에 없는 타입은 바꾸지 않음 — 예: Passive만 별도 메시를 쓰고 싶으면 `CoinTypeVisuals`에 `Passive` 항목만 채워두면 됨

### ACPCoinThrowArea
- `ThrowVolume`(UBoxComponent, RootComponent, Overlap 전용 — `OverlapAllDynamic`) : 충돌하지 않고 겹침만 감지. 크기는 BP/디테일 패널에서 자유롭게 조정
- `ActiveThrow()` : `ThrowVolume`과 겹쳐 있는 모든 타입의 `ACPCoin`에게 `Launch()`로 속도를 부여해 날림. 위(월드 Z)/앞(월드 X) 방향은 액터 회전과 무관하게 항상 고정
- `MaxUpPower`/`MinUpPower`, `MaxForwardPower`/`MinForwardPower`(모두 EditAnywhere) : `bIsRandomize`가 true면 매 `ActiveThrow()`마다 Min~Max 사이에서 랜덤한 위/앞 힘을 사용, false면 항상 Max 값을 그대로 사용
- 두 곳에서 각자 별도의 `ACPCoinThrowArea` 인스턴스를 **컴포넌트를 통한 Has-a**로 소유함 — 서로 다른 용도이므로 공유하지 않음
  - `ACPCoinPusher`의 `CoinThrowAreaComponents`(ChildActorComponent 5개) : `ActiveWaveThrow()`가 순차적으로 활성화
  - `ACPCoin`의 `CoinThrowAreaComponent`(ChildActorComponent 1개) : 자신의 스케일 연출이 최대 크기에 도달하면 활성화 (위 "ACPCoin" 참고)

### ACPItem
- `CollisionSphere`(USphereComponent, RootComponent, 물리 시뮬레이션) — 구 형태라 자연스럽게 굴러감. `ACPCoin`과 동일하게 Dispenser가 `UPrimitiveComponent` 루트로 인식해 발사 가능
- `Mesh`(StaticMeshComponent, `CollisionSphere`에 부착, 콜리전 없음) — 구 메시를 assign하는 순수 비주얼 파츠
- `ItemCode`(FName, EditAnywhere)로 어떤 아이템인지 식별
- `ICPCoinPusherItem` 구현
- `OnDroppedInZone(DropZone)` : `DropZone->RecordCollectedItem(ItemCode)` 호출 후 `BP_OnCollected` 이벤트 + Destroy

### UCPItemRegistry
- `UDataAsset` 서브클래스라서 BP 에셋으로 만들어(예: `DA_CPItemRegistry`) Dispenser 등 여러 클래스가 같은 인스턴스를 참조 형태로 공유할 수 있음
- `Items`(`TMap<FName, FCPDispenserItemEntry>`, EditAnywhere) : ItemID 키 → `FCPDispenserItemEntry`(`CoinPusherItemClass` + `WorldItemClass`) 값. 에셋 디폴트에서 직접 채워 넣을 수 있음
- `RegisterItem(ItemID, CoinPusherItemClass, WorldItemClass)` : BP 그래프 등에서 런타임에 항목을 추가/덮어쓸 때 사용
- `GetItemClass(ItemID, SpawnType)` : `SpawnType`(CoinPusherItem/WorldItem)에 맞는 클래스를 반환 (없으면 nullptr)

### ACPDropZone
- `CollectionVolume`(UBoxComponent)에 `ICPCoinPusherItem`을 구현하는 오브젝트가 겹치면(OnComponentBeginOverlap) 감지해 `Item->OnDroppedInZone(this)` 호출 — Coin/Item 등 구체 타입은 전혀 모름
- `AddCollectedCoins(int32 Amount = 1)` : `CollectedCoinCount` 증가 + `OnCoinCollected` 브로드캐스트 (`ACPCoin`이 호출). `CollectedCoinCount`가 10의 배수가 될 때마다 `GetAuthGameMode()`로 `ACPGameMode`를 찾아 떨어진 코인 수량(`CollectedCoinCount`)을 그대로 넘겨줄 준비를 함 — 티켓 환산 로직은 GameMode 쪽 책임. 실제 `GameMode->UpdateTicketCount(CollectedCoinCount)` 호출은 `ACPGameMode`에 아직 그 함수가 없어서 주석 처리된 상태 (TODO)
- `RecordCollectedItem(FName ItemCode)` : `CollectedItemCodes` 배열에 추가 + `OnItemCollected` 브로드캐스트 (`ACPItem`이 호출). 이어서 `ItemRespawnDispenser`가 설정되어 있으면 `DispenseItemByID(ItemCode, 1)`을 호출해 같은 아이템을 다시 생성 요청 — "아이템이 Drop되면 해당 ItemID를 연결된 Dispenser에 넘겨 Spawn" 요구사항을 만족
- `ItemRespawnDispenser`(`TObjectPtr<ACPDispenser>`, VisibleInstanceOnly) : 위 재생성을 맡을 Dispenser. **DropZone은 `ACPCoinPusher`의 ChildActorComponent로 스폰되는 인스턴스라 레벨에서 직접 편집할 수 없으므로**, 에디터에서 직접 설정하지 않고 `SetItemRespawnDispenser()`를 통해서만 설정됨 (소유자인 `ACPCoinPusher`가 `PostInitializeComponents`에서 호출)

### ACPPusher
- `PushPlate`(StaticMeshComponent)를 물리 시뮬레이션 없이(Kinematic) `Movable`로 두고 `Tick`에서 위치만 이동
- `Tick`에서 sine 파형으로 0~`PushDistance` 사이를 `CycleSpeed` 속도로 왕복 이동시켜 코인을 밀어냄
- 물리 시뮬레이션 코인(ACPCoin)과는 충돌 블로킹으로 밀어내는 상호작용이 발생

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
- `ItemRegistry`(`TObjectPtr<UCPItemRegistry>`, EditAnywhere) : ItemID → 클래스 조회에 사용하는 공용 데이터 에셋 참조. 여러 Dispenser가 같은 에셋을 공유해서 지정할 수 있음
- `DispenseItemByID(ItemID, SpawnCount, SpawnType = CoinPusherItem, bLaunch = true)` : `ItemRegistry->GetItemClass(ItemID, SpawnType)`로 클래스를 찾아 `SpawnCount`개 생성. `CoinPusherItem` 타입은 `ICPCoinPusherItem` 구현 여부를 검사하고(WorldItem은 검사 안 함), `bLaunch=false`면 발사 속도를 부여하지 않고 그 자리에 둠 — Roulette/DropZone처럼 특정 ItemID를 지정해서 만들어야 하는 경우에 사용 (`DispenseItem()`/`DispenseItems()`는 여전히 단일 `ItemClass`를 쓰는 기존 경로)
- `SpawnItemClass(ClassToSpawn, bLaunch)`(private) : 실제 스폰 + (옵션) 발사를 수행하는 공용 헬퍼. `DispenseItem()`과 `DispenseItemByID()`가 함께 사용

### ACPCoinPusher
- `Floor`(`UBoxComponent`, RootComponent) : 액터의 루트. `BlockAllDynamic` 프로파일로 실제 충돌 기준이 됨. `BoxExtent`로 직접 크기 지정 (예: `150,150,10`)
- `Body`(StaticMeshComponent, `Floor`에 부착) : `SetCollisionEnabled(NoCollision)`으로 콜리전을 꺼서 순수 비주얼 메시로만 사용
- `LeftWall` / `RightWall` / `BackWall` / `FrontWall`(`UBoxComponent`, `Floor`에 부착) : `Floor`와 함께 전부 `BlockAllDynamic` 프로파일 — 코인(WorldDynamic, 물리 시뮬레이션)을 막아 기계 안에 가두고, Pawn도 막아 실제 벽처럼 동작. 상대 위치/크기는 BP에서 실제 메시에 맞게 조정. `FrontWall`은 `BeginPlay`에서 타이머를 걸어 `FrontWallRemovalDelay`초 후 `RemoveFrontWall()`로 콜리전/비주얼을 꺼서 코인이 앞으로 빠질 수 있게 함
- `ExtraBoxMesh`(`UStaticMeshComponent`, `Floor`에 부착, 콜리전 없음) : 추가 비주얼 메시. 아직 특정 용도는 없고 확장을 위한 자리
- `PusherComponent` / `DispenserComponentA` / `DispenserComponentB` / `CeilingDispenserComponents`(5개) / `DropZoneComponent` / `PassiveCoinConvertAreaComponent` / `CoinThrowAreaComponents`(5개) 모두 `UChildActorComponent`로 각각 `ACPPusher`, `ACPDispenser`, `ACPDropZone`, `ACPPassiveCoinConvertArea`, `ACPCoinThrowArea`를 소유 — 전부 Actor이지만 **컴포넌트로 감싸서 Has-a 관계**를 구현 (실제 사용할 BP 서브클래스는 각 컴포넌트의 `Child Actor Class`에 지정)
- `InputA` / `InputB`(`TObjectPtr<ACPInput>`, EditInstanceOnly) : 레벨에 배치한 `ACPInput`을 CoinPusher에서 직접 연결 (앞으로 던지는 `DispenserComponentA`/`B`용)
- `ItemRespawnDispenser`(`TObjectPtr<ACPDispenser>`, EditInstanceOnly) : DropZone에 아이템이 떨어졌을 때 재생성을 맡을 Dispenser. 레벨에서 이 CoinPusher 인스턴스의 자식 액터로 스폰된 Dispenser 중 하나를(보통 천장 Dispenser) 피커로 선택해서 지정
- `InitialCoinDropCount`(int32, EditAnywhere, 기본값 10) : 게임 시작 시 천장 Dispenser 하나당 드롭할 코인 개수
- `PostInitializeComponents()` : Dispenser 자식 액터가 스폰된 직후(=`BeginPlay` 이전) `GetDispenserA()->SetLinkedInput(InputA)`, `GetDispenserB()->SetLinkedInput(InputB)`를 호출해 Dispenser의 `LinkedInput`을 CoinPusher가 대신 설정하고, `GetDropZone()->SetItemRespawnDispenser(ItemRespawnDispenser)`도 함께 호출. Dispenser/DropZone 모두 ChildActorComponent로 스폰되는 인스턴스라 에디터에서 직접 편집한 값이 안정적으로 유지되지 않기 때문에 CoinPusher가 대신 전달해준다
- `BeginPlay()` : `CeilingDispenserComponents` 5개를 순회하며 각각 스폰된 `ACPDispenser`의 `DispenseItems(InitialCoinDropCount)`를 호출 — "게임 시작 시 5개의 Dispenser에서 코인을 10개씩 드롭" 요구사항을 만족. 실제로 코인이 나오려면 5개 천장 Dispenser BP 인스턴스의 `ItemClass`를 코인 클래스(`BP_CPCoin`)로 지정해야 함
- `GetDispenserA()` / `GetDispenserB()` / `GetCeilingDispenser(Index)` / `GetDropZone()` : 해당 ChildActorComponent가 실제로 스폰한 액터 인스턴스를 캐스팅해 반환 (`Child Actor Class`가 지정되어야 유효)
- 체력 시스템: `MaxHealth` / `CurrentHealth`, `ApplyDamage(Damage, DamageCauser)`로 감소
- `TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser)` 오버라이드 : 표준 엔진 데미지 경로. `Super::TakeDamage(...)`를 호출해 실제 데미지 값을 구한 뒤 `ApplyDamage(ActualDamage, DamageCauser)`로 위임 — 적(또는 다른 무엇이든)이 `UGameplayStatics::ApplyDamage` / `ApplyPointDamage` / `ApplyRadialDamage`를 호출하면 이 경로를 통해 체력이 깎임. 기존에 있던 오버랩 태그 기반 피격 판정(`OnActorOverlapBegin`, `EnemyActorTag`, `EnemyContactDamage`)은 제거하고 이 방식으로 대체함
- 체력이 0 이하가 되면 `HandleDestroyed()` → `OnCoinPusherDestroyed` 브로드캐스트 + `BP_OnDestroyed` BP 이벤트 호출
- `ItemSpawn(ItemID, SpawnCount)` : `CeilingDispenserComponents` 5개 중 실제로 스폰된 `ACPDispenser`들 가운데 하나를 랜덤하게 골라 그 Dispenser의 `DispenseItemByID(ItemID, SpawnCount)`를 호출 — Roulette 등 외부 시스템이 "이 ItemID를 이만큼 만들어줘"라고 요청하는 진입점
- `ActiveWaveThrow()` : `WaveThrowInterval`(기본 0.15초) 간격으로 `CoinThrowAreaComponents` 5개를 인덱스 순서대로 하나씩 `ActiveThrow()` — 첫 번째는 즉시 호출되고 이후 매 인터벌마다 다음 것을 활성화, 5개를 모두 돌면 타이머를 정지. `GetCoinThrowArea(Index)`로 각 ChildActorComponent가 실제로 스폰한 `ACPCoinThrowArea` 인스턴스에 접근

## 화면 캡처(Picture-in-Picture) 시스템

각 `ACPCoinPusher`는 `ViewCaptureBoom`(SpringArm) + `ViewCaptureComponent`(SceneCaptureComponent2D)로 자기 자신을 비추는 카메라를 갖고 있고, 이 화면을 게임 화면 왼쪽 30%에 항상 띄울 수 있다.

- `UCPCoinPusherViewportClient`(`CoinPusher/CPCoinPusherViewportClient.h`) : 프로젝트 전역 `GameViewportClient` (`DefaultEngine.ini`의 `[/Script/Engine.Engine] GameViewportClientClassName`으로 지정됨). `LayoutPlayers()`에서 1P 로컬 플레이어의 카메라 뷰포트를 `CaptureWidthRatio`(기본 0.3)만큼 왼쪽을 제외한 오른쪽 영역으로 축소한다 — Player 카메라와 PIP가 같은 픽셀을 두고 겹쳐그리기 경쟁을 하지 않도록, Player 뷰 자체를 줄이는 방식
- `UCPCoinPusherViewCaptureComponent`(`CoinPusher/CPCoinPusherViewCaptureComponent.h`) : `SceneCaptureComponent2D` 서브클래스. `TextureTarget`을 BP/디테일 패널에서 직접 Render Target 에셋으로 지정할 수 있고, 비워두면 `BeginPlay`에서 게임 뷰포트 실제 픽셀 크기(`CaptureWidthRatio` 반영) + `SupersampleFactor`에 맞춰 자동 생성한다(확대로 인한 pixel화 방지). `bAlwaysPersistRenderingState = true`와 캡처 자체의 Lumen GI/Reflection Off, Auto Exposure Min/MaxBrightness 고정 등 이 프로젝트(Lumen/RayTracing 사용)에서 캡처가 검게 나오지 않도록 하는 설정이 생성자에 들어있다. `bCaptureEveryFrame`의 엔진 자동 캡처가 런타임 생성 RenderTarget과 잘 맞지 않아, 대신 `TickComponent`에서 매 틱 직접 `CaptureScene()`을 호출한다
- `UCPCoinPusherCaptureWidget`(`CoinPusher/CPCoinPusherCaptureWidget.h`) : WBP 없이 `RebuildWidget()`에서 Slate(`SConstraintCanvas` + `SImage`)로 직접 화면 왼쪽 `CaptureWidthRatio`(기본 0.3) 영역에 이미지를 앵커링하는 위젯. `SetCaptureTexture(RenderTarget)`으로 표시할 텍스처를 지정. 이 클래스 자체를 그대로 써도 되고(별도 WBP 불필요), 커스텀 스타일이 필요하면 WBP로 상속해서 오버라이드 가능
- 실제로 화면에 띄우려면 PlayerController가 `UCPCoinPusherCaptureWidget`을 만들어 `AddToViewport()`하고 CoinPusher(또는 캡처를 가진 액터)의 `ViewCaptureComponent->GetViewRenderTarget()`을 `SetCaptureTexture()`로 넘겨줘야 한다 — 예시는 `CoinPusher/Test/CPCoinPusherCaptureTestPlayerController` 참고

## 테스트용 Actor/GameMode/PlayerController (`CoinPusher/Test`)

- **PIP 캡처 테스트**: `ACPCoinPusherCaptureTestActor`(비주얼 메시 + `ViewCaptureBoom`/`ViewCaptureComponent`를 직접 가진 독립 액터, 실제 `ACPCoinPusher` 전체 설정 없이 캡처 기능만 테스트) / `ACPCoinPusherCaptureTestPawn`(`ADefaultPawn` 상속, 자유비행 카메라) / `ACPCoinPusherCaptureTestGameMode`(위 Pawn을 DefaultPawnClass로) / `ACPCoinPusherCaptureTestPlayerController`(레벨의 `ACPCoinPusherCaptureTestActor` 또는 `ACPCoinPusher`를 찾아 `UCPCoinPusherCaptureWidget`을 만들고 RenderTarget을 연결)
- **ItemSpawn / PassiveCoinConvertArea / CoinThrowArea 테스트**: `ACPCoinPusherItemSpawnTestPawn`(`ADefaultPawn` 상속, `TargetCoinPusher` 하나만 직접 할당하거나 레벨에서 자동으로 찾음 — ConvertArea/ThrowArea 모두 CoinPusher가 ChildActorComponent로 소유하므로 이 Pawn은 CoinPusher 하나만 참조).
  - Space바: `TargetCoinPusher->ItemSpawn(CoinItemID, 1)` 호출
  - P키: `TargetCoinPusher->GetPassiveCoinConvertArea()->ConvertActive(PassiveConvertCount)` 호출
  - O키: `TargetCoinPusher->GetPassiveCoinConvertArea()->HPConvertActive(HPConvertCount)` 호출 (기본 5개, Normal 코인만 대상)
  - M키: `TargetCoinPusher->ActiveWaveThrow()` 호출
  - `ACPCoinPusherItemSpawnTestGameMode`(위 Pawn을 DefaultPawnClass로)

## Roulette 연동

`ACPRoulette`(`Source/CP/Roulette`)는 8칸 룰렛을 돌려 당첨된 슬롯을 결정한다. `ACPRoulette::SpawnSlotItems`는 더 이상 직접 스폰하지 않고, `EditInstanceOnly`로 레벨에서 연결한 `CoinPusher` 참조의 `ItemSpawn(SlotData.ItemID, SlotData.SpawnCount)`를 호출한다 — "룰렛에서 당첨된 아이템이 CoinPusher의 천장 Dispenser 중 하나에서 나온다"는 흐름. 자세한 내용은 `Roulette/README.md` 참고.

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
8. `UCPItemRegistry` 데이터 에셋을 하나 만들고(예: `DA_CPItemRegistry`), `Items`에 ItemID별로 `CoinPusherItemClass`(예: `BP_CPItem`)와 필요시 `WorldItemClass`를 등록
9. ItemID로 스폰해야 하는 모든 Dispenser(천장 Dispenser 등)의 `ItemRegistry`에 위에서 만든 데이터 에셋을 연결
10. `BP_CPCoinPusher` 인스턴스의 `ItemRespawnDispenser`에 (보통 천장 Dispenser 자식 액터 중 하나를 피커로 선택해) 연결 — DropZone에 아이템이 떨어졌을 때 이 Dispenser가 재생성을 담당
