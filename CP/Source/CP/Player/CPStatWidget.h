// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/CPStatInterface.h"
#include "Player/CPItemInventory.h"
#include "CPStatWidget.generated.h"

class UTextBlock;
class UButton;

/**
 *  Simple debug/test UI that displays the current values of a player's stats, plus the team-shared
 *  resources owned by ACPGameMode (ticket/experience/level). Per-player stats are read purely
 *  through ICPStatInterface, so it works with any actor that implements it. No layout/design is
 *  provided - place the TextBlocks (matching these variable names) in the Widget Blueprint that
 *  inherits from this class.
 */
UCLASS(abstract)
class CP_API UCPStatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* ExperienceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* AttackPowerText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* MoveSpeedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* AttackSpeedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* DefenseText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* LevelText;

	/** Displays the names of every currently owned item */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* OwnedItemsText;

	/** Displays the currently equipped weapon's display name, or "Unarmed" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* CurrentWeaponText;

	/** Displays the current team ticket count (ACPGameMode::GetTeamTicketCount) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TicketText;

	/** Test button that grants TestExperienceAmount team experience when clicked */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* AddExperienceButton;

	/** Amount of team experience granted by a single AddExperienceButton click */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta = (ClampMin = 0))
	float TestExperienceAmount = 20.0f;

	/** Object providing the stat values to display. Defaults to the owning player's pawn if it implements ICPStatInterface */
	UPROPERTY()
	TScriptInterface<ICPStatInterface> StatSource;

public:

	/** Assigns the object whose stats should be displayed */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetStatSource(TScriptInterface<ICPStatInterface> InStatSource);

protected:

	/** Gameplay initialization */
	virtual void NativeConstruct() override;

	/** Refreshes the stat text every frame */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Pulls the current stat values from StatSource and updates the TextBlocks */
	void RefreshStats();

	/** Bound to AddExperienceButton's OnClicked. Grants TestExperienceAmount experience through the stat interface */
	UFUNCTION()
	void HandleAddExperienceClicked();
};
