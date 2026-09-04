// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Nexus/CPGoddess.h"
#include "Nexus/CPNexus.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Monster/Spawner/CPUserWidget_WaveStatus.h"
#include "Player/CPPartyCamera.h"
#include "Player/CPPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "UI/CPHealthBarWidget.h"
#include "UI/CPTicketCountWidget.h"
#include "UI/CPCoinCountWidget.h"
#include "UI/CPRadialGaugeComponent.h"

ACPGameMode::ACPGameMode()
{
	// stub
}

void ACPGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Create each additional local player - Player 0 (keyboard/mouse) is created automatically as part
	// of regular game init. Fixed at level start, no drop-in join (see NumberOfLocalPlayers)
	for (int32 i = 2; i <= NumberOfLocalPlayers; ++i)
	{
		UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
	}

	// A gamepad plugged in before launch is often already detected by now - try right away...
	TryAssignGamepadToSecondPlayer();

	// ...but device detection (XInput/RawInput polling) can still lag a frame or more past BeginPlay,
	// so keep watching for one to show up later too
	InputDeviceConnectionChangeHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(this, &ACPGameMode::HandleInputDeviceConnectionChange);

	// KohMs // Goddess가 죽으면 패배
	if (ACPGoddess* Goddess = Cast<ACPGoddess>(UGameplayStatics::GetActorOfClass(this, ACPGoddess::StaticClass())))
	{
		Goddess->OnGoddessDead.AddUniqueDynamic(this, &ACPGameMode::HandleGoddessDead);
	}

	// KohMS // 웨이브 진행 상황을 화면에 텍스트로 표시 (레벨에 배치된 스포너 중 하나의 상태를 그대로 보여줍니다)
	WaveStatusSourceSpawner = Cast<ACPMonsterSpawner>(UGameplayStatics::GetActorOfClass(this, ACPMonsterSpawner::StaticClass()));
	if (WaveStatusSourceSpawner)
	{
		WaveStatusWidget = CreateWidget<UCPUserWidget_WaveStatus>(GetWorld(), UCPUserWidget_WaveStatus::StaticClass());
		if (WaveStatusWidget)
		{
			WaveStatusWidget->AddToViewport();
			GetWorld()->GetTimerManager().SetTimer(WaveStatusUpdateTimer, this, &ACPGameMode::UpdateWaveStatusDisplay, 0.2f, true);
		}
	}

	SetupTeamResourceWidgets();

	// Per-local-player UI: health bar (1P/2P) and the revive progress gauge. Both players' pawns are
	// already possessed by this point (created above/by the engine's own initial-player flow)
	TArray<TSubclassOf<UCPHealthBarWidget>> HealthBarClassesByIndex = { Player1HealthBarWidgetClass, Player2HealthBarWidgetClass };
	int32 LocalPlayerIndex = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}

		if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(PC->GetPawn()))
		{
			AttachReviveGaugeToPlayer(PlayerCharacter);

			if (HealthBarClassesByIndex.IsValidIndex(LocalPlayerIndex))
			{
				SetupPlayerHealthBarWidget(PlayerCharacter, HealthBarClassesByIndex[LocalPlayerIndex]);
			}
		}

		++LocalPlayerIndex;
	}
}

void ACPGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(InputDeviceConnectionChangeHandle);

	Super::EndPlay(EndPlayReason);
}

void ACPGameMode::HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	if (NewConnectionState == EInputDeviceConnectionState::Connected)
	{
		TryAssignGamepadToSecondPlayer();
	}
}

void ACPGameMode::TryAssignGamepadToSecondPlayer()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULocalPlayer* SecondLocalPlayer = GameInstance ? GameInstance->GetLocalPlayerByIndex(1) : nullptr;
	if (!SecondLocalPlayer)
	{
		return;
	}

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	const FPlatformUserId SecondPlayerUserId = SecondLocalPlayer->GetPlatformUserId();

	// Already has a device mapped (e.g. a previous call already assigned one)? Nothing to do
	TArray<FInputDeviceId> ExistingDevices;
	DeviceMapper.GetAllInputDevicesForUser(SecondPlayerUserId, ExistingDevices);
	if (!ExistingDevices.IsEmpty())
	{
		return;
	}

	const FInputDeviceId DefaultDevice = DeviceMapper.GetDefaultInputDevice();

	TArray<FInputDeviceId> ConnectedDevices;
	DeviceMapper.GetAllConnectedInputDevices(ConnectedDevices);

	for (const FInputDeviceId& DeviceId : ConnectedDevices)
	{
		// Skip the default device (keyboard/mouse) - only assign an actual gamepad
		if (DeviceId == DefaultDevice)
		{
			continue;
		}

		const FPlatformUserId OldUserId = DeviceMapper.GetUserForInputDevice(DeviceId);
		DeviceMapper.Internal_ChangeInputDeviceUserMapping(DeviceId, SecondPlayerUserId, OldUserId);
		break;
	}
}

AActor* ACPGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Build the current player tag
	const FName PlayerTag = FName(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));

	// Find all player starts with the matching player tag
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	++CurrentPlayerStartAssignment;

	// If no PlayerStarts were found with this tag, fall back to any PlayerStart in the level
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

