// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherItem.h"
#include "CPCoin.generated.h"

class UStaticMeshComponent;

UCLASS(abstract)
class CP_API ACPCoin : public AActor, public ICPCoinPusherItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	ACPCoin();

protected:

	//Coin 가치
	UPROPERTY(EditAnywhere, Category="Coin")
	float CoinValue = 1.0f;

	//Coin의 Item ID. ItemRegistry / Dispenser의 ItemID와 동일한 개념의 식별자
	UPROPERTY(EditAnywhere, Category="Coin")
	FName ItemID;

	bool bCollected = false;

public:

	//Coin 가치 반환
	UFUNCTION(BlueprintPure, Category="Coin")
	float GetCoinValue() const { return CoinValue; }

	//Coin의 Item ID 반환
	UFUNCTION(BlueprintPure, Category="Coin")
	FName GetItemID() const { return ItemID; }

	//코인 발사
	UFUNCTION(BlueprintCallable, Category="Coin")
	void Launch(const FVector& LaunchVelocity);

	//DropZone에 떨어졌을 때 호출
	UFUNCTION(BlueprintCallable, Category="Coin")
	void Collect();

	// ~begin ICPCoinPusherItem
	//DropZone의 코인 개수를 올린 뒤 Collect() 호출
	virtual void OnDroppedInZone(ACPDropZone* DropZone) override;
	// ~end ICPCoinPusherItem

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Coin", meta = (DisplayName = "On Collected"))
	void BP_OnCollected();

public:

	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
};
