// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPDropZone.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinCollected, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCollected, FName, ItemCode);


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

	//수집한 아이템들의 Item 코드 (수집한 순서대로 기록)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drop Zone")
	TArray<FName> CollectedItemCodes;

public:

	//Coin 수집 시 BroadCast
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnCoinCollected OnCoinCollected;

	//Item 수집 시 BroadCast
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnItemCollected OnItemCollected;

	//수집한 동전 개수 반환
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	int32 GetCollectedCoinCount() const { return CollectedCoinCount; }

	//수집한 아이템 코드 목록 반환
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	const TArray<FName>& GetCollectedItemCodes() const { return CollectedItemCodes; }

	//ICPCoinPusherItem 구현체(Coin)가 호출 - 동전 개수를 올리고 BroadCast
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void AddCollectedCoins(int32 Amount = 1);

	//ICPCoinPusherItem 구현체(Item)가 호출 - 아이템 코드를 기록하고 BroadCast
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void RecordCollectedItem(FName ItemCode);

protected:

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	FORCEINLINE UBoxComponent* GetCollectionVolume() const { return CollectionVolume; }
};
