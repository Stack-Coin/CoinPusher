// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPDebugCollisionShapeComponent.generated.h"

class UPrimitiveComponent;

/**
 *  UCPDebugCollisionShapeComponent
 *  Reusable building block for the F1 debug widget's collision checkboxes. Attach to any actor that owns
 *  a Capsule/Sphere collision component relevant to one of ECPDebugCollisionCategory's categories, and
 *  call SetTargetComponent from the owner's constructor once that component exists. From then on this
 *  subscribes to UCPDebugCollisionSubsystem and redraws the target's wireframe on a short repeating timer
 *  while its Category is toggled on - the same redraw-on-a-timer approach ACPPlayerCharacter's revive range
 *  and ACPProjectile's collision debug draw already use. Purely visual - never touches actual collision.
 */
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent))
class CP_API UCPDebugCollisionShapeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCPDebugCollisionShapeComponent();

	/** Which F1 checkbox toggles this shape's debug draw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	ECPDebugCollisionCategory Category = ECPDebugCollisionCategory::PlayerHitbox;

	/** Color the shape is drawn in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	FColor ShapeColor = FColor::Green;

	/** Assigns the Capsule/Sphere component this instance draws. Must be called by the owner's constructor,
	 *  after the target component itself has been created */
	void SetTargetComponent(UPrimitiveComponent* InTarget) { TargetComponent = InTarget; }

protected:

	/** Capsule or Sphere component this instance draws a debug wireframe for */
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> TargetComponent;

	/** Redraws TargetComponent's shape at its current location. Runs on DrawTimerHandle while enabled */
	FTimerHandle DrawTimerHandle;

	/** Subscribes to UCPDebugCollisionSubsystem and applies its current state for Category */
	virtual void BeginPlay() override;

	/** Stops DrawTimerHandle */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Bound to UCPDebugCollisionSubsystem::OnCollisionVisibilityChanged */
	UFUNCTION()
	void HandleCollisionVisibilityChanged(ECPDebugCollisionCategory ChangedCategory, bool bVisible);

	/** Starts/stops the repeating DrawShape timer */
	void SetDrawEnabled(bool bEnabled);

	/** Draws TargetComponent's current shape (Capsule or Sphere) as a debug wireframe */
	void DrawShape() const;
};
