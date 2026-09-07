// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/CPDebugWidget.h"
#include "Debug/CPDebugCollisionSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/EditableText.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CPStatInterface.h"
#include "Player/CPWeaponEquipper.h"
#include "Weapon/CPWeaponBase.h"
#include "Player/CPGameMode.h"
#include "Player/CPPlayerCharacter.h"

void UCPDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeCheckBox(PlayerHitboxCheckBox, ECPDebugCollisionCategory::PlayerHitbox);
	InitializeCheckBox(PlayerWeaponCheckBox, ECPDebugCollisionCategory::PlayerWeapon);
	InitializeCheckBox(EnemyHitboxCheckBox, ECPDebugCollisionCategory::EnemyHitbox);
	InitializeCheckBox(MonsterAttackRangeCheckBox, ECPDebugCollisionCategory::MonsterAttackRange);
	InitializeCheckBox(MonsterDetectRangeCheckBox, ECPDebugCollisionCategory::MonsterDetectRange);
	InitializeCheckBox(PlayerReviveCheckBox, ECPDebugCollisionCategory::PlayerRevive);
	InitializeCheckBox(CoinNexusCheckBox, ECPDebugCollisionCategory::CoinNexus);
	InitializeCheckBox(ItemPickupCheckBox, ECPDebugCollisionCategory::ItemPickup);

	if (PlayerHitboxCheckBox)
	{
		PlayerHitboxCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayerHitboxCheckChanged);
	}
	if (PlayerWeaponCheckBox)
	{
		PlayerWeaponCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayerWeaponCheckChanged);
	}
	if (EnemyHitboxCheckBox)
	{
		EnemyHitboxCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandleEnemyHitboxCheckChanged);
	}
	if (MonsterAttackRangeCheckBox)
	{
		MonsterAttackRangeCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandleMonsterAttackRangeCheckChanged);
	}
	if (MonsterDetectRangeCheckBox)
	{
		MonsterDetectRangeCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandleMonsterDetectRangeCheckChanged);
	}
	if (PlayerReviveCheckBox)
	{
		PlayerReviveCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayerReviveCheckChanged);
	}
	if (CoinNexusCheckBox)
	{
		CoinNexusCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandleCoinNexusCheckChanged);
	}
	if (ItemPickupCheckBox)
	{
		ItemPickupCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandleItemPickupCheckChanged);
	}
	if (SetTeamCoinButton)
	{
		SetTeamCoinButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleSetTeamCoinClicked);
	}
	if (SetTeamTicketButton)
	{
		SetTeamTicketButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleSetTeamTicketClicked);
	}
	if (Player1InvincibleCheckBox)
	{
		Player1InvincibleCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayer1InvincibleCheckChanged);
	}
	if (Player2InvincibleCheckBox)
	{
		Player2InvincibleCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayer2InvincibleCheckChanged);
	}

	RefreshPlayerInfo();
}

void UCPDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	RefreshPlayerInfo();
}

void UCPDebugWidget::RefreshPlayerInfo()
{
	if (Player1InfoText)
	{
		Player1InfoText->SetText(FText::FromString(BuildPlayerInfoString(0)));
	}

	if (Player2InfoText)
	{
		Player2InfoText->SetText(FText::FromString(BuildPlayerInfoString(1)));
	}
}

FString UCPDebugWidget::BuildPlayerInfoString(int32 PlayerIndex) const
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), PlayerIndex);
	if (!Pawn)
	{
		return FString::Printf(TEXT("Player %d : (no pawn)"), PlayerIndex + 1);
	}

	FString WeaponName = TEXT("Unarmed");
	if (ICPWeaponEquipper* WeaponEquipper = Cast<ICPWeaponEquipper>(Pawn))
	{
		if (const ACPWeaponBase* CurrentWeapon = WeaponEquipper->GetCurrentWeapon())
		{
			WeaponName = CurrentWeapon->GetWeaponDisplayName().ToString();
		}
	}

	const ICPStatInterface* StatInterface = Cast<ICPStatInterface>(Pawn);
	if (!StatInterface)
	{
		return FString::Printf(TEXT("Player %d\nWeapon : %s"), PlayerIndex + 1, *WeaponName);
	}

	return FString::Printf(
		TEXT("Player %d\nHealth : %.0f\nAttackPower : %.0f\nMoveSpeed : %.0f\nAttackSpeed : %.2f\nDefense : %.0f\nWeapon : %s"),
		PlayerIndex + 1,
		StatInterface->GetStat(ECPStatType::Health),
		StatInterface->GetStat(ECPStatType::AttackPower),
		StatInterface->GetStat(ECPStatType::MoveSpeed),
		StatInterface->GetStat(ECPStatType::AttackSpeed),
		StatInterface->GetStat(ECPStatType::Defense),
		*WeaponName);
}

void UCPDebugWidget::InitializeCheckBox(UCheckBox* CheckBox, ECPDebugCollisionCategory Category)
{
	if (!CheckBox)
	{
		return;
	}

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		// SetIsChecked only updates the widget's own state - it does not invoke OnCheckStateChanged
		CheckBox->SetIsChecked(Subsystem->IsCategoryVisible(Category));
	}
}

void UCPDebugWidget::SetCategoryVisible(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->SetCategoryVisible(Category, bVisible);
	}
}

void UCPDebugWidget::HandlePlayerHitboxCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::PlayerHitbox, bIsChecked);
}

void UCPDebugWidget::HandlePlayerWeaponCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::PlayerWeapon, bIsChecked);
}

void UCPDebugWidget::HandleEnemyHitboxCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::EnemyHitbox, bIsChecked);
}

void UCPDebugWidget::HandleMonsterAttackRangeCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::MonsterAttackRange, bIsChecked);
}

void UCPDebugWidget::HandleMonsterDetectRangeCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::MonsterDetectRange, bIsChecked);
}

void UCPDebugWidget::HandlePlayerReviveCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::PlayerRevive, bIsChecked);
}

void UCPDebugWidget::HandleCoinNexusCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::CoinNexus, bIsChecked);
}

void UCPDebugWidget::HandleItemPickupCheckChanged(bool bIsChecked)
{
	SetCategoryVisible(ECPDebugCollisionCategory::ItemPickup, bIsChecked);
}

void UCPDebugWidget::HandleSetTeamCoinClicked()
{
	if (!TeamCoinInputText)
	{
		return;
	}

	if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
	{
		// Adds to the current count rather than replacing it - this is an "add N coins" button, not a
		// "set the count to N" one, so clicking it repeatedly with the same input keeps incrementing
		GameMode->AddCoin(FCString::Atoi(*TeamCoinInputText->GetText().ToString()));
	}
}

void UCPDebugWidget::HandleSetTeamTicketClicked()
{
	if (!TeamTicketInputText)
	{
		return;
	}

	if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
	{
		// Adds to the current count rather than replacing it - see HandleSetTeamCoinClicked
		GameMode->AddTeamTickets(FCString::Atoi(*TeamTicketInputText->GetText().ToString()));
	}
}

void UCPDebugWidget::HandlePlayer1InvincibleCheckChanged(bool bIsChecked)
{
	SetPlayerDebugInvincible(0, bIsChecked);
}

void UCPDebugWidget::HandlePlayer2InvincibleCheckChanged(bool bIsChecked)
{
	SetPlayerDebugInvincible(1, bIsChecked);
}

void UCPDebugWidget::SetPlayerDebugInvincible(int32 PlayerIndex, bool bEnabled)
{
	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), PlayerIndex)))
	{
		PlayerCharacter->SetDebugInvincible(bEnabled);
	}
}
