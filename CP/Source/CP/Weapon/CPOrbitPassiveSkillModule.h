#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "CPOrbitPassiveSkillModule.generated.h"

class ACPOrbitingCrescent;

USTRUCT(BlueprintType)
struct FCPOrbitLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 SatelliteCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float OrbitRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "deg/s"))
	float OrbitSpeedDegPerSec = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "deg/s"))
	float SelfSpinSpeedDegPerSec = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float HitRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "cm"))
	float VerticalOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float Damage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01, Units = "s"))
	float DamageTickInterval = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float EffectScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "s"))
	float ChargeDurationPerActivation = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01, Units = "s"))
	float MaxDuration = 10.0f;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Orbiting Crescent"))
class CP_API UCPOrbitPassiveSkillModule : public UCPWeaponPassiveSkillModule
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) override;

	virtual int32 GetMaxLevel() const override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	TSubclassOf<ACPOrbitingCrescent> CrescentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Levels")
	TArray<FCPOrbitLevelData> LevelData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACPOrbitingCrescent>> ActiveCrescents;

	FTimerHandle GroupExpireTimerHandle;

	/** Weapon passed in via the last Activate()'s Context - kept around only to reach its GetWorldTimerManager()
	 *  from GetActiveDurationRemaining(), the same way Activate() itself uses Context.Weapon for GetWorld() */
	TWeakObjectPtr<ACPWeaponBase> CachedWeapon;

	/** LevelInfo.MaxDuration used by the last Activate() call - the "max time" pairing GetActiveDurationRemaining()'s
	 *  "current time" for a buff icon widget's UpdateBuff(Current, Max) */
	float ActiveMaxDuration = 0.0f;

protected:

	const FCPOrbitLevelData& GetLevelData(int32 Level) const;

	void SpawnCrescentGroup(const FCPPassiveSkillActivationContext& Context, const FCPOrbitLevelData& LevelInfo);

	void DespawnCrescentGroup();

public:

	/** Seconds left before the active crescent group despawns, or 0 if no group is currently active */
	UFUNCTION(BlueprintPure, Category="Orbit")
	float GetActiveDurationRemaining() const;

	/** MaxDuration used by the last Activate() call - pair with GetActiveDurationRemaining() to drive a
	 *  buff icon widget's UpdateBuff(Current, Max) */
	UFUNCTION(BlueprintPure, Category="Orbit")
	float GetActiveMaxDuration() const { return ActiveMaxDuration; }
};
