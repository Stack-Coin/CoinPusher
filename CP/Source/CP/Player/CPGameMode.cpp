// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Monster/Spawner/CPUserWidget_WaveStatus.h"
#include "Player/CPPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "UI/CPHealthBarWidget.h"
#include "UI/CPTicketCountWidget.h"
#include "UI/CPCoinCountWidget.h"
#include "UI/CPRadialGaugeComponent.h"
#include "UI/CPInventoryWidget.h"

ACPGameMode::ACPGameMode()
{
	// stub
}

void ACPGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(PC->GetPawn()))
		{
			AttachReviveGaugeToPlayer(PlayerCharacter);
			SetupPlayerHealthBarWidget(PlayerCharacter, PlayerHealthBarWidgetClass);
			SetupPlayerWalletWidgets(PlayerCharacter);
			SetupPlayerInventoryWidget(PlayerCharacter);
		}
	}
}

AActor* ACPGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	static const FName PlayerTag(TEXT("Player0"));

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	if (!PlayerStarts.IsEmpty())
	{
		return PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
	}

	return nullptr;
}

void ACPGameMode::SetupPlayerWalletWidgets(ACPPlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		return;
	}

	if (TicketWidgetClass)
	{
		if (UCPTicketCountWidget* TicketWidget = CreateWidget<UCPTicketCountWidget>(GetWorld(), TicketWidgetClass))
		{
			TicketWidget->AddToViewport();
			PlayerCharacter->OnTicketChanged.AddDynamic(TicketWidget, &UCPTicketCountWidget::UpdateTicketCount);
			TicketWidget->UpdateTicketCount(PlayerCharacter->GetTicketCount());
		}
	}

	if (CoinWidgetClass)
	{
		if (UCPCoinCountWidget* CoinWidget = CreateWidget<UCPCoinCountWidget>(GetWorld(), CoinWidgetClass))
		{
			CoinWidget->AddToViewport();
			PlayerCharacter->OnCoinChanged.AddDynamic(CoinWidget, &UCPCoinCountWidget::UpdateCoinCount);
			CoinWidget->UpdateCoinCount(PlayerCharacter->GetCoinAmount());
		}
	}
}

void ACPGameMode::SetupPlayerHealthBarWidget(ACPPlayerCharacter* PlayerCharacter, TSubclassOf<UCPHealthBarWidget> HealthBarWidgetClass)
{
	if (!PlayerCharacter || !HealthBarWidgetClass)
	{
		return;
	}

	APlayerController* OwningController = Cast<APlayerController>(PlayerCharacter->GetController());
	if (!OwningController)
	{
		return;
	}

	UCPHealthBarWidget* HealthBarWidget = CreateWidget<UCPHealthBarWidget>(OwningController, HealthBarWidgetClass);
	if (!HealthBarWidget)
	{
		return;
	}

	HealthBarWidget->AddToViewport();

	PlayerCharacter->OnHealthChanged.AddDynamic(HealthBarWidget, &UCPHealthBarWidget::UpdateHealth);
	HealthBarWidget->UpdateHealth(PlayerCharacter->GetStat(ECPStatType::Health), PlayerCharacter->GetMaxHealth());
}

void ACPGameMode::SetupPlayerInventoryWidget(ACPPlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !InventoryWidgetClass)
	{
		return;
	}

	APlayerController* OwningController = Cast<APlayerController>(PlayerCharacter->GetController());
	if (!OwningController)
	{
		return;
	}

	if (UCPInventoryWidget* InventoryWidget = CreateWidget<UCPInventoryWidget>(OwningController, InventoryWidgetClass))
	{
		InventoryWidget->AddToViewport();
	}
}

void ACPGameMode::AttachReviveGaugeToPlayer(ACPPlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !ReviveGaugeComponentClass || PlayerCharacter->GetReviveGaugeComponent())
	{
		return;
	}

	UCPRadialGaugeComponent* Gauge = NewObject<UCPRadialGaugeComponent>(PlayerCharacter, ReviveGaugeComponentClass);
	if (!Gauge)
	{
		return;
	}

	
	Gauge->SetupAttachment(PlayerCharacter->GetRootComponent());
	Gauge->RegisterComponent();
	Gauge->SetGaugeEnabled(false);

	PlayerCharacter->SetReviveGaugeComponent(Gauge);
}

float ACPGameMode::GetRequiredTeamExperience() const
{
	return BaseRequiredTeamExperience + static_cast<float>(TeamLevel - 1) * RequiredTeamExperiencePerLevel;
}

void ACPGameMode::AddTeamExperience(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	TeamExperience = FMath::Clamp(TeamExperience + Amount, TeamExperienceRange.Min, TeamExperienceRange.Max);

	float RequiredExperience = GetRequiredTeamExperience();
	while (TeamLevel < FMath::RoundToInt32(TeamLevelRange.Max) && TeamExperience >= RequiredExperience && RequiredExperience > 0.0f)
	{
		TeamExperience -= RequiredExperience;
		TeamLevel += 1;

		OnTeamLevelUp.Broadcast(TeamLevel);

		RequiredExperience = GetRequiredTeamExperience();
	}

	TeamLevel = FMath::Clamp(TeamLevel, FMath::RoundToInt32(TeamLevelRange.Min), FMath::RoundToInt32(TeamLevelRange.Max));
}
}