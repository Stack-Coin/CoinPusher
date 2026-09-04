// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPViewportHealthBarComponent.h"
#include "UI/CPHealthBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UCPViewportHealthBarComponent::UCPViewportHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCPViewportHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!WidgetClass)
	{
		return;
	}

	// Owner가 Pawn이고 로컬 PlayerController를 가지고 있으면 그 플레이어 화면에만 표시,
	// 아니라면(예: GameMode 등) 기본 뷰포트에 표시
	APlayerController* OwningController = nullptr;
	if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		OwningController = Cast<APlayerController>(OwningPawn->GetController());
	}
	else
	{
		OwningController = Cast<APlayerController>(GetOwner());
	}

	if (OwningController && OwningController->IsLocalController())
	{
		HealthBarWidget = CreateWidget<UCPHealthBarWidget>(OwningController, WidgetClass);
		if (HealthBarWidget)
		{
			HealthBarWidget->AddToPlayerScreen(ZOrder);
		}
	}
	else
	{
		HealthBarWidget = CreateWidget<UCPHealthBarWidget>(GetWorld(), WidgetClass);
		if (HealthBarWidget)
		{
			HealthBarWidget->AddToViewport(ZOrder);
		}
	}
}

void UCPViewportHealthBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealthBarWidget)
	{
		HealthBarWidget->RemoveFromParent();
		HealthBarWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UCPViewportHealthBarComponent::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (HealthBarWidget)
	{
		HealthBarWidget->UpdateHealth(CurrentHealth, MaxHealth);
	}
}
