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
	// 공격 판정 함수
	virtual void AttackHitCheck() override;
	virtual void Dead();

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

protected:
	virtual void NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted);

	/** 몽타주 재생 + Attacking CC 상태 부여 + 종료 시 NotifyAttackActionEnd 호출을 하나로 묶은 헬퍼.
	 *  기본 AttackByAI()는 AttackMontage로 이걸 호출하고, 보스처럼 상황에 따라 여러 몽타주 중
	 *  골라야 하는 경우 AttackByAI()를 오버라이드해서 원하는 몽타주로 이 헬퍼를 재사용하면 됨 */
	void PlayAttackMontage(UAnimMontage* Montage);

protected:
	void SeparateFromOtherMonsters(float DeltaSeconds);

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

	// [임시 디버그] Tick 생존 확인용 로그 스로틀 (인스턴스별로 따로 누적되어야 해서 static 지역변수 대신 멤버로 둠)
	float DebugTickLogAccum = 0.f;
};
