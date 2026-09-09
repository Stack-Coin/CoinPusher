// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Nexus/CPGoddess.h"
#include "Nexus/CPNexus.h"
#include "Monster/Spawner/CPMonsterSpawnManager.h"
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

	// KohMs // Goddess가 죽으면 패배
	if (ACPGoddess* Goddess = Cast<ACPGoddess>(UGameplayStatics::GetActorOfClass(this, ACPGoddess::StaticClass())))
	{
		Goddess->OnGoddessDead.AddUniqueDynamic(this, &ACPGameMode::HandleGoddessDead);
	}

	// KohMS // 웨이브 진행 상황을 화면에 텍스트로 표시 (레벨에 배치된 매니저의 상태를 그대로 보여줍니다)
	WaveStatusSourceManager = Cast<ACPMonsterSpawnManager>(UGameplayStatics::GetActorOfClass(this, ACPMonsterSpawnManager::StaticClass()));
	if (WaveStatusSourceManager)
	{
		WaveStatusWidget = CreateWidget<UCPUserWidget_WaveStatus>(GetWorld(), UCPUserWidget_WaveStatus::StaticClass());
		if (WaveStatusWidget)
		{
			WaveStatusWidget->AddToViewport();
			GetWorld()->GetTimerManager().SetTimer(WaveStatusUpdateTimer, this, &ACPGameMode::UpdateWaveStatusDisplay, 0.2f, true);
		}
	}

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

// KohMS
void ACPGameMode::UpdateWaveStatusDisplay()
{
	if (!WaveStatusSourceManager || !WaveStatusWidget)
	{
		return;
	}

	const ECPWavePhase Phase = WaveStatusSourceManager->GetCurrentPhase();

	int32 ProgressSeconds = 0;
	int32 TotalSeconds = 0;

	switch (Phase)
	{
	case ECPWavePhase::Spawning:
		ProgressSeconds = WaveStatusSourceManager->GetSpawnElapsedSeconds();
		TotalSeconds = WaveStatusSourceManager->GetSpawnTotalSeconds();
		break;

	case ECPWavePhase::WaveWait:
		TotalSeconds = FMath::RoundToInt(WaveStatusSourceManager->GetWaveIntervalSeconds());
		ProgressSeconds = FMath::Clamp(TotalSeconds - FMath::CeilToInt(WaveStatusSourceManager->GetWaveWaitSecondsRemaining()), 0, TotalSeconds);
		break;

	case ECPWavePhase::RoundWait:
		TotalSeconds = FMath::RoundToInt(WaveStatusSourceManager->GetRoundEndWaitSeconds());
		ProgressSeconds = FMath::Clamp(TotalSeconds - FMath::CeilToInt(WaveStatusSourceManager->GetRoundWaitSecondsRemaining()), 0, TotalSeconds);
		break;

	default:
		break;
	}

	WaveStatusWidget->UpdateWaveStatus(
		WaveStatusSourceManager->GetCurrentWaveIndex() + 1,
		WaveStatusSourceManager->GetWaveCount(),
		Phase,
		ProgressSeconds,
		TotalSeconds);
}

void ACPGameMode::HandleGoddessDead()
{
	if (bIsGameOver)
	{
		return;
	}

	bIsGameOver = true;

	UE_LOG(LogTemp, Warning, TEXT("[CPGameMode] Defeat! Goddess has fallen."));
}