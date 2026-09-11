// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Monster/Spawner/CPUserWidget_WaveStatus.h"
#include "Player/CPPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "UI/CPHorizonGuageBarWidget.h"
#include "UI/CPTicketCountWidget.h"
#include "UI/CPCoinCountWidget.h"
#include "UI/CPRadialGaugeComponent.h"
#include "UI/CPInventoryWidget.h"
#include "UI/CPInGameWidget.h"
#include "Player/CPTopDownPlayerController.h"
#include "TimerManager.h"

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

			PlayerCharacter->OnPlayerDowned.AddDynamic(this, &ACPGameMode::HandlePlayerDowned);

			// InGameUI는 컨트롤러 자신의 BeginPlay에서 만들어지는데, 액터 간 BeginPlay 순서는
			// 보장되지 않으므로 한 틱 미뤄서 항상 준비된 뒤에 바인딩한다
			TWeakObjectPtr<ACPPlayerCharacter> WeakPlayerCharacter(PlayerCharacter);
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ACPGameMode::SetupPlayerInGameWidgetBindings, WeakPlayerCharacter));
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
			PlayerCharacter->OnScoreChanged.AddDynamic(CoinWidget, &UCPCoinCountWidget::UpdateCoinCount);
			CoinWidget->UpdateCoinCount(PlayerCharacter->GetScoreAmount());
		}
	}
}

void ACPGameMode::SetupPlayerHealthBarWidget(ACPPlayerCharacter* PlayerCharacter, TSubclassOf<UCPHorizonGuageBarWidget> HealthBarWidgetClass)
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

	UCPHorizonGuageBarWidget* HealthBarWidget = CreateWidget<UCPHorizonGuageBarWidget>(OwningController, HealthBarWidgetClass);
	if (!HealthBarWidget)
	{
		return;
	}

	HealthBarWidget->AddToViewport();

	PlayerCharacter->OnHealthChanged.AddDynamic(HealthBarWidget, &UCPHorizonGuageBarWidget::Update);
	HealthBarWidget->Update(PlayerCharacter->GetStat(ECPStatType::Health), PlayerCharacter->GetMaxHealth());
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

void ACPGameMode::SetupPlayerInGameWidgetBindings(TWeakObjectPtr<ACPPlayerCharacter> WeakPlayerCharacter)
{
	ACPPlayerCharacter* PlayerCharacter = WeakPlayerCharacter.Get();
	if (!PlayerCharacter)
	{
		return;
	}

	ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(PlayerCharacter->GetController());
	UCPInGameWidget* InGameWidget = PC ? PC->GetInGameWidget() : nullptr;
	if (!InGameWidget)
	{
		return;
	}

	PlayerCharacter->OnHealthChanged.AddDynamic(InGameWidget, &UCPInGameWidget::UpdatePlayerHealth);
	PlayerCharacter->OnExpChanged.AddDynamic(InGameWidget, &UCPInGameWidget::UpdatePlayerExp);
	PlayerCharacter->OnLevelChanged.AddDynamic(InGameWidget, &UCPInGameWidget::SetPlayerLevel);
	PlayerCharacter->OnTicketChanged.AddDynamic(InGameWidget, &UCPInGameWidget::UpdateTicketCount);

	InGameWidget->UpdatePlayerHealth(PlayerCharacter->GetStat(ECPStatType::Health), PlayerCharacter->GetMaxHealth());
	InGameWidget->UpdatePlayerExp(PlayerCharacter->GetStat(ECPStatType::Experience), PlayerCharacter->GetMaxExperience());
	InGameWidget->SetPlayerLevel(PlayerCharacter->GetPlayerLevel());
	InGameWidget->UpdateTicketCount(PlayerCharacter->GetTicketCount());
}

void ACPGameMode::HandlePlayerDowned()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ACPTopDownPlayerController* TopDownPC = Cast<ACPTopDownPlayerController>(PC))
		{
			TopDownPC->ShowEndingResult(false);
		}
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