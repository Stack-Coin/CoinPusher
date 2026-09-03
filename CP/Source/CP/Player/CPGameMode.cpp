// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Nexus/CPGoddess.h"
#include "Nexus/CPNexus.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Monster/Spawner/CPUserWidget_WaveStatus.h"

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
}

bool ACPGameMode::TrySpendCoin(int32 Amount)
{
	if (!HasEnoughCoin(Amount))
	{
		return false;
	}

	TeamCoinCount -= Amount;

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