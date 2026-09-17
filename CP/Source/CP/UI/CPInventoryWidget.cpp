#include "UI/CPInventoryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Player/CPPlayerCharacter.h"
#include "Player/Inventory/CPInventoryComponent.h"
#include "Datatables/CPItemData.h"
#include "TimerManager.h"

void UCPInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCPInventoryWidget::BindToPlayerInventory));
}

void UCPInventoryWidget::BindToPlayerInventory()
{
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
	SetSlotDisplay(EastSlotImage, EastSlotCountText, 2);
	SetSlotDisplay(NorthSlotImage, NorthSlotCountText, 1);
	SetSlotDisplay(WestSlotImage, WestSlotCountText, 0);
	SetSlotDisplay(SouthSlotImage, SouthSlotCountText, 3);
}

void UCPInventoryWidget::SetSlotDisplay(UImage* SlotImage, UTextBlock* SlotCountText, int32 SlotIndex) const
{
	UCPInventoryComponent* Inventory = BoundInventory.Get();
	const TArray<FCPInventorySlot>* Slots = Inventory ? &Inventory->GetSlots() : nullptr;
	const FCPInventorySlot* InventorySlot = (Slots && Slots->IsValidIndex(SlotIndex)) ? &(*Slots)[SlotIndex] : nullptr;

	// 슬롯은 ItemID가 항상 배정되어 있고(EditFixedSize, BP에서 지정) Count만 0~n으로 오르내리므로,
	// 아이콘/수량 텍스트 모두 Count와 무관하게 ItemID 배정 여부만으로 표시한다 - 수량이 0이어도 이
	// 슬롯이 어떤 아이템 슬롯이고 몇 개 있는지("0") 계속 보여주기 위함
	const bool bSlotHasItemID = InventorySlot && !InventorySlot->ItemID.IsNone();
	const FItemData* ItemData = (bSlotHasItemID && Inventory)
		? Inventory->FindItemData(InventorySlot->ItemID)
		: nullptr;
	const bool bHasIcon = ItemData && ItemData->InventoryIcon;

	if (SlotImage)
	{
		if (bHasIcon)
		{
			SlotImage->SetBrushFromTexture(ItemData->InventoryIcon, false);
		}

		// ItemDataTable에 아이콘 자체가 없는 경우에만 투명하게 - 슬롯 프레임/레이아웃과 상호작용(호버,
		// 드래그앤드롭 등)은 그대로 유지한 채 아이콘만 안 보이게 함
		SlotImage->SetRenderOpacity(bHasIcon ? 1.0f : 0.0f);
		SlotImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (SlotCountText)
	{
		if (bSlotHasItemID)
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
