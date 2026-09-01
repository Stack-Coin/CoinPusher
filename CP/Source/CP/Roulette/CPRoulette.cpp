// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPRoulette.h"
#include "CPRouletteWidget.h"
#include "../CoinPusher/CPCoinPusherItem.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACPRoulette::ACPRoulette()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));

	Slots.SetNum(NumSlots);
}

void ACPRoulette::Roll()
{
	if (Slots.Num() == 0)
	{
		return;
	}

	const int32 ResultIndex = FMath::RandRange(0, Slots.Num() - 1);

	if (UCPRouletteWidget* Widget = GetOrCreateRouletteWidget())
	{
		Widget->PlaySpin(ResultIndex, Slots.Num());
	}
	else
	{
		// UI 없이도 결과 처리는 그대로 동작하도록 하는 폴백
		HandleRouletteResultDetermined(ResultIndex);
	}
}

UCPRouletteWidget* ACPRoulette::GetOrCreateRouletteWidget()
{
	if (RouletteWidgetInstance)
	{
		return RouletteWidgetInstance;
	}

	if (!RouletteWidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return nullptr;
	}

	RouletteWidgetInstance = CreateWidget<UCPRouletteWidget>(PC, RouletteWidgetClass);
	if (RouletteWidgetInstance)
	{
		RouletteWidgetInstance->AddToViewport();
		RouletteWidgetInstance->OnResultDetermined.AddUniqueDynamic(this, &ACPRoulette::HandleRouletteResultDetermined);
	}

	return RouletteWidgetInstance;
}

void ACPRoulette::HandleRouletteResultDetermined(int32 ResultIndex)
{
	if (!Slots.IsValidIndex(ResultIndex))
	{
		return;
	}

	SpawnSlotItems(Slots[ResultIndex]);
}

void ACPRoulette::SpawnSlotItems(const FCPRouletteSlotData& SlotData)
{
	if (!SlotData.ItemClass || !GetWorld())
	{
		return;
	}

	// ICPCoinPusherItem을 구현하지 않는 클래스는 DropZone이 처리할 수 없으므로 스폰하지 않는다
	if (!SlotData.ItemClass->ImplementsInterface(UCPCoinPusherItem::StaticClass()))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 Index = 0; Index < SlotData.SpawnCount; ++Index)
	{
		if (AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(SlotData.ItemClass, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentRotation(), SpawnParams))
		{
			// RootComponent가 물리 시뮬레이션 중인 프리미티브라면 발사 속도를 부여 (ACPCoin, ACPItem 등)
			if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(SpawnedItem->GetRootComponent()))
			{
				const FVector LaunchVelocity = SpawnPoint->GetForwardVector() * LaunchForwardSpeed + FVector::UpVector * LaunchUpwardSpeed;
				RootPrimitive->SetPhysicsLinearVelocity(LaunchVelocity);
			}
		}
	}
}
