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

	/** 매니저/스포너가 스폰 직후 호출: 지정한 웨이브 번호 기준으로 스탯을 다시 계산해 적용합니다.
	 *  (BeginPlay는 항상 1웨이브 기준으로 초기화하므로, 실제 웨이브에 맞춰 덮어쓸 때 사용) */
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ApplyWaveStat(int32 InWave);

	// Wave별
	virtual float GetAIMaxHealth() override;
	virtual float GetAICurrentHealth() override;
	virtual float GetAIMoveSpeed() override;
	virtual float GetAIAttackPower() override;

	// Default
	virtual float GetAIAttackSpeed() override;
	virtual float GetAICollisionRadius() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;
	virtual float GetAIMoveAcceptableRadius() override;

	// 몬스터 간 분리 - StatComponent->DefaultStat 기반 (RVO 회피 값 자체는 코드 상수로 고정 - BeginPlay 참고)
	virtual float GetAISeparationPadding() override;
	virtual float GetAISeparationSpeed() override;

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
};
