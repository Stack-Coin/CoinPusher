// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPItemRegistry.h"

void UCPItemRegistry::RegisterItem(FName ItemID, TSubclassOf<AActor> CoinPusherItemClass, TSubclassOf<AActor> WorldItemClass)
{
	if (ItemID.IsNone())
	{
		return;
	}

	FCPDispenserItemEntry& Entry = Items.FindOrAdd(ItemID);
	Entry.CoinPusherItemClass = CoinPusherItemClass;
	Entry.WorldItemClass = WorldItemClass;
}

TSubclassOf<AActor> UCPItemRegistry::GetItemClass(FName ItemID, ECPDispenserSpawnType SpawnType) const
{
	const FCPDispenserItemEntry* Entry = Items.Find(ItemID);
	if (!Entry)
	{
		return nullptr;
	}

	return (SpawnType == ECPDispenserSpawnType::WorldItem) ? Entry->WorldItemClass : Entry->CoinPusherItemClass;
}
