#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPMeteor.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class ACPMeteorGroundZone;

UCLASS(Blueprintable)
class CP_API ACPMeteor : public AActor
{
	GENERATED_BODY()

public:

	ACPMeteor();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeteorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> MeteorMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meteor")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, Category="Meteor|Debug")
	bool bDrawDebugImpactRadius = false;

	FVector LandingLocation = FVector::ZeroVector;
	float ImpactRadius = 300.0f;
	float ImpactDamage = 0.0f;
	float ImpactKnockbackDistance = 400.0f;

	TSubclassOf<ACPMeteorGroundZone> GroundZoneClass;
	float GroundZoneDuration = 0.0f;
	float GroundZoneDamagePerTick = 0.0f;
	float GroundZoneTickInterval = 0.5f;
	float GroundZoneRadius = 300.0f;

	TWeakObjectPtr<AController> InstigatorController;
	TWeakObjectPtr<AActor> DamageCauserActor;

	FTimerHandle ImpactTimerHandle;
	FTimerHandle DebugDrawTimerHandle;

public:

	void InitializeMeteor(const FVector& InLandingLocation, float FallSpeed, float InImpactRadius, float InImpactDamage, float InImpactKnockbackDistance, TSubclassOf<ACPMeteorGroundZone> InGroundZoneClass, float InGroundZoneDuration, float InGroundZoneDamagePerTick, float InGroundZoneTickInterval, float InGroundZoneRadius, AController* InInstigatorController, AActor* InDamageCauser);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void Impact();

	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);

	void SetDebugDrawEnabled(bool bEnabled);

	void DrawDebugImpactShape() const;
};
