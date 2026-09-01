// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Monster/CPMonsterAttackInterface.h"
#include "Monster/CPMonsterAIInterface.h"
#include "CPMonsterBase.generated.h"

/*
// todo. 공격 피격 테스트 + TakeDamage 구현됐는지 + Trace Channel 및 Collision Preset 설정
// todo. 사망 구현 및 사망 시 item drop
// todo. spawner

// todo. Knock Back
// todo. 체력바

// todo. mesh 겹침
// todo. 애니메이션
// todo. 오브젝트 풀링

// todo. 모듈화 (Stat Component 등...)
*/

UCLASS()
class CP_API ACPMonsterBase : public ACharacter, public ICPMonsterAttackInterface, public ICPMonsterAIInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACPMonsterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	// 공격 판정 함수
	virtual void AttackHitCheck() override;
	virtual void Dead();

public:
	// todo. Stat Component에서 구하기
	virtual float GetAIPatrolRadius() override;
	virtual float GetAIDetectRange() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;

	// 공격 함수 // BTTask에서 수행
	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) override;
	virtual void AttackByAI() override;

	FAICharacterAttackFinished OnAttackFinished;

	// 피격 함수 // 협업용
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	TObjectPtr<UCapsuleComponent> Collider;
	
	// todo. Data Asset 형태로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterMesh")
	TObjectPtr<USkeletalMeshComponent> MonsterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeadMontage;

	// todo. Data Table 형태로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHealth = 100.0f;
};
