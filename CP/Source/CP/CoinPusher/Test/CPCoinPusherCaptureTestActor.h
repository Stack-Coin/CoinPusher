// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherCaptureTestActor.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCPCoinPusherViewCaptureComponent;

/**
 *  Standalone stand-in for ACPCoinPusher used to test the SceneCaptureComponent2D Picture-in-Picture system
 *  (see CoinPusher/CPCoinPusherViewCaptureComponent.h, CPCoinPusherCaptureWidget.h and
 *  CPCoinPusherViewportClient.h) in isolation, without needing a fully set up ACPCoinPusher (dispensers,
 *  pusher, drop zone, etc). Drop one into a test level - DisplayMesh gives the capture camera something to
 *  look at, and ViewCaptureComponent creates its own render target in BeginPlay.
 *  ACPCoinPusherCaptureTestPlayerController is what finds this actor, pulls that render target out, and
 *  feeds it to the on-screen UCPCoinPusherCaptureWidget.
 */
UCLASS()
class CP_API ACPCoinPusherCaptureTestActor : public AActor
{
	GENERATED_BODY()

	/** Simple stand-in visual for the coin pusher machine, so the capture camera has something to show */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* DisplayMesh;

	/** ViewCaptureComponent를 붙여서 위치/각도를 잡아주는 SpringArm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* ViewCaptureBoom;

	/** Captures a view of DisplayMesh into its own render target (see GetViewCaptureComponent) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPCoinPusherViewCaptureComponent* ViewCaptureComponent;

public:

	ACPCoinPusherCaptureTestActor();

	FORCEINLINE UStaticMeshComponent* GetDisplayMesh() const { return DisplayMesh; }
	FORCEINLINE USpringArmComponent* GetViewCaptureBoom() const { return ViewCaptureBoom; }
	FORCEINLINE UCPCoinPusherViewCaptureComponent* GetViewCaptureComponent() const { return ViewCaptureComponent; }
};
