#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "CPOrbitPassiveSkillModule.generated.h"

class ACPOrbitingCrescent;

UCLASS(EditInlineNew, meta = (DisplayName = "Orbiting Crescent"))
class CP_API UCPOrbitPassiveSkillModule : public UCPWeaponPassiveSkillModule
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	TSubclassOf<ACPOrbitingCrescent> CrescentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit", meta = (ClampMin = 0, Units = "cm"))
	float OrbitRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	float OrbitSpeedDegPerSec = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	float SelfSpinSpeedDegPerSec = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit", meta = (ClampMin = 0, Units = "cm"))
	float HitRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit", meta = (Units = "cm"))
	float VerticalOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit", meta = (ClampMin = 0, Units = "s"))
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Damage", meta = (ClampMin = 0))
	float Damage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Damage", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Damage", meta = (ClampMin = 0.01, Units = "s"))
	float DamageTickInterval = 0.3f;
};
