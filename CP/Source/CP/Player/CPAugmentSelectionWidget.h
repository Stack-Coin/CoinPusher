// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/CPAugmentTypes.h"
#include "Player/CPStatInterface.h"
#include "CPAugmentSelectionWidget.generated.h"

class UHorizontalBox;
class UCPAugmentCardWidget;

/**
 *  Shows a fixed number of randomly rolled augment cards laid out in a single horizontal row.
 *  Clicking a card applies its effects (through ICPStatInterface only) and closes the selection.
 *  New augments are added purely as data in AugmentPool - the selection code never has to change.
 */
UCLASS(abstract)
class CP_API UCPAugmentSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Container the augment cards are placed into, always laid out in a single horizontal row */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UHorizontalBox* CardContainer;

	/** Widget class spawned for each augment card. Must inherit from UCPAugmentCardWidget */
	UPROPERTY(EditAnywhere, Category="Augment")
	TSubclassOf<UCPAugmentCardWidget> CardWidgetClass;

	/** Pool of every possible augment. Add new augments here without touching any code or graph */
	UPROPERTY(EditAnywhere, Category="Augment")
	TArray<FCPAugmentData> AugmentPool;

	/** Number of cards offered on each level up */
	UPROPERTY(EditAnywhere, Category="Augment", meta = (ClampMin = 1))
	int32 NumCardsToOffer = 3;

	/** Horizontal gap between cards, in Slate units (pixels) */
	UPROPERTY(EditAnywhere, Category="Augment", meta = (ClampMin = 0))
	float CardSpacing = 20.0f;

	/** Object whose stats the selected augment will modify */
	UPROPERTY()
	TScriptInterface<ICPStatInterface> StatTarget;

public:

	/** Rolls NumCardsToOffer random augments from the pool and shows the selection UI */
	UFUNCTION(BlueprintCallable, Category="Augment")
	void OpenAugmentSelection(TScriptInterface<ICPStatInterface> InStatTarget);

protected:

	/** Bound to every spawned card's OnCardSelected */
	UFUNCTION()
	void HandleCardSelected(const FCPAugmentData& SelectedAugment);

	/** Applies every stat effect of an augment, through the stat interface only */
	void ApplyAugment(const FCPAugmentData& Augment);
};
