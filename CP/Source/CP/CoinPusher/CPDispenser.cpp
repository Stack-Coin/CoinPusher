// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDispenser.h"
#include "CPCoinPusherItem.h"
#include "CPCoin.h"
#include "CPItem.h"
//#include "CPInput.h"
#include "../Nexus/CPNexus.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Log/CPLogCategories.h"

ACPDispenser::ACPDispenser()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->bNavigationRelevant = false;
}

void ACPDispenser::BeginPlay()
{
	Super::BeginPlay();

	//Input에 Delegate에 등록 (ACPCoinPusher가 미리 SetLinkedInput()으로 연결해 둔 경우 여기서는 이미 바인딩되어 있음)
	BindToLinkedInput();
}

void ACPDispenser::DispenseItem()
{
	//ICPCoinPusherItem을 구현하지 않는 클래스는 DropZone이 처리할 수 없으므로 스폰하지 않는다
	if (!ItemClass || !ItemClass->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		return;
	}

	SpawnItemClass(ItemClass, /*bLaunch=*/true);
}

void ACPDispenser::DispenseItems(int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		DispenseItem();
	}
}

void ACPDispenser::DispenseItemByID(FName ItemID, int32 SpawnCount, bool bLaunch)
{
	const FItemData* Row = FindItemData(ItemID);
	if (!Row)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] DispenseItemByID 무시됨(%s) - ItemDataTable(%s)에서 ItemID '%s' 행을 찾지 못했습니다."),
			*GetName(), ItemDataTable ? *ItemDataTable->GetName() : TEXT("null"), *ItemID.ToString());
		return;
	}

	if (!Row->CoinPusherSpawnBPClass)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] DispenseItemByID 무시됨(%s) - ItemID '%s' 행의 CoinPusherSpawnBPClass가 지정되지 않았습니다."),
			*GetName(), *ItemID.ToString());
		return;
	}

	//ICPCoinPusherItem을 구현하지 않는 클래스는 DropZone이 처리할 수 없으므로 스폰하지 않는다
	if (!Row->CoinPusherSpawnBPClass->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] DispenseItemByID 무시됨(%s) - ItemID '%s' 행의 CoinPusherSpawnBPClass(%s)가 ICPCoinPusherItem을 구현하지 않습니다."),
			*GetName(), *ItemID.ToString(), *Row->CoinPusherSpawnBPClass->GetName());
		return;
	}

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		AActor* SpawnedActor = SpawnFromItemData(ItemID, *Row, Row->CoinPusherSpawnBPClass, bLaunch);
		if (!SpawnedActor)
		{
			UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] DispenseItemByID 스폰 실패(%s) - ItemID '%s'(%d/%d), Class: %s"),
				*GetName(), *ItemID.ToString(), Index + 1, SpawnCount, *Row->CoinPusherSpawnBPClass->GetName());
		}
		else
		{
			UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] DispenseItemByID 스폰 성공(%s) - ItemID '%s'(%d/%d), Actor: %s"),
				*GetName(), *ItemID.ToString(), Index + 1, SpawnCount, *SpawnedActor->GetName());
		}
	}
}

ACPCoin* ACPDispenser::DispenseCoinByID(FName ItemID, bool bLaunch)
{
	const FItemData* Row = FindItemData(ItemID);
	if (!Row || !Row->CoinPusherSpawnBPClass || !Row->CoinPusherSpawnBPClass->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		return nullptr;
	}

	return Cast<ACPCoin>(SpawnFromItemData(ItemID, *Row, Row->CoinPusherSpawnBPClass, bLaunch));
}

const FItemData* ACPDispenser::FindItemData(FName ItemID) const
{
	if (!ItemDataTable)
	{
		return nullptr;
	}

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ACPDispenser::FindItemData"));
}

AActor* ACPDispenser::SpawnFromItemData(FName ItemID, const FItemData& Row, TSubclassOf<AActor> ClassToSpawn, bool bLaunch)
{
	AActor* SpawnedActor = SpawnItemClass(ClassToSpawn, bLaunch);
	if (!SpawnedActor)
	{
		return nullptr;
	}

	//Category가 "Coin"인 행이면, 실제로 스폰된 액터가 ACPCoin일 때만 행에 지정된 CoinType을 적용
	if (Row.Category == FName("Coin"))
	{
		if (ACPCoin* SpawnedCoin = Cast<ACPCoin>(SpawnedActor))
		{
			SpawnedCoin->SetCoinType(Row.CoinType);
		}
	}

	//실제로 ACPItem이면 BP Class Defaults에 고정된 ItemId 대신, 지금 스폰을 요청한 실제 ItemID로
	//갱신해야 Mesh/Material/Image가 이 ItemID에 맞게 나온다 (안 그러면 화면에 다른 아이템의 모습으로
	//나오거나, 해당 ItemID 행에 시각 정보가 없으면 아예 안 보이게 됨)
	if (ACPItem* SpawnedItem = Cast<ACPItem>(SpawnedActor))
	{
		SpawnedItem->SetItemId(ItemID);
	}

	return SpawnedActor;
}

AActor* ACPDispenser::SpawnItemClass(TSubclassOf<AActor> ClassToSpawn, bool bLaunch)
{
	if (!ClassToSpawn || !GetWorld())
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] SpawnItemClass 실패(%s) - ClassToSpawn: %s, World: %s"),
			*GetName(), ClassToSpawn ? *ClassToSpawn->GetName() : TEXT("null"), GetWorld() ? TEXT("OK") : TEXT("null"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentRotation(), SpawnParams);
	if (!SpawnedItem)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPDispenser] SpawnItemClass 실패(%s) - SpawnActor가 %s를 생성하지 못했습니다."),
			*GetName(), *ClassToSpawn->GetName());
		return nullptr;
	}

	if (bLaunch)
	{
		//RootComponent가 물리 시뮬레이션 중인 프리미티브라면 발사 속도를 부여 (ACPCoin, ACPItem 공통)
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(SpawnedItem->GetRootComponent()))
		{
			const FVector LaunchVelocity = SpawnPoint->GetForwardVector() * LaunchForwardSpeed + FVector::UpVector * LaunchUpwardSpeed;
			RootPrimitive->SetPhysicsLinearVelocity(LaunchVelocity);
		}
	}

	return SpawnedItem;
}

void ACPDispenser::SetLinkedInput(ACPNexus* NewLinkedInput)
{
	if (LinkedInput == NewLinkedInput)
	{
		return;
	}

	//기존에 연결되어 있던 Input의 델리게이트는 해제
	if (LinkedInput)
	{
		LinkedInput->OnInteracted.RemoveDynamic(this, &ACPDispenser::HandleInputInteracted);
	}

	LinkedInput = NewLinkedInput;

	BindToLinkedInput();
}

void ACPDispenser::HandleInputInteracted(AActor* Interactor)
{
	DispenseItem();
}

void ACPDispenser::BindToLinkedInput()
{
	if (LinkedInput)
	{
		LinkedInput->OnInteracted.AddUniqueDynamic(this, &ACPDispenser::HandleInputInteracted);
	}
}
