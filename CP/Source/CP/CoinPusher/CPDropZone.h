// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	//ICPCoinPusherItem ����ü(Coin)�� ȣ�� - ���� ������ �ø��� BroadCast
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void AddCollectedCoins(int32 Amount = 1);

	//ICPCoinPusherItem ����ü(Item)�� ȣ�� - ������ �ڵ带 ����ϰ� BroadCast + ItemRespawnDispenser�� ����� ��û
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
