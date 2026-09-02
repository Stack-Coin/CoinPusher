// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CPItemRegistry.generated.h"

//생성할 오브젝트의 종류
UENUM(BlueprintType)
enum class ECPDispenserSpawnType : uint8
{
	//ICPCoinPusherItem을 구현하는 클래스 (ACPCoin, ACPItem 등). DropZone에 떨어져 처리됨
	CoinPusherItem,
	//ACPWorldItem(또는 그 서브클래스). 플레이어가 직접 상호작용해 습득
	WorldItem
};

//하나의 ItemID에 대해 스폰 방식(CoinPusherItem/WorldItem)별로 생성할 클래스를 가짐
USTRUCT(BlueprintType)
struct FCPDispenserItemEntry
{
	GENERATED_BODY()

	//ICPCoinPusherItem을 구현하는 클래스 (ACPCoin, ACPItem 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Registry")
	TSubclassOf<AActor> CoinPusherItemClass;

	//ACPWorldItem(또는 그 서브클래스)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Registry")
	TSubclassOf<AActor> WorldItemClass;
};

/**
 *  ItemID + 스폰 방식(CoinPusherItem/WorldItem)만 주어지면 생성할 클래스를 반환하는 데이터 에셋.
 *  ACPDispenser 등 아이템을 스폰할 수 있는 어떤 클래스든 이 에셋 하나를 참조해서 재사용할 수 있다.
 *  Items는 BP(에셋 디폴트)에서 직접 채워 넣거나, RegisterItem()으로 런타임에 등록할 수 있다.
 */
UCLASS(BlueprintType)
class CP_API UCPItemRegistry : public UDataAsset
{
	GENERATED_BODY()

protected:

	//ItemID 키 : 생성할 클래스 목록(Value)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Registry")
	TMap<FName, FCPDispenserItemEntry> Items;

public:

	//ItemID에 대한 항목을 등록(이미 있으면 덮어씀). BP 그래프에서 런타임에 등록할 때 사용
	UFUNCTION(BlueprintCallable, Category="Item Registry")
	void RegisterItem(FName ItemID, TSubclassOf<AActor> CoinPusherItemClass, TSubclassOf<AActor> WorldItemClass);

	//ItemID + SpawnType에 해당하는 클래스를 반환. 등록되어 있지 않으면 nullptr
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item Registry")
	TSubclassOf<AActor> GetItemClass(FName ItemID, ECPDispenserSpawnType SpawnType) const;
};
