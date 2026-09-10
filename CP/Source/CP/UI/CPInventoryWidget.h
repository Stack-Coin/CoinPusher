#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPInventoryWidget.generated.h"

class UImage;
class UTextBlock;
class UCPInventoryComponent;

UCLASS(abstract)
class CP_API UCPInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* EastSlotImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* NorthSlotImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* WestSlotImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* SouthSlotImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* EastSlotCountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* NorthSlotCountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* WestSlotCountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* SouthSlotCountText;

	TWeakObjectPtr<UCPInventoryComponent> BoundInventory;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void RefreshSlots();

	void SetSlotDisplay(UImage* SlotImage, UTextBlock* SlotCountText, int32 SlotIndex) const;
};
