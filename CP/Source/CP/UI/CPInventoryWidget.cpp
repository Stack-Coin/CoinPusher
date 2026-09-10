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
		}

		// 수량이 0(빈 슬롯)이면 완전히 숨기는 대신 투명하게 - 슬롯 프레임/레이아웃과 상호작용(호버, 드래그앤드롭
		// 등)은 그대로 유지한 채 아이콘만 안 보이게 함
		SlotImage->SetRenderOpacity(bHasItem ? 1.0f : 0.0f);
		SlotImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
