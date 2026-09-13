// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Monster/CPMonsterAttackInterface.h"
#include "Monster/CPMonsterAIInterface.h"
#include "Monster/Stat/CPMonsterStatComponent.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Player/CPCoinItem.h"
#include "Debug/CPDebugTypes.h"
#include "CPMonsterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMonsterDied);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECPMonsterCCState : uint8
{
	None      = 0,
	Knockback = 1 << 0,
	Stunned   = 1 << 1,
	Rooted    = 1 << 2,
	Attacking = 1 << 3,
	Dead      = 1 << 4,
	Invulnerable = 1 << 5,
};
ENUM_CLASS_FLAGS(ECPMonsterCCState);

UCLASS()
class CP_API ACPMonsterBase : public ACharacter, public ICPMonsterAttackInterface, public ICPMonsterAIInterface, public ICPKnockbackable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACPMonsterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	/** Bound to UCPDebugCollisionSubsystem::OnCollisionVisibilityChanged. Reacts to MonsterAttackRange */
	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);

public:
	/** 정면 스윕 공격 판정의 시작/끝/두께 - AttackHitCheck()와 공격범위 표시 NotifyState가 항상
	 *  같은 값을 쓰도록 여기로 뽑음(따로 계산하면 판정이랑 화면에 보이는 범위가 어긋날 수 있음) */
	struct FAttackSweepShape
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		float Radius = 0.f;
	};
	/** InForwardOverride를 주면 그 방향 기준으로 계산함(예: 공격범위 표시가 TurnToTarget이 덜 끝난
	 *  상태에서도 실제 플레이어 방향을 보여주고 싶을 때) - 비워두면(기본) 캡슐 Forward 그대로 씀,
	 *  AttackHitCheck()의 실제 판정은 항상 기본값으로 호출해서 동작 그대로 유지됨 */
	FAttackSweepShape GetAttackSweepShape(const FVector& InForwardOverride = FVector::ZeroVector);

	/** 원형 AOE 공격(보스 슬램 등)의 판정 반경 - 기본은 AttackRange를 그대로 씀. AOE 판정을
	 *  따로 쓰는 서브클래스(보스)는 이걸 오버라이드해서 그 판정에 실제로 쓰는 반경을 반환하면,
	 *  공격범위 표시 NotifyState도 같은 값으로 그려짐 */
	virtual float GetAIAOERadius() { return GetAIAttackRange(); }

	// 공격 판정 함수
	virtual void AttackHitCheck() override;
	virtual void Dead();

protected:
	/** Dead()가 사망 연출(몽타주 재생/2초 대기) 끝에 호출 - 풀 서브시스템이 있으면 그리로 반환하고,
	 *  없으면(에디터 유틸리티 등 예외) 기존처럼 파괴함 */
	void ReturnToPoolOrDestroy();

	// 공격 함수 // BTTask에서 수행
	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) override;
	virtual void AttackByAI() override;
	virtual void CancelAIAttack() override;

	// 피격 함수 // 협업용
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// 넉백 함수 // 협업용
	virtual void ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor) override;

public:
	/** 특정 CC 상태(들)가 하나라도 걸려있는지 */
	bool HasCCState(ECPMonsterCCState State) const { return EnumHasAnyFlags(CurrentCCState, State); }

protected:
	/** CC 상태 비트를 추가. 매 틱이 아니라 상태가 실제로 바뀌는 시점(부여/해제)에만 호출됨 */
	void AddCCState(ECPMonsterCCState State) { EnumAddFlags(CurrentCCState, State); }
	/** CC 상태 비트를 해제 */
	void RemoveCCState(ECPMonsterCCState State) { EnumRemoveFlags(CurrentCCState, State); }

