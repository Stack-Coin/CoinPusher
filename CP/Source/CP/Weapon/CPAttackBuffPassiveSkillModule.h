#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "CPAttackBuffPassiveSkillModule.generated.h"

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FCPAttackBuffLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float AttackPowerBonus = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float RangeMultiplierBonus = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float AttackSpeedMultiplierBonus = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float EffectScale = 1.0f;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Attack Buff"))
class CP_API UCPAttackBuffPassiveSkillModule : public UCPWeaponPassiveSkillModule
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) override;

	virtual int32 GetMaxLevel() const override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff|Levels")
	TArray<FCPAttackBuffLevelData> LevelData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff", meta = (ClampMin = 0.01, Units = "s"))
	float Duration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff|Effect")
	TObjectPtr<UNiagaraSystem> BuffEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff|Effect")
	FVector BuffEffectLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff|Effect")
	FRotator BuffEffectRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Buff|Effect")
	FVector BuffEffectScale = FVector(1.0f, 1.0f, 1.0f);

protected:

	const FCPAttackBuffLevelData& GetLevelData(int32 Level) const;
};
