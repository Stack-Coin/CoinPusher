// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPStatWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

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

	if (ExperienceText)
	{
		ExperienceText->SetText(FText::FromString(FString::Printf(TEXT("Experience : %.0f"), Interface->GetStat(ECPStatType::Experience))));
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

	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level : %.0f"), Interface->GetStat(ECPStatType::Level))));
	}
}

void UCPStatWidget::HandleAddExperienceClicked()
{
	if (ICPStatInterface* Interface = StatSource.GetInterface())
	{
		Interface->ModifyStat(ECPStatType::Experience, TestExperienceAmount);
	}
}
