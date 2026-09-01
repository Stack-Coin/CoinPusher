// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/CPItemTypes.h"
#include "Engine/TimerHandle.h"
#include "CPItemToastWidget.generated.h"

class UTextBlock;

/**
 *  Simple toast that shows an acquired item's name and description for a fixed duration,
 *  then hides itself. Showing a new item while one is already visible restarts the timer
 *  with the new item's text instead of stacking or being ignored.
 */
UCLASS(abstract)
class CP_API UCPItemToastWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* ItemNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* ItemDescriptionText;

	/** How long the toast stays visible before automatically hiding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toast", meta = (ClampMin = 0, Units = "s"))
	float ToastDisplayDuration = 3.0f;

	/** Timer used to automatically hide the toast */
	FTimerHandle HideTimerHandle;

public:

	/** Updates the toast's text and shows it, restarting the display timer even if already visible */
	UFUNCTION(BlueprintCallable, Category="Toast")
	void ShowToast(const FCPItemData& ItemData);

protected:

	/** Gameplay initialization */
	virtual void NativeConstruct() override;

	/** Bound to the hide timer */
	void HideToast();
};
