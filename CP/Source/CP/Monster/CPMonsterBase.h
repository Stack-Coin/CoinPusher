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

	// 피격 함수 // 협업용
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// 넉백 함수 // 협업용
	virtual void ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor) override;

public:
	// StatComponent의 값을 참조
	virtual UCPMonsterStatComponent* GetAIStatComponent() const override;

	// Wave별
	virtual float GetAIMaxHealth() override;
	virtual float GetAICurrentHealth() override;
	virtual float GetAIMoveSpeed() override;
	virtual float GetAIAttackPower() override;

	// Default
	virtual float GetAIAttackSpeed() override;
	virtual float GetAIKnockbackPower() override;
	virtual float GetAIKnockbackDuration() override;
	virtual float GetAIKnockbackDistance() override;
	virtual float GetAIDetectRange() override;
	virtual float GetAICollisionRadius() override;
	virtual float GetAIPatrolRadius() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;
	virtual float GetAIMoveAcceptableRadius() override;

protected:
	virtual void NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted);

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

	// todo. Data Asset 형태로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterMesh")
	TObjectPtr<USkeletalMeshComponent> MonsterMesh;

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
	bool bIsDead = false;

protected:
	/** 넉백으로 뜨는 최종 수직 속도가 이 값을 넘지 않도록 clamp (현재 Z 속도 + KnockbackPower 위에 적용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Knockback")
	float MaxKnockbackZVelocity = 500.f;

	/** ApplyKnockback이 아주 짧은 시간 안에 다시 들어와도(예: 근접 공격 + 그 후속 폭발 모듈처럼 한 번의
	 *  공격에 넉백 판정이 여러 번 발생하는 경우) 다시 위로 붕 뜨지 않도록 무시하는 최소 재적용 간격(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Knockback")
	float KnockbackReapplyCooldown = 0.15f;

	/** 마지막으로 ApplyKnockback이 적용된 월드 시간(초). 초기값 -1은 "아직 한 번도 적용된 적 없음"을 의미 */
	float LastKnockbackTime = -1.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Separation")
	float SeparationPadding = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Separation")
	float SeparationSpeed = 400.f;
};
