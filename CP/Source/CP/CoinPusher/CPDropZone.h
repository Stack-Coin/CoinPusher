// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinTypes.h"
#include "CPDropZone.generated.h"

class UBoxComponent;
class ACPDispenser;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinCollected, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCollected, FName, ItemCode);

/** DropZone에 아이템(코인 포함)이 떨어질 때마다 ItemID만 실어 Broadcast하는 범용 알림용 델리게이트.
 *  OnCoinCollected/OnItemCollected와 달리 코인/아이템 구분 없이 "무엇이 떨어졌는지"만 알려준다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPDropZoneDropped, FName, ItemID);

/** 코인이 이 DropZone에 떨어져 수집될 때마다, 그 코인이 떨어진 월드 위치와 함께 Broadcast하는
 *  델리게이트. UI/CPCoinPointUI.h의 UCPCoinPointUI::ShowCoinPointText(FVector)와 시그니처가 같아
 *  그대로 Bind Event해서 "코인 획득" 안내 문구를 그 위치에 잠깐 띄우는 용도로 쓸 수 있다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPCoinDropped, FVector, WorldLocation);


UCLASS(abstract)
class CP_API ACPDropZone : public AActor
{
	GENERATED_BODY()

	// 드랍 존
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollectionVolume;

public:

	ACPDropZone();

protected:

	//������ ���� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drop Zone")
	int32 CollectedCoinCount = 0;

	/** Team experience (ACPGameMode) granted per coin collected here, multiplied by AddCollectedCoins' Amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone", meta = (ClampMin = 0))
	float ExperiencePerCoin = 1.0f;

	/** Every time this many coins have been collected here in total, the local player is granted 1 ticket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone", meta = (ClampMin = 1))
	int32 CoinsPerTicket = 10;

	//������ �����۵��� Item �ڵ� (������ ������� ���)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drop Zone")
	TArray<FName> CollectedItemCodes;

	//�������� �������� �� ������� ��û�� Dispenser. DropZone�� ACPCoinPusher�� ChildActorComponent��
	//�����ǹǷ� �������� ���� �������� �ʰ�, �������� ACPCoinPusher�� SetItemRespawnDispenser()��
	//���� �����Ѵ�
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Drop Zone")
	TObjectPtr<ACPDispenser> ItemRespawnDispenser;

public:

	//Coin ���� �� BroadCast
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnCoinCollected OnCoinCollected;

	//Item ���� �� BroadCast
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnItemCollected OnItemCollected;

	/** 아이템(코인 포함)이 떨어질 때마다 ItemID와 함께 Broadcast (코인/아이템 종류 구분 없는 범용 알림) */
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnCPDropZoneDropped OnDropped;

	/** 코인이 떨어져 수집될 때마다 그 월드 위치와 함께 Broadcast - CoinPointUI 등 위치 기반 UI 연출에 사용 */
	UPROPERTY(BlueprintAssignable, Category="Drop Zone")
	FOnCPCoinDropped OnCoinDropped;

	//������ ���� ���� ��ȯ
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	int32 GetCollectedCoinCount() const { return CollectedCoinCount; }

	//������ ������ �ڵ� ��� ��ȯ
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	const TArray<FName>& GetCollectedItemCodes() const { return CollectedItemCodes; }

	//ICPCoinPusherItem 구현체(Coin)가 호출 - 수집 개수를 늘리고 BroadCast + GameMode로 드랍 정보 전달.
	//ItemID를 함께 넘기면(코인은 항상 넘김) GetAuthGameMode()가 ICPDroppedItemReceiver를 구현하는
	//경우 ReceiveDroppedItem(ItemID, Amount, CoinType) 호출. WorldLocation은 그 코인이 떨어진 위치
	//(보통 호출부의 GetActorLocation())로, OnCoinDropped 델리게이트에 그대로 실려 Broadcast된다
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void AddCollectedCoins(int32 Amount = 1, FName ItemID = NAME_None, ECPCoinType CoinType = ECPCoinType::Normal, FVector WorldLocation = FVector::ZeroVector);

	//ICPCoinPusherItem 구현체(Item)가 호출 - 아이템 코드를 기록하고 BroadCast + ItemRespawnDispenser에
	//재생성 요청 + GameMode로 드랍 정보 전달
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void RecordCollectedItem(FName ItemCode);

	//������(ACPCoinPusher)�� ȣ�� - ������ ��� �� ������� ���� Dispenser�� ����
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void SetItemRespawnDispenser(ACPDispenser* NewItemRespawnDispenser);

protected:

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	FORCEINLINE UBoxComponent* GetCollectionVolume() const { return CollectionVolume; }

	FORCEINLINE ACPDispenser* GetItemRespawnDispenser() const { return ItemRespawnDispenser; }
};
