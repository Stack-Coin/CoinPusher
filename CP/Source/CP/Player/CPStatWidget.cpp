// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPStatWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CPWeaponEquipper.h"
#include "Weapon/CPWeaponBase.h"
#include "Player/CPGameMode.h"

void UCPStatWidget::SetStatSource(TScriptInterface<ICPStatInterface> InStatSource)
{
	StatSource = InStatSource;
}

void UCPStatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!StatSource.GetObject())
	{
		if (APawn* OwningPawn = GetOwningPlayerPawn())
		{
			if (OwningPawn->GetClass()->ImplementsInterface(UCPStatInterface::StaticClass()))
			{
				StatSource.SetObject(OwningPawn);
				StatSource.SetInterface(Cast<ICPStatInterface>(OwningPawn));
			}
		}
	}

	if (AddExperienceButton)
	{
		AddExperienceButton->OnClicked.AddDynamic(this, &UCPStatWidget::HandleAddExperienceClicked);
	}

	RefreshStats();
}

void UCPStatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshStats();
}

void UCPStatWidget::RefreshStats()
{
	ICPStatInterface* Interface = StatSource.GetInterface();
	if (!Interface)
	{
		return;
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("Health : %.0f"), Interface->GetStat(ECPStatType::Health))));
	}

	if (AttackPowerText)
	{
		AttackPowerText->SetText(FText::FromString(FString::Printf(TEXT("AttackPower : %.0f"), Interface->GetStat(ECPStatType::AttackPower))));
	}

	if (MoveSpeedText)
	{
		MoveSpeedText->SetText(FText::FromString(FString::Printf(TEXT("MoveSpeed : %.0f"), Interface->GetStat(ECPStatType::MoveSpeed))));
	}

	if (AttackSpeedText)
	{
		AttackSpeedText->SetText(FText::FromString(FString::Printf(TEXT("AttackSpeed : %.2f"), Interface->GetStat(ECPStatType::AttackSpeed))));
	}

	if (DefenseText)
	{
		DefenseText->SetText(FText::FromString(FString::Printf(TEXT("Defense : %.0f"), Interface->GetStat(ECPStatType::Defense))));
	}

	UObject* SourceObject = StatSource.GetObject();

	const ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr;

	if (ExperienceText && GameMode)
	{
		ExperienceText->SetText(FText::FromString(FString::Printf(TEXT("Experience : %.0f"), GameMode->GetTeamExperience())));
	}

	if (LevelText && GameMode)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level : %d"), GameMode->GetTeamLevel())));
	}

	if (TicketText && GameMode)
	{
		TicketText->SetText(FText::FromString(FString::Printf(TEXT("Ticket : %d"), GameMode->GetTeamTicketCount())));
	}

	if (OwnedItemsText)
	{
		if (ICPItemInventory* Inventory = Cast<ICPItemInventory>(SourceObject))
		{
			const TArray<FCPItemData>& Items = Inventory->GetOwnedItems();

			TArray<FString> ItemNames;
			ItemNames.Reserve(Items.Num());
			for (const FCPItemData& Item : Items)
			{
				ItemNames.Add(Item.ItemName.ToString());
			}

			const FString JoinedNames = Items.Num() > 0 ? FString::Join(ItemNames, TEXT(", ")) : TEXT("(none)");
			OwnedItemsText->SetText(FText::FromString(FString::Printf(TEXT("Items : %s"), *JoinedNames)));
		}
	}

	if (CurrentWeaponText)
	{
		if (ICPWeaponEquipper* WeaponEquipper = Cast<ICPWeaponEquipper>(SourceObject))
		{
			const ACPWeaponBase* CurrentWeapon = WeaponEquipper->GetCurrentWeapon();
			const FText WeaponName = CurrentWeapon ? CurrentWeapon->GetWeaponDisplayName() : FText::FromString(TEXT("Unarmed"));
			CurrentWeaponText->SetText(FText::FromString(FString::Printf(TEXT("Weapon : %s"), *WeaponName.ToString())));
		}
	}

}

void UCPStatWidget::HandleAddExperienceClicked()
{
	if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
	{
		GameMode->AddTeamExperience(TestExperienceAmount);
	}
}
