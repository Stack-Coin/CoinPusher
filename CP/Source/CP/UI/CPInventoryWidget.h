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

	/** NativeConstruct 한 틱 뒤 호출 - 소유 Pawn의 InventoryComponent를 찾아 OnInventoryChanged를
	 *  구독한다. 이 위젯은 ACPTopDownPlayerController::BeginPlay()가 InGameWidget을 생성하며 함께
	 *  NativeConstruct되는데, 액터 간 BeginPlay 순서(Controller vs PlayerCharacter)가 보장되지
	 *  않아 그 시점엔 아직 소유 Pawn/InventoryComponent가 준비되지 않았을 수 있다
	 *  (ACPGameMode::SetupPlayerInGameWidgetBindings와 동일한 이유로 동일하게 한 틱 지연) */
	void BindToPlayerInventory();

	UFUNCTION()
	void RefreshSlots();

	void SetSlotDisplay(UImage* SlotImage, UTextBlock* SlotCountText, int32 SlotIndex) const;
};
