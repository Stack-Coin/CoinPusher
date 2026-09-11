// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPInGameWidget.h"
#include "UI/CPCharacterInfoWidget.h"
#include "UI/CPCoinComboWidget.h"
#include "UI/CPTicketCountWidget.h"
#include "UI/CPInventoryWidget.h"
#include "UI/CPCoinPointUI.h"
#include "Roulette/CPRouletteWidget.h"
#include "Components/Image.h"

namespace
{
	ESlateVisibility ToSlateVisibility(bool bVisible)
	{
		return bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	}
}

void UCPInGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 아직 보스 관련 시스템이 없어 당장 보여줄 값이 없으므로 기본적으로 꺼둔다
	SetBossInfoVisible(false);
}

void UCPInGameWidget::UpdatePlayerHealth(float CurrentHealth, float MaxHealth)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->UpdateHealth(CurrentHealth, MaxHealth);
	}
}

void UCPInGameWidget::UpdatePlayerExp(float CurrentExp, float MaxExp)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->UpdateExp(CurrentExp, MaxExp);
	}
}

void UCPInGameWidget::SetPlayerName(const FText& CharacterName)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetCharacterName(CharacterName);
	}
}

void UCPInGameWidget::SetPlayerLevel(int32 Level)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetLevel(Level);
	}
}

void UCPInGameWidget::SetPlayerPortrait(UTexture2D* Portrait)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetPortrait(Portrait);
	}
}

void UCPInGameWidget::UpdateBossHealth(float CurrentHealth, float MaxHealth)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->UpdateHealth(CurrentHealth, MaxHealth);
	}
}

void UCPInGameWidget::UpdateBossExp(float CurrentExp, float MaxExp)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->UpdateExp(CurrentExp, MaxExp);
	}
}

void UCPInGameWidget::SetBossName(const FText& CharacterName)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->SetCharacterName(CharacterName);
	}
}

void UCPInGameWidget::SetBossLevel(int32 Level)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->SetLevel(Level);
	}
}

void UCPInGameWidget::SetBossPortrait(UTexture2D* Portrait)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->SetPortrait(Portrait);
	}
}

void UCPInGameWidget::UpdateTicketCount(int32 Count)
{
	if (TicketCountWidget)
	{
		TicketCountWidget->UpdateTicketCount(Count);
	}
}

void UCPInGameWidget::SetComboCount(int32 Count)
{
	if (CoinComboWidget)
	{
		CoinComboWidget->SetComboCount(Count);
	}
}

void UCPInGameWidget::UpdateComboGauge(float CurrentValue, float MaxValue)
{
	if (CoinComboWidget)
	{
		CoinComboWidget->UpdateComboGauge(CurrentValue, MaxValue);
	}
}

void UCPInGameWidget::SetPlayerInfoVisible(bool bVisible)
{
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetBossInfoVisible(bool bVisible)
{
	if (BossInfoWidget)
	{
		BossInfoWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetBackgroundVisible(bool bVisible)
{
	if (BackgroundImage)
	{
		BackgroundImage->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetTicketCountVisible(bool bVisible)
{
	if (TicketCountWidget)
	{
		TicketCountWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetInventoryVisible(bool bVisible)
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetCoinComboVisible(bool bVisible)
{
	if (CoinComboWidget)
	{
		CoinComboWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetRouletteVisible(bool bVisible)
{
	if (RouletteWidget)
	{
		RouletteWidget->SetVisibility(ToSlateVisibility(bVisible));
	}
}

void UCPInGameWidget::SetCoinPointUIVisible(bool bVisible)
{
	if (CoinPointUI)
	{
		CoinPointUI->SetVisibility(ToSlateVisibility(bVisible));
	}
}