void ACPGameMode::ToggleCameraMode()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bIsSingleCameraMode = !bIsSingleCameraMode;

	UGameViewportClient* ViewportClient = World->GetGameViewport();

	if (bIsSingleCameraMode)
	{
		if (!PartyCamera)
		{
			TSubclassOf<ACPPartyCamera> ClassToSpawn = ACPPartyCamera::StaticClass();
			if (PartyCameraClass)
			{
				ClassToSpawn = PartyCameraClass;
			}
			PartyCamera = World->SpawnActor<ACPPartyCamera>(ClassToSpawn);
		}

		if (!PartyCamera)
		{
			bIsSingleCameraMode = false;
			return;
		}

		TArray<TWeakObjectPtr<AActor>> TrackedPlayers;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController() && PC->GetPawn())
			{
				TrackedPlayers.Add(PC->GetPawn());
			}
		}
		PartyCamera->SetTrackedActors(TrackedPlayers);
		PartyCamera->SetActorTickEnabled(true);

		if (ViewportClient)
		{
			ViewportClient->SetForceDisableSplitscreen(true);
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (PC->IsLocalController())
				{
					PC->SetViewTargetWithBlend(PartyCamera, CameraSwapBlendTime);
				}
			}
		}
	}
	else
	{
		if (ViewportClient)
		{
			ViewportClient->SetForceDisableSplitscreen(false);
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (PC->IsLocalController() && PC->GetPawn())
				{
					PC->SetViewTargetWithBlend(PC->GetPawn(), CameraSwapBlendTime);
				}
			}
		}

		if (PartyCamera)
		{
			PartyCamera->SetActorTickEnabled(false);
		}
	}
}

void ACPGameMode::SetupTeamResourceWidgets()
{
	if (TicketWidgetClass)
	{
		if (UCPTicketCountWidget* TicketWidget = CreateWidget<UCPTicketCountWidget>(GetWorld(), TicketWidgetClass))
		{
			TicketWidget->AddToViewport();
			OnTeamTicketCountChanged.AddDynamic(TicketWidget, &UCPTicketCountWidget::UpdateTicketCount);
			TicketWidget->UpdateTicketCount(TeamTicketCount);
		}
	}

	if (CoinWidgetClass)
	{
		if (UCPCoinCountWidget* CoinWidget = CreateWidget<UCPCoinCountWidget>(GetWorld(), CoinWidgetClass))
		{
			CoinWidget->AddToViewport();
			OnTeamCoinCountChanged.AddDynamic(CoinWidget, &UCPCoinCountWidget::UpdateCoinCount);
			CoinWidget->UpdateCoinCount(TeamCoinCount);
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

	HealthBarWidget->AddToPlayerScreen();

	PlayerCharacter->OnHealthChanged.AddDynamic(HealthBarWidget, &UCPHealthBarWidget::UpdateHealth);
	HealthBarWidget->UpdateHealth(PlayerCharacter->GetStat(ECPStatType::Health), PlayerCharacter->GetMaxHealth());
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

void ACPGameMode::AddTeamTickets(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TeamTicketCount += Amount;

	OnTeamTicketCountChanged.Broadcast(TeamTicketCount);
}

bool ACPGameMode::TrySpendTeamTicket(int32 Amount)
{
	if (Amount <= 0 || TeamTicketCount < Amount)
	{
		return false;
	}

	TeamTicketCount -= Amount;

	OnTeamTicketCountChanged.Broadcast(TeamTicketCount);

	return true;
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

void ACPGameMode::AddCoin(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TeamCoinCount += Amount;

	OnTeamCoinCountChanged.Broadcast(TeamCoinCount);
}

bool ACPGameMode::TrySpendCoin(int32 Amount)
{
	if (!HasEnoughCoin(Amount))
	{
		return false;
	}

	TeamCoinCount -= Amount;

	OnTeamCoinCountChanged.Broadcast(TeamCoinCount);

	return true;
}

// KohMS
void ACPGameMode::UpdateWaveStatusDisplay()
{
	if (!WaveStatusSourceSpawner || !WaveStatusWidget)
	{
		return;
	}

	const ECPWavePhase Phase = WaveStatusSourceSpawner->GetCurrentPhase();

	int32 ProgressSeconds = 0;
	int32 TotalSeconds = 0;

	switch (Phase)
	{
	case ECPWavePhase::Spawning:
		ProgressSeconds = WaveStatusSourceSpawner->GetSpawnElapsedSeconds();
		TotalSeconds = WaveStatusSourceSpawner->GetSpawnTotalSeconds();
		break;

	case ECPWavePhase::WaveWait:
		TotalSeconds = FMath::RoundToInt(WaveStatusSourceSpawner->GetWaveIntervalSeconds());
		ProgressSeconds = FMath::Clamp(TotalSeconds - FMath::CeilToInt(WaveStatusSourceSpawner->GetWaveWaitSecondsRemaining()), 0, TotalSeconds);
		break;

	case ECPWavePhase::RoundWait:
		TotalSeconds = FMath::RoundToInt(WaveStatusSourceSpawner->GetRoundEndWaitSeconds());
		ProgressSeconds = FMath::Clamp(TotalSeconds - FMath::CeilToInt(WaveStatusSourceSpawner->GetRoundWaitSecondsRemaining()), 0, TotalSeconds);
		break;

	default:
		break;
	}

	WaveStatusWidget->UpdateWaveStatus(
		WaveStatusSourceSpawner->GetCurrentWaveIndex() + 1,
		WaveStatusSourceSpawner->GetWaveCount(),
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