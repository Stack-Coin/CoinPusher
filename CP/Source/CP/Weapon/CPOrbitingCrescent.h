#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPOrbitingCrescent.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orbit")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditAnywhere, Category="Orbit|Debug")
	bool bDrawDebugHitRadius = false;

	TWeakObjectPtr<ACharacter> OrbitOwner;

	float OrbitRadius = 200.0f;
	float OrbitSpeedDegPerSec = 90.0f;
	float SelfSpinSpeedDegPerSec = 180.0f;
	float HitRadius = 80.0f;
	float VerticalOffset = 80.0f;
	float Damage = 15.0f;
	float KnockbackDistance = 300.0f;

	TWeakObjectPtr<AController> InstigatorController;
	TWeakObjectPtr<AActor> DamageCauserActor;

	float CurrentOrbitAngleDegrees = 0.0f;
	float CurrentSelfSpinDegrees = 0.0f;

	FTimerHandle DamageTickTimerHandle;

public:

	void InitializeCrescent(ACharacter* InOwnerCharacter, float InOrbitRadius, float InOrbitSpeedDegPerSec, float InSelfSpinSpeedDegPerSec, float InHitRadius, float InVerticalOffset, float InDuration, float InDamage, float InKnockbackDistance, float InDamageTickInterval, AController* InInstigatorController, AActor* InDamageCauser);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void ApplyPulseDamage();

	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);
};
