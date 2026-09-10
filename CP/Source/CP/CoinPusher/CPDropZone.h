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

	/** Every time this many coins have been collected here in total, the team (ACPGameMode) is granted 1 ticket */
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

	//������ ���� ���� ��ȯ
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	int32 GetCollectedCoinCount() const { return CollectedCoinCount; }

	//������ ������ �ڵ� ��� ��ȯ
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	const TArray<FName>& GetCollectedItemCodes() const { return CollectedItemCodes; }

	//ICPCoinPusherItem 구현체(Coin)가 호출 - 수집 개수를 늘리고 BroadCast + GameMode로 드랍 정보 전달.
	//ItemID를 함께 넘기면(코인은 항상 넘김) GameMode->ReceiveDroppedItem(ItemID, Amount, CoinType) 호출
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void AddCollectedCoins(int32 Amount = 1, FName ItemID = NAME_None, ECPCoinType CoinType = ECPCoinType::Normal);

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
