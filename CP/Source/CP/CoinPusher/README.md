# CoinPusher 관련 C++

##개요

CoinPusher 기계를 구성하는 Actor들의 C++ 구현. 모든 클래스는 `UCLASS(abstract)`로 선언되어 있으며,
실제 메시/이펙트 등 콘텐츠 참조는 이 클래스들을 상속한 Blueprint에서 채워 넣는 방식(프로젝트 기존 관례)을 따른다.

## Class 구조

- `ACPCoinPusher`
    - `Body` : 몸체 StaticMeshComponent (RootComponent)
    - `PusherComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — `ACPPusher`를 소유
    - `DispenserComponentA` / `DispenserComponentB` (`UChildActorComponent`, 2개) : **컴포넌트를 통한 Has-a** — `ACPDispenser`를 소유
    - `DropZoneComponent` (`UChildActorComponent`) : **컴포넌트를 통한 Has-a** — `ACPDropZone`을 소유
    - 체력(Health) 보유, 적(Enemy 태그)과 접촉 시 피해를 입음
- `ACPPusher` : 앞뒤로 왕복 운동하며 코인을 밀어내는 Actor
- `ACPDispenser` : 코인을 생성해 앞으로 던지는 Actor. **Has-a** `ACPInput`
- `ACPInput` : 플레이어와 상호작용 가능한 Actor (`ICPInteractable` 구현)
- `ACPDropZone` : 코인이 떨어지면 파괴하고 카운트를 올리는 트리거 Actor
- `ACPCoin` : 물리 시뮬레이션을 받는 코인 Actor
- `ICPInteractable` : 상호작용 인터페이스 (`Interact(AActor* Interactor)`)

## 클래스별 상세

### ACPCoin
- `Mesh`(StaticMeshComponent, RootComponent)에서 물리 시뮬레이션(SimulatePhysics)을 켜서 중력/충돌의 영향을 받음
- `Launch(LaunchVelocity)` : Dispenser가 던질 때 속도를 부여
- `Collect()` : DropZone에 수집되었을 때 호출, `BP_OnCollected` 이벤트 후 자신을 Destroy

### ACPDropZone
- `CollectionVolume`(UBoxComponent)에 코인이 겹치면(OnComponentBeginOverlap) 감지
- `ACPCoin`과 겹치면 `CollectedCoinCount` 증가, `OnCoinCollected` 델리게이트 브로드캐스트 후 `Coin->Collect()` 호출

### ACPPusher
- `PushPlate`(StaticMeshComponent)를 물리 시뮬레이션 없이(Kinematic) `Movable`로 두고 `Tick`에서 위치만 이동
- `Tick`에서 sine 파형으로 0~`PushDistance` 사이를 `CycleSpeed` 속도로 왕복 이동시켜 코인을 밀어냄
- 물리 시뮬레이션 코인(ACPCoin)과는 충돌 블로킹으로 밀어내는 상호작용이 발생

### ACPInput
- `Mesh`를 통해 플레이어의 상호작용 트레이스에 맞을 수 있도록 충돌 설정
- `ICPInteractable::Interact(AActor* Interactor)` 구현 → `OnInteracted` 델리게이트 브로드캐스트 + `BP_OnInteracted` BP 이벤트 호출
- 자신이 직접 Dispenser를 알 필요 없이, 상호작용 사실만 델리게이트로 알림 (디커플링)

### ACPDispenser
- `Body`(StaticMeshComponent, Root) + `SpawnPoint`(SceneComponent)로 코인이 튀어나갈 위치/방향 지정
- `LinkedInput`(`TObjectPtr<ACPInput>`, VisibleInstanceOnly)으로 **Has-a Input** 관계를 표현. Dispenser가 이제 `ACPCoinPusher`의 ChildActorComponent로 스폰되기 때문에 에디터에서 직접 편집하지 않고, `SetLinkedInput()`을 통해서만 설정한다
- `SetLinkedInput(NewLinkedInput)` : 기존 Input의 `OnInteracted` 바인딩을 해제하고 새 Input에 다시 바인딩. 소유자인 `ACPCoinPusher`가 `PostInitializeComponents`에서 호출
- `BeginPlay`에서도 `BindToLinkedInput()`을 호출해 이미 `LinkedInput`이 설정된 경우를 한 번 더 보장 (`AddUniqueDynamic`이라 중복 바인딩되지 않음)
- `DispenseCoin()` : `CoinClass`로 지정된 `ACPCoin`을 `SpawnPoint` 위치/회전으로 스폰하고, 전방+상방 속도(`LaunchForwardSpeed`, `LaunchUpwardSpeed`)로 `Launch()`
- Input이 상호작용되면 `HandleInputInteracted` → `DispenseCoin()`이 호출되어 "Input과 상호작용 시 연결된 Dispenser가 코인을 발사" 요구사항을 만족

### ACPCoinPusher
- `Body`(StaticMeshComponent, Root)
- `PusherComponent` / `DispenserComponentA` / `DispenserComponentB` / `DropZoneComponent` 모두 `UChildActorComponent`로 각각 `ACPPusher`, `ACPDispenser` x2, `ACPDropZone`을 소유 — 네 종류 모두 Actor이지만 **컴포넌트로 감싸서 Has-a 관계**를 구현 (실제 사용할 BP 서브클래스는 각 컴포넌트의 `Child Actor Class`에 지정)
- `InputA` / `InputB`(`TObjectPtr<ACPInput>`, EditInstanceOnly) : 레벨에 배치한 `ACPInput`을 CoinPusher에서 직접 연결
- `PostInitializeComponents()` : Dispenser 자식 액터가 스폰된 직후(=`BeginPlay` 이전) `GetDispenserA()->SetLinkedInput(InputA)`, `GetDispenserB()->SetLinkedInput(InputB)`를 호출해 Dispenser의 `LinkedInput`을 CoinPusher가 대신 설정. Dispenser는 ChildActorComponent로 스폰되는 인스턴스라 에디터에서 직접 편집한 값이 안정적으로 유지되지 않기 때문
- `GetDispenserA()` / `GetDispenserB()` / `GetDropZone()` : 해당 ChildActorComponent가 실제로 스폰한 액터 인스턴스를 캐스팅해 반환 (`Child Actor Class`가 지정되어야 유효)
- 체력 시스템: `MaxHealth` / `CurrentHealth`, `ApplyDamage(Damage, DamageCauser)`로 감소
- `OnActorBeginOverlap`에서 겹친 Actor가 `EnemyActorTag`(기본값 `"Enemy"`) 태그를 가지고 있으면 `EnemyContactDamage`만큼 자동으로 피해 적용
- 체력이 0 이하가 되면 `HandleDestroyed()` → `OnCoinPusherDestroyed` 브로드캐스트 + `BP_OnDestroyed` BP 이벤트 호출

## 플레이어 상호작용 (ACPCharacter 수정)

`ACPInput`을 실제로 사용할 수 있도록 기본 플레이어 캐릭터(`ACPCharacter`)에 상호작용 기능을 추가:

- `InteractAction`(UInputAction), `InteractionDistance` 프로퍼티 추가
- `DoInteract()` : 캐릭터 전방으로 Sphere Sweep을 수행해 `ICPInteractable`을 구현한 Actor를 찾고, 있으면 `Interact(this)` 호출

## 에디터에서 준비해야 할 것

C++ 클래스들은 모두 abstract이므로 실제 배치를 위해서는 각 클래스를 상속하는 Blueprint를 만들어야 함:

1. `BP_CPCoin`, `BP_CPPusher`, `BP_CPInput`, `BP_CPDispenser`, `BP_CPDropZone`, `BP_CPCoinPusher` 생성 후 메시/충돌 크기 등 콘텐츠 설정
2. `BP_CPCoinPusher`에서 `PusherComponent`의 `Child Actor Class`를 `BP_CPPusher`로, `DispenserComponentA`/`DispenserComponentB`의 `Child Actor Class`를 `BP_CPDispenser`로, `DropZoneComponent`의 `Child Actor Class`를 `BP_CPDropZone`으로 지정 (모두 CoinPusher가 소유하므로 레벨에 별도 배치 불필요)
3. `BP_CPCoinPusher` 인스턴스를 레벨에 배치하면 Pusher/Dispenser 2개/DropZone이 자식 액터로 함께 스폰됨. 필요하면 각 컴포넌트의 상대 위치를 조정해 배치
4. `ACPInput`(`BP_CPInput`) 2개를 레벨에 배치하고, `BP_CPCoinPusher` 인스턴스의 `InputA` / `InputB`에 각각 연결 (Dispenser의 `LinkedInput`은 여기서 직접 건드리지 않아도 `PostInitializeComponents`가 자동으로 설정)
5. 적(Enemy) 액터에 Actor Tag로 `Enemy`를 추가하면 `ACPCoinPusher`와 겹칠 때 자동으로 피해 발생
6. `ACPCharacter`를 상속하는 캐릭터 BP에 `InteractAction` Input Action 에셋을 연결
