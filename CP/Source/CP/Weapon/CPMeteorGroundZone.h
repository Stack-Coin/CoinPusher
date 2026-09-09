#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPMeteorGroundZone.generated.h"

class UNiagaraComponent;

UCLASS(Blueprintable)
class CP_API ACPMeteorGroundZone : public AActor
{
	GENERATED_BODY()

public:

	ACPMeteorGroundZone();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNiagaraComponent> ZoneEffect;

	UPROPERTY(EditAnywhere, Category="Ground Zone|Debug")
	bool bDrawDebugZoneRadius = false;

	float ZoneRadius = 300.0f;
	float DamagePerTick = 0.0f;
	float TickInterval = 0.5f;

	TWeakObjectPtr<AController> InstigatorController;
	TWeakObjectPtr<AActor> DamageCauserActor;

	FTimerHandle DamageTickTimerHandle;
	FTimerHandle DebugDrawTimerHandle;

public:

	void InitializeZone(float InZoneRadius, float InDamagePerTick, float InTickInterval, float InDuration, AController* InInstigatorController, AActor* InDamageCauser);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void ApplyZoneDamage();

	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);

	void SetDebugDrawEnabled(bool bEnabled);

	void DrawDebugZoneShape() const;
};
