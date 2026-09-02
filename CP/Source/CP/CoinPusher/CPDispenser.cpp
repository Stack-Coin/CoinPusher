// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDispenser.h"
#include "CPCoinPusherItem.h"
#include "CPInput.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

ACPDispenser::ACPDispenser()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetCollisionProfileName(FName("BlockAllDynamic"));
	Body->bNavigationRelevant = false;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
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

void ACPDispenser::DispenseItemByID(FName ItemID, int32 SpawnCount, ECPDispenserSpawnType SpawnType, bool bLaunch)
{
	if (!ItemRegistry)
	{
		return;
	}

	const TSubclassOf<AActor> ClassToSpawn = ItemRegistry->GetItemClass(ItemID, SpawnType);
	if (!ClassToSpawn)
	{
		return;
	}

	//CoinPusherItem 타입은 DropZone이 처리할 수 있도록 ICPCoinPusherItem을 구현해야 함.
	//WorldItem 타입은 별도의 상호작용 시스템(ICPInteractable/ICPInteractor)을 쓰므로 검사하지 않는다
	if (SpawnType == ECPDispenserSpawnType::CoinPusherItem && !ClassToSpawn->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		return;
	}

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		SpawnItemClass(ClassToSpawn, bLaunch);
	}
}

void ACPDispenser::SpawnItemClass(TSubclassOf<AActor> ClassToSpawn, bool bLaunch)
{
	if (!ClassToSpawn || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentRotation(), SpawnParams))
	{
		if (bLaunch)
		{
			//RootComponent가 물리 시뮬레이션 중인 프리미티브라면 발사 속도를 부여 (ACPCoin, ACPItem 공통)
			if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(SpawnedItem->GetRootComponent()))
			{
				const FVector LaunchVelocity = SpawnPoint->GetForwardVector() * LaunchForwardSpeed + FVector::UpVector * LaunchUpwardSpeed;
				RootPrimitive->SetPhysicsLinearVelocity(LaunchVelocity);
			}
		}
	}
}

void ACPDispenser::SetLinkedInput(ACPInput* NewLinkedInput)
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
