// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPPartyCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 *  CPPartyCamera
 *  The single shared camera shown in place of per-player split-screen (see ACPGameMode::ToggleCameraMode).
 *  Every Tick, it follows the midpoint of every tracked actor (the local players) and smoothly zooms its
 *  spring arm out as they spread apart, and back in as they regroup. Ticks continuously rather than using
 *  a timer since it's tracking multiple moving pawns every frame, the same documented exception as
 *  ACPMonsterBase/ACPPusher/UCPRouletteWidget.
 */
UCLASS()
class CP_API ACPPartyCamera : public AActor
{
	GENERATED_BODY()

	/** Camera boom whose arm length is driven by the spread between TrackedActors */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** The actual camera. Set as the view target for every local player while in single-camera mode */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Spring arm length used when the tracked actors are at or below MinTrackedDistance apart (fully zoomed in) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0, Units = "cm"))
	float MinArmLength = 900.0f;

	/** Spring arm length used when the tracked actors are at or above MaxTrackedDistance apart (fully zoomed out) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0, Units = "cm"))
	float MaxArmLength = 2200.0f;

	/** Distance between the two farthest tracked actors at or below which the arm length is MinArmLength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0, Units = "cm"))
	float MinTrackedDistance = 200.0f;

	/** Distance between the two farthest tracked actors at or above which the arm length is MaxArmLength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0, Units = "cm"))
	float MaxTrackedDistance = 2500.0f;

	/** How quickly the spring arm interpolates toward its target length as the tracked actors move */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0))
	float ZoomInterpSpeed = 3.0f;

	/** How quickly this actor's location interpolates toward the tracked actors' midpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Zoom", meta = (ClampMin = 0))
	float FollowInterpSpeed = 5.0f;

	/** Actors this camera frames - the midpoint of these (typically the local players' pawns) is followed,
	 *  and the distance between the two farthest-apart of them drives the zoom. Set by ACPGameMode */
	TArray<TWeakObjectPtr<AActor>> TrackedActors;

public:

	/** Constructor */
	ACPPartyCamera();

	/** Gameplay initialization */
	virtual void Tick(float DeltaTime) override;

	/** Replaces the set of actors this camera follows/frames (typically the local players' pawns) */
	void SetTrackedActors(const TArray<TWeakObjectPtr<AActor>>& InTrackedActors) { TrackedActors = InTrackedActors; }

	/** Returns FollowCamera subobject, e.g. to set as a PlayerController's view target */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
