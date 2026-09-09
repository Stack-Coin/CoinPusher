#include "UI/CPInventoryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Player/CPPlayerCharacter.h"
#include "Player/Inventory/CPInventoryComponent.h"

void UCPInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (UCPInventoryComponent* Inventory = PlayerCharacter->GetInventoryComponent())
		{
			BoundInventory = Inventory;
			Inventory->OnInventoryChanged.AddDynamic(this, &UCPInventoryWidget::RefreshSlots);
		}
	}

	RefreshSlots();
}

void UCPInventoryWidget::RefreshSlots()
{
	SetSlotDisplay(EastSlotImage, EastSlotCountText, 0);
	SetSlotDisplay(NorthSlotImage, NorthSlotCountText, 1);
	SetSlotDisplay(WestSlotImage, WestSlotCountText, 2);
	SetSlotDisplay(SouthSlotImage, SouthSlotCountText, 3);
}

void UCPInventoryWidget::SetSlotDisplay(UImage* SlotImage, UTextBlock* SlotCountText, int32 SlotIndex) const
{
	UCPInventoryComponent* Inventory = BoundInventory.Get();
	const TArray<FCPInventorySlot>* Slots = Inventory ? &Inventory->GetSlots() : nullptr;
	const FCPInventorySlot* InventorySlot = (Slots && Slots->IsValidIndex(SlotIndex)) ? &(*Slots)[SlotIndex] : nullptr;

	const bool bHasItem = InventorySlot && !InventorySlot->IsEmpty() && InventorySlot->Item.Icon;

	if (SlotImage)
	{
		if (bHasItem)
		{
			SlotImage->SetBrushFromTexture(InventorySlot->Item.Icon, false);
			SlotImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SlotImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (SlotCountText)
	{
		if (bHasItem)
		{
			SlotCountText->SetText(FText::AsNumber(InventorySlot->Count));
			SlotCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SlotCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
