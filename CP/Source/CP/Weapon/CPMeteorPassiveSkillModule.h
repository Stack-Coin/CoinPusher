#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "CPMeteorPassiveSkillModule.generated.h"

class ACPMeteor;
class ACPMeteorGroundZone;

UCLASS(EditInlineNew, meta = (DisplayName = "Meteor"))
class CP_API UCPMeteorPassiveSkillModule : public UCPWeaponPassiveSkillModule
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor")
	TSubclassOf<ACPMeteor> MeteorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor", meta = (ClampMin = 0, Units = "cm"))
	float DropRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor", meta = (ClampMin = 0, Units = "cm"))
	float FallHeight = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor", meta = (ClampMin = 1, Units = "cm/s"))
	float FallSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Impact", meta = (ClampMin = 0, Units = "cm"))
	float ImpactRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Impact", meta = (ClampMin = 0))
	float ImpactDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Impact", meta = (ClampMin = 0, Units = "cm"))
	float ImpactKnockbackDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Ground Zone")
	TSubclassOf<ACPMeteorGroundZone> GroundZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Ground Zone", meta = (ClampMin = 0, Units = "s"))
	float GroundZoneDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Ground Zone", meta = (ClampMin = 0))
	float GroundZoneDamagePerTick = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Ground Zone", meta = (ClampMin = 0.01, Units = "s"))
	float GroundZoneTickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Ground Zone", meta = (ClampMin = 0, Units = "cm"))
	float GroundZoneRadius = 300.0f;

protected:

	FVector ComputeLandingLocation(const FVector& Center, UWorld* World) const;
};
