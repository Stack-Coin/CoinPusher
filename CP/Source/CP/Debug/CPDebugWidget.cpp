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
#include "Player/CPPlayerCharacter.h"
#include "Player/Inventory/CPInventoryComponent.h"

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
	if (PlayerInvincibleCheckBox)
	{
		PlayerInvincibleCheckBox->OnCheckStateChanged.AddDynamic(this, &UCPDebugWidget::HandlePlayerInvincibleCheckChanged);
	}
	if (ApplyDamageButton)
	{
		ApplyDamageButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleApplyDamageClicked);
	}
	if (ActivatePassiveSkillButton)
	{
		ActivatePassiveSkillButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleActivatePassiveSkillClicked);
	}
	if (StoreItemButton)
	{
		StoreItemButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleStoreItemClicked);
	}
	if (RemoveItemButton)
	{
		RemoveItemButton->OnClicked.AddDynamic(this, &UCPDebugWidget::HandleRemoveItemClicked);
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
	if (PlayerInfoText)
	{
		PlayerInfoText->SetText(FText::FromString(BuildPlayerInfoString()));
	}
}

FString UCPDebugWidget::BuildPlayerInfoString() const
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Pawn)
	{
		return TEXT("Player : (no pawn)");
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
		return FString::Printf(TEXT("Weapon : %s"), *WeaponName);
	}

	return FString::Printf(
		TEXT("Health : %.0f\nAttackPower : %.0f\nMoveSpeed : %.0f\nAttackSpeed : %.2f\nExperience : %.0f\nLevel : %.0f\nWeapon : %s"),
		StatInterface->GetStat(ECPStatType::Health),
		StatInterface->GetStat(ECPStatType::AttackPower),
		StatInterface->GetStat(ECPStatType::MoveSpeed),
		StatInterface->GetStat(ECPStatType::AttackSpeed),
		StatInterface->GetStat(ECPStatType::Experience),
		StatInterface->GetStat(ECPStatType::Level),
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

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		// Adds to the current count rather than replacing it - this is an "add N coins" button, not a
		// "set the count to N" one, so clicking it repeatedly with the same input keeps incrementing
		PlayerCharacter->AddCoin(FCString::Atoi(*TeamCoinInputText->GetText().ToString()));
	}
}

void UCPDebugWidget::HandleSetTeamTicketClicked()
{
	if (!TeamTicketInputText)
	{
		return;
	}

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		// Adds to the current count rather than replacing it - see HandleSetTeamCoinClicked
		PlayerCharacter->AddTicket(FCString::Atoi(*TeamTicketInputText->GetText().ToString()));
	}
}

void UCPDebugWidget::HandlePlayerInvincibleCheckChanged(bool bIsChecked)
{
	SetPlayerDebugInvincible(bIsChecked);
}

void UCPDebugWidget::SetPlayerDebugInvincible(bool bEnabled)
{
	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		PlayerCharacter->SetDebugInvincible(bEnabled);
	}
}

void UCPDebugWidget::HandleApplyDamageClicked()
{
	if (!DamageInputText)
	{
		return;
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		UGameplayStatics::ApplyDamage(PlayerPawn, FCString::Atof(*DamageInputText->GetText().ToString()), nullptr, nullptr, nullptr);
	}
}

void UCPDebugWidget::HandleActivatePassiveSkillClicked()
{
	ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPDebugWidget::HandleActivatePassiveSkillClicked - no local ACPPlayerCharacter found"));
		return;
	}

	ACPWeaponBase* Weapon = PlayerCharacter->GetCurrentWeapon();
	if (!Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPDebugWidget::HandleActivatePassiveSkillClicked - player has no weapon equipped"));
		return;
	}

	Weapon->ActivatePassiveSkill();
}

void UCPDebugWidget::HandleStoreItemClicked()
{
	if (!ItemCodeInputText || !ItemCountInputText)
	{
		return;
	}

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		if (UCPInventoryComponent* Inventory = PlayerCharacter->GetInventoryComponent())
		{
			const FName ItemCode(*ItemCodeInputText->GetText().ToString());
			Inventory->StoreItem(ItemCode, FCString::Atoi(*ItemCountInputText->GetText().ToString()));
		}
	}
}

void UCPDebugWidget::HandleRemoveItemClicked()
{
	if (!ItemCodeInputText || !ItemCountInputText)
	{
		return;
	}

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		if (UCPInventoryComponent* Inventory = PlayerCharacter->GetInventoryComponent())
		{
			const FName ItemCode(*ItemCodeInputText->GetText().ToString());
			Inventory->RemoveItem(ItemCode, FCString::Atoi(*ItemCountInputText->GetText().ToString()));
		}
	}
}
