#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "CPMeteorPassiveSkillModule.generated.h"

class ACPMeteor;
class ACPMeteorGroundZone;
class ACPWeaponBase;
class ACharacter;

USTRUCT(BlueprintType)
struct FCPMeteorLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 MeteorCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "s"))
	float MeteorSpawnInterval = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float MeteorScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float EffectScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float DropRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float FallHeight = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1, Units = "cm/s"))
	float FallSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float ImpactRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float ImpactDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float ImpactKnockbackDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "s"))
	float GroundZoneDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float GroundZoneDamagePerTick = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01, Units = "s"))
	float GroundZoneTickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float GroundZoneRadius = 300.0f;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Meteor"))
class CP_API UCPMeteorPassiveSkillModule : public UCPWeaponPassiveSkillModule
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) override;

	virtual int32 GetMaxLevel() const override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor")
	TSubclassOf<ACPMeteor> MeteorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor")
	TSubclassOf<ACPMeteorGroundZone> GroundZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor|Levels")
	TArray<FCPMeteorLevelData> LevelData;

protected:

	const FCPMeteorLevelData& GetLevelData(int32 Level) const;

	FVector ComputeLandingLocation(const FVector& Center, UWorld* World, float DropRadius) const;

	void SpawnMeteor(TWeakObjectPtr<ACPWeaponBase> WeakWeapon, TWeakObjectPtr<ACharacter> WeakOwnerCharacter, TWeakObjectPtr<AActor> WeakDamageCauser, TWeakObjectPtr<AController> WeakInstigatorController, FVector Origin, FCPMeteorLevelData LevelInfo) const;
};
