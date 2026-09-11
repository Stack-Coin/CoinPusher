#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/Inventory/CPSlotInventory.h"
#include "CPInventoryComponent.generated.h"

class ACPCoinPusher;
struct FItemData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CP_API UCPInventoryComponent : public UActorComponent, public ICPSlotInventory
{
	GENERATED_BODY()

public:
	UCPInventoryComponent();

protected:

	static constexpr int32 NumSlots = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta = (EditFixedSize))
	TArray<FCPInventorySlot> Slots;

	/** Owner의 ACPCoinPusher 참조. BeginPlay에서 캐싱해서 StoreItem()의 ItemDataTable 조회에도
	 *  재사용한다 (LinkedRoulette 구독에 쓰던 지역 변수를 멤버로 승격) */
	UPROPERTY()
	TObjectPtr<ACPCoinPusher> CoinPusher;

	/** GetOwner()를 ACPPlayerCharacter로 캐스팅해 그 CoinPusher(->GetCoinPusher())를 캐싱하고,
	 *  그 LinkedRoulette(->GetLinkedRoulette())를 찾아 OnPickedUp에 HandleRoulettePickedUp()을
	 *  등록해 룰렛에서 뽑힌 아이템이 자동으로 이 인벤토리에 쌓이도록 한다 */
	virtual void BeginPlay() override;

	/** LinkedRoulette::OnPickedUp에 등록되는 핸들러. OnPickedUp은 void(FName, int32) 시그니처의
	 *  다이나믹 델리게이트라 bool을 반환하는 StoreItem()을 AddDynamic으로 직접 등록할 수 없으므로,
	 *  이 함수가 그 시그니처를 맞춰주고 내부에서 StoreItem(ItemID, Count)을 호출한다
	 *  (ACPCoinPusher::HandleRoulettePickedUp과 동일한 이유의 동일한 패턴) */
	UFUNCTION()
	void HandleRoulettePickedUp(FName ItemID, int32 Count);

public:

	/** CoinPusher->GetItemDataTable()에서 ItemID 행을 조회 (없으면 nullptr). 슬롯은 ItemID만
	 *  들고 있으므로, 이름/아이콘/효과 등이 필요한 곳(StoreItem, UseSlotItem, 인벤토리 위젯)은
	 *  전부 이 헬퍼로 ItemDataTable을 원본 삼아 조회한다 */
	const FItemData* FindItemData(FName ItemID) const;

	virtual bool StoreItem(FName ItemCode, int32 Count) override;

	virtual bool RemoveItem(FName ItemCode, int32 Count) override;

	virtual int32 GetItemCount(FName ItemCode) const override;

	virtual bool UseSlotItem(int32 SlotIndex) override;

	virtual const TArray<FCPInventorySlot>& GetSlots() const override { return Slots; }

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnCPInventoryChanged OnInventoryChanged;

	/** UseSlotItem()이 아이템을 실제로 소모할 때마다 Broadcast (ItemID, Count) */
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnCPInventoryItemUsed OnItemUsed;
};
