// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
#include "CPMonsterAIInterface.generated.h"

class UCPMonsterStatComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCPMonsterAIInterface : public UInterface
{
	GENERATED_BODY()
};

DECLARE_DELEGATE(FAICharacterAttackFinished);

/**
 *
 */
class CP_API ICPMonsterAIInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// FCPMonsterTemplate
	virtual ECPMonsterMoveType GetAIMoveType() = 0;
	virtual ECPMonsterAttackType GetAIAttackType() = 0;
	virtual FCPMonsterProjectileStat GetAIProjectileStat() = 0;

	// Wave별
	virtual float GetAIMaxHealth() = 0;
	virtual float GetAICurrentHealth() = 0;
	virtual float GetAIMoveSpeed() = 0;
	virtual float GetAIAttackPower() = 0;

	// Default
	virtual float GetAIAttackSpeed() = 0;
	virtual float GetAIKnockbackPower() = 0;
	virtual float GetAIDetectRange() = 0;
	virtual float GetAICollisionRadius() = 0;
	virtual float GetAIPatrolRadius() = 0;
	virtual float GetAIAttackRange() = 0;
	virtual float GetAITurnSpeed() = 0;

	/**
	 * 최대체력 / 이동방식 / 공격력 / 공격방식 / 공격속도 / 넉백 / 콜리전 / 투사체 등,
	 * 값이 늘어날 때마다 인터페이스 함수를 추가하는 대신 StatComponent 전체를 한 번에 참조하기 위한 통로.
	 * (개별 필드가 필요하면 여기서 얻은 StatComponent의 property를 직접 읽으면 됨)
	 */
	virtual UCPMonsterStatComponent* GetAIStatComponent() const = 0;

	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) = 0;
	virtual void AttackByAI() = 0;
};