public:
	// StatComponent의 값을 참조
	virtual UCPMonsterStatComponent* GetAIStatComponent() const override;

	/** 매니저/스포너가 스폰 직후 호출: 지정한 라운드/웨이브 번호 기준으로 스탯을 다시 계산해 적용합니다.
	 *  (BeginPlay는 항상 1라운드/1웨이브 기준으로 초기화하므로, 실제 라운드/웨이브에 맞춰 덮어쓸 때 사용) */
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ApplyWaveStat(int32 InRound, int32 InWave);

	// Wave별
	virtual float GetAIMaxHealth() override;
	virtual float GetAICurrentHealth() override;
	virtual float GetAIMoveSpeed() override;
	virtual float GetAIAttackPower() override;

	// Default
	virtual float GetAIAttackInterval() override;
	virtual float GetAICollisionRadius() override;
	virtual float GetAICollisionHalfHeight() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;
	virtual float GetAIMoveAcceptableRadius() override;

	// 몬스터 간 분리 - StatComponent->DefaultStat 기반 (RVO 회피 값 자체는 코드 상수로 고정 - BeginPlay 참고)
	virtual float GetAISeparationPadding() override;
	virtual float GetAISeparationSpeed() override;

	/** 스포너가 스폰 Z를 계산할 때 참조: 스포너 위치(지면)로부터 이 높이만큼 띄워서 스폰함.
	 *  기본은 캡슐 Half Height를 반환해서 캡슐 바닥이 지면에 닿게 하고, 날아다니는 몬스터(Ranged)는
	 *  이걸 오버라이드해서 지면과 무관한 고정 비행 높이를 반환하면 됨 */
	virtual float GetSpawnHeightOffset() const;

	/** true면 스포너가 NavMesh 투영(ResolveFreeSpawnLocation/ProjectToNavMesh) 결과의 Z를 무시하고
	 *  GetSpawnHeightOffset() 기준 높이를 그대로 강제함. NavMesh 투영은 지면(NavMesh 표면) 위의 점을
	 *  돌려주므로, 일반 몹처럼 "지면에 닿아야 하는" 경우엔 필요하지만 Ranged/Bomb처럼 "항상 고정
	 *  비행 고도를 유지해야 하는" 경우엔 그대로 쓰면 스폰 시 지면 높이로 끌려 내려가 파묻힘 */
	virtual bool ShouldUseFixedSpawnHeight() const { return false; }

	ECPMonsterType GetMonsterType() const { return MonsterType; }

	/** UCPMonsterPoolSubsystem이 풀로 반환할 때(Dead() 경유) 호출 - Collision/Tick/AI/Movement를 전부
	 *  끄고 화면에서 숨김. Dead()를 거쳤든 안 거쳤든(Pre-warm으로 갓 스폰된 액터 포함) 이것만 호출하면
	 *  안전하게 비활성화되도록 멱등하게 구현됨 */
	virtual void OnReturnedToPool();

	/** UCPMonsterPoolSubsystem이 풀에서 꺼내 재사용할 때 호출 - 위치 배치 + 상태/Collision/Tick/AI 복원.
	 *  스탯(체력 등) 재적용은 호출부(스포너)가 기존 ApplyWaveStat()으로 별도 처리함 - 여기서는 안 건드림.
	 *  서브클래스(Bomb의 FuseEffect 등)는 Super:: 호출 후 자기 고유 이펙트/상태만 추가로 리셋 */
	virtual void OnAcquiredFromPool(const FTransform& NewTransform);

protected:
	/** RVO 회피 가중치(0~1) - 다른 몬스터와 경로가 겹칠 때 자기 진행 방향을 얼마나 고수할지.
	 *  기본은 전부 동일(일반 몹끼리는 서로 동등하게 비켜줘야 자연스러운 스웜이 됨). 보스처럼
	 *  "남들이 나한테 더 비켜줘야 하는" 예외만 이걸 오버라이드해서 값을 올리면 됨 - 일반 몹끼리의
	 *  상호 회피(0.5 vs 0.5)는 그대로 유지되고, 보스와 마주칠 때만 보스가 덜 양보하게 됨 */
	virtual float GetAIAvoidanceWeight() const { return 0.5f; }

protected:
	virtual void NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted);

	/** 몽타주 재생 + Attacking CC 상태 부여 + 종료 시 NotifyAttackActionEnd 호출을 하나로 묶은 헬퍼.
	 *  기본 AttackByAI()는 AttackMontage로 이걸 호출하고, 보스처럼 상황에 따라 여러 몽타주 중
	 *  골라야 하는 경우 AttackByAI()를 오버라이드해서 원하는 몽타주로 이 헬퍼를 재사용하면 됨 */
	void PlayAttackMontage(UAnimMontage* Montage);

