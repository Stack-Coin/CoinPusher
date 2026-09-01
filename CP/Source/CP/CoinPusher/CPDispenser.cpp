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
	if (!ItemClass || !GetWorld())
	{
		return;
	}

	//ICPCoinPusherItem을 구현하지 않는 클래스는 DropZone이 처리할 수 없으므로 스폰하지 않는다
	if (!ItemClass->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(ItemClass, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentRotation(), SpawnParams))
	{
		//RootComponent가 물리 시뮬레이션 중인 프리미티브라면 발사 속도를 부여 (ACPCoin, ACPItem 공통)
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(SpawnedItem->GetRootComponent()))
		{
			const FVector LaunchVelocity = SpawnPoint->GetForwardVector() * LaunchForwardSpeed + FVector::UpVector * LaunchUpwardSpeed;
			RootPrimitive->SetPhysicsLinearVelocity(LaunchVelocity);
		}
	}
}

void ACPDispenser::DispenseItems(int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		DispenseItem();
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
