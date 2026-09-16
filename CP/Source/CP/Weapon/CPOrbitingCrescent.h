#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPOrbitingCrescent.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class ACharacter;

UCLASS(Blueprintable)
class CP_API ACPOrbitingCrescent : public AActor
{
	GENERATED_BODY()

public:

	ACPOrbitingCrescent();

	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> CrescentMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNiagaraComponent> CrescentEffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Effect")
	FVector CrescentEffectLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Effect")
	FRotator CrescentEffectRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Effect")
	FVector CrescentEffectScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Spawn Sound")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Spawn Sound")
	FVector SpawnSoundLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit|Spawn Sound", meta = (ClampMin = 0))
	float SpawnSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditAnywhere, Category="Orbit|Debug")
	bool bDrawDebugHitRadius = false;

	TWeakObjectPtr<ACharacter> OrbitOwner;

	float BaseOrbitAngleOffset = 0.0f;
	float OrbitRadius = 200.0f;
	float OrbitSpeedDegPerSec = 90.0f;
	float SelfSpinSpeedDegPerSec = 180.0f;
	UPROPERTY(EditAnywhere, Category = "Effect")
	float HitRadius = 80.0f;
	float VerticalOffset = 80.0f;
	float Damage = 15.0f;
	float KnockbackDistance = 300.0f;
	float EffectScaleMultiplier = 1.0f;

	TWeakObjectPtr<AController> InstigatorController;
	TWeakObjectPtr<AActor> DamageCauserActor;

	float CurrentOrbitAngleDegrees = 0.0f;
	float CurrentSelfSpinDegrees = 0.0f;

	FTimerHandle DamageTickTimerHandle;

public:

	void InitializeCrescent(ACharacter* InOwnerCharacter, float InOrbitRadius, float InOrbitSpeedDegPerSec, float InSelfSpinSpeedDegPerSec, float InHitRadius, float InVerticalOffset, float InBaseOrbitAngleOffset, float InDamage, float InKnockbackDistance, float InDamageTickInterval, float InEffectScaleMultiplier, AController* InInstigatorController, AActor* InDamageCauser);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void ApplyPulseDamage();

	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);
};