protected:
	void SeparateFromOtherMonsters(float DeltaSeconds);

	/** SeparateFromOtherMonsters()는 몬스터당 매 틱 OverlapMulti를 돌려서 비용이 큼(몹 수가 늘수록
	 *  O(n^2)에 가까워짐) - 몬스터마다 GetUniqueID() 기준으로 실행 프레임을 분산시켜, 이 값만큼의
	 *  프레임에 한 번씩만 계산하고 그동안 누적된 DeltaSeconds를 몰아서 넘김(이동 거리는 보존, 계산
	 *  빈도만 줄임). 타입별로 다르게 주고 싶으면(예: 보스는 매틱, 잡몹은 4~8프레임) BP에서 오버라이드 */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization", meta = (ClampMin = 1))
	int32 SeparationFrameInterval = 4;

public:
	FAICharacterAttackFinished OnAttackFinished;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMonsterDied OnMonsterDied;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	TObjectPtr<UCapsuleComponent> Collider;

	/** Draws GetCapsuleComponent()'s wireframe while the F1 debug widget's EnemyHitbox checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCPDebugCollisionShapeComponent> DebugHitboxShape;

	/** If true, AttackHitCheck draws its sweep shape. Driven by the F1 debug widget's MonsterAttackRange checkbox */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bDrawDebugAttackRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeadMontage;

	// BaseStatTable에서 이 값과 일치하는 RowName(Normal/Tanker/Ranged)의 스탯을 찾아옴
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	ECPMonsterType MonsterType = ECPMonsterType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	TObjectPtr<UCPMonsterStatComponent> StatComponent;

protected:
	/** Coin pickup spawned in the field on death (ACPCoinItem - walk-over auto-collect, distinct from the
	 *  physics-simulated ACPCoin used by the coin-pusher machine, which only ever gets collected by
	 *  falling into ACPDropZone and can't be picked up out in the field) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<ACPCoinItem> CoinItem;

protected:
	/** AttackHitCheck()의 스윕이 이번에 실제로 맞춘 액터(못 맞췄으면 nullptr, AttackHitCheck() 진입 시마다
	 *  초기화됨). Boss/Bomb처럼 "플레이어를 직접 맞췄는지"가 필요한 서브클래스가 Super::AttackHitCheck()
	 *  호출 직후 Cast<ACPPlayerCharacter>(LastAttackHitActor)로 확인하는 용도 - 넥서스를 맞췄거나
	 *  빗나간 경우와 구분하기 위해 필요함(스윕 자체는 대상을 가리지 않는 공용 로직이라서) */
	AActor* LastAttackHitActor = nullptr;

	static constexpr float KnockbackDuration = 0.2f;

	bool bIsDead = false;

	/** 체력이 0 이하가 된 순간부터, 실제 Dead()가 호출되기(KnockbackDuration 후) 전까지 true.
	 *  넉백은 공격자가 TakeDamage 직후 별도로 호출하는 구조라 Dead() 안에서 직접 틀 수 없어서,
	 *  대신 Dead() 호출 자체를 넉백 재생 시간만큼 미루는 방식으로 우회함 */
	bool bPendingDeath = false;

	ECPMonsterCCState CurrentCCState = ECPMonsterCCState::None;

	/** ApplyKnockback이 RVO/AI 이동을 복구할 때 쓰는 타이머 핸들. 멤버로 둬서, KnockbackDuration 안에
	 *  넉백이 연속으로 걸려도 이전 복구 타이머를 취소하고 새로 걸 수 있게 함(안 그러면 먼저 걸린 타이머가
	 *  나중 넉백을 중간에 취소시켜버림) */
	FTimerHandle KnockbackRestoreHandle;

	/** OnReturnedToPool()이 무브먼트를 끄기(DisableMovement) 직전의 MovementMode를 저장해뒀다가
	 *  OnAcquiredFromPool()에서 그대로 복원함 - 일반형은 MOVE_Walking, Ranged/Bomb 같은 비행형은
	 *  MOVE_Flying이라 값이 서로 다른데, 여기서 직접 값을 정하지 않고 껐던 값을 그대로 되돌리는
	 *  방식이라 서브클래스가 따로 오버라이드하지 않아도 됨 */
	TEnumAsByte<EMovementMode> PooledMovementMode = MOVE_Walking;

	/** BeginPlay()에서 한 번 캐시해둔 캡슐/메쉬의 원래 CollisionEnabled 값. Dead()가 사망 시 이 둘을
	 *  NoCollision으로 꺼버리므로, 풀에서 재사용될 때(OnAcquiredFromPool) 원래 값으로 복원하는 데 씀 */
	TEnumAsByte<ECollisionEnabled::Type> DefaultCapsuleCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	TEnumAsByte<ECollisionEnabled::Type> DefaultMeshCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
};
