// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/CPAugmentTypes.h"
#include "CPAugmentCardWidget.generated.h"

class UButton;
class UTextBlock;

/** Broadcast when this card is clicked, carrying the augment it represents */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPAugmentCardSelected, const FCPAugmentData&, SelectedAugment);

/**
 *  A single clickable augment card: a button showing the augment's name/description text.
 *  No image design is required - place a plain Button (with a TextBlock inside it, or overlaid)
 *  in the inheriting Widget Blueprint.
 */
UCLASS(abstract)
class CP_API UCPAugmentCardWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The button that makes the whole card clickable */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* CardButton;

	/** Displays the augment's name and description */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* CardText;

	/** The augment this card currently represents */
	UPROPERTY(BlueprintReadOnly, Category="Augment")
	FCPAugmentData AugmentData;

public:

	/** Broadcast when the card is clicked */
	UPROPERTY(BlueprintAssignable, Category="Augment")
	FOnCPAugmentCardSelected OnCardSelected;

	/** Assigns the augment this card represents and refreshes its display text */
	UFUNCTION(BlueprintCallable, Category="Augment")
	void SetAugmentData(const FCPAugmentData& InAugmentData);

protected:

	/** Gameplay initialization */
	virtual void NativeConstruct() override;

	/** Bound to CardButton's OnClicked */
	UFUNCTION()
	void HandleClicked();
};
