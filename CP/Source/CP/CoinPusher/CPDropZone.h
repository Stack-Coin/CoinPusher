// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPDropZone.generated.h"

class UBoxComponent;
class ACPDispenser;

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

	//아이템이 떨어졌을 때 재생성을 요청할 Dispenser. DropZone은 ACPCoinPusher의 ChildActorComponent로
	//스폰되므로 레벨에서 직접 편집하지 않고, 소유자인 ACPCoinPusher가 SetItemRespawnDispenser()를
	//통해 설정한다
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Drop Zone")
	TObjectPtr<ACPDispenser> ItemRespawnDispenser;

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

	//ICPCoinPusherItem 구현체(Item)가 호출 - 아이템 코드를 기록하고 BroadCast + ItemRespawnDispenser에 재생성 요청
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void RecordCollectedItem(FName ItemCode);

	//소유자(ACPCoinPusher)가 호출 - 아이템 드롭 시 재생성을 맡을 Dispenser를 설정
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void SetItemRespawnDispenser(ACPDispenser* NewItemRespawnDispenser);

protected:

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	FORCEINLINE UBoxComponent* GetCollectionVolume() const { return CollectionVolume; }

	FORCEINLINE ACPDispenser* GetItemRespawnDispenser() const { return ItemRespawnDispenser; }
};
