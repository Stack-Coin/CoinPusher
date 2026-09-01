// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPDropZone.generated.h"

class UBoxComponent;
class ACPCoin;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinCollected, ACPCoin*, Coin, int32, NewCount);


UCLASS(abstract)
class CP_API ACPDropZone : public AActor
{
	GENERATED_BODY()

	//Trigger 충돌 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollectionVolume;

public:

	ACPDropZone();

protected:

	//수집한 동전 개수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drop Zone")
	int32 CollectedCoinCount = 0;

public:

	//Coin 수집 시 BroadCast
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnCoinCollected OnCoinCollected;

	//수집한 동전 개수
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	int32 GetCollectedCoinCount() const { return CollectedCoinCount; }

protected:

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	FORCEINLINE UBoxComponent* GetCollectionVolume() const { return CollectionVolume; }
};
