// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinTypes.h"
#include "CPDropZone.generated.h"

class UBoxComponent;
class ACPDispenser;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinCollected, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCollected, FName, ItemCode);

/** DropZone에 아이템(코인 포함)이 떨어질 때마다 ItemID만 실어 Broadcast하는 범용 알림용 델리게이트.
 *  OnCoinCollected/OnItemCollected와 달리 코인/아이템 구분 없이 "무엇이 떨어졌는지"만 알려준다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPDropZoneDropped, FName, ItemID);

/** 코인/아이템이 이 DropZone에 떨어져 수집될 때마다, 그 코인/아이템의 ItemID/RowName과 떨어진
 *  월드 위치를 함께 Broadcast하는 델리게이트(이름은 OnCoinDropped이지만 RecordCollectedItem에서도
 *  함께 쓰임). UI/CPCoinPointUI.h의 UCPCoinPointUI::ShowCoinPointText(FName, FVector)와
 *  시그니처가 같아 그대로 Bind Event해서 ItemDataTable의 CoinPointText를 그 위치에 잠깐 띄우는
 *  용도로 쓸 수 있다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPCoinDropped, FName, ItemID, FVector, WorldLocation);

/** 콤보 수가 바뀔 때마다(증가 또는 0으로 리셋) Broadcast. UI/CPInGameWidget.h의
 *  UCPInGameWidget::SetComboCount(int32)와 시그니처가 같다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPComboCountChanged, int32, NewComboCount);

/** 콤보 게이지 값이 바뀔 때마다 Broadcast. CurrentValue는 콤보가 끊기기까지 남은 시간(초),
 *  MaxValue는 ComboWindowSeconds. UI/CPInGameWidget.h의
 *  UCPInGameWidget::UpdateComboGauge(float, float)와 시그니처가 같다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPComboGaugeChanged, float, CurrentValue, float, MaxValue);


UCLASS(abstract)
class CP_API ACPDropZone : public AActor
{
	GENERATED_BODY()

	// 드랍 존
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollectionVolume;

public:

	ACPDropZone();

	/** 콤보가 진행 중인 동안(ComboCount > 0)에만 매 틱마다 UpdateComboGaugeDisplay()를 호출해
	 *  게이지가 부드럽게 줄어드는 것을 보여준다 - RegisterComboHit()이 켜고, HandleComboWindowExpired()가
	 *  콤보가 끊길 때 끈다(평소에는 틱이 꺼져 있어 불필요한 매 프레임 연산이 없음) */
	virtual void Tick(float DeltaTime) override;

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

	/** 콤보가 유지되려면 이전 드랍 이후 이 시간(초) 안에 다음 코인/아이템이 떨어져야 한다 - 넘기면
	 *  콤보가 끊기고(ComboCount 0으로 리셋) 다음 드랍부터 다시 1콤보로 시작한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combo", meta = (ClampMin = 0))
	float ComboWindowSeconds = 1.5f;

	/** 현재 콤보 수. 코인/아이템이 하나 떨어질 때마다 1 증가하고, ComboWindowSeconds 안에 다음
	 *  드랍이 없으면 0으로 리셋된다(HandleComboWindowExpired) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combo")
	int32 ComboCount = 0;

	/** 가장 최근 콤보 히트가 발생한 월드 시간(GetWorld()->GetTimeSeconds()) - 콤보 게이지의 남은
	 *  시간을 계산하는 기준점 */
	float ComboWindowStartTime = -1.0f;

	/** ComboWindowSeconds 후 만료되어 HandleComboWindowExpired를 호출하는 타이머 - 매 콤보 히트마다
	 *  새로 시작되므로, 그 안에 다음 히트가 들어오면 자연스럽게 "시간 초기화"가 된다 */
	FTimerHandle ComboWindowTimerHandle;

	/** Normal/Passive/HP 코인이 DropZone에 떨어져 수집될 때마다 이 중 하나를 균등 확률로 랜덤 재생
	 *  (PlayRandomItemDropSound) - Big/Monster/CoinTower/일반 아이템(ACPItem)은 대상 아님. 개수 제한
	 *  없이 채워도 됨, 몇 개든 같은 확률로 뽑힘 */
	UPROPERTY(EditAnywhere, Category="Sound")
	TArray<TObjectPtr<USoundBase>> ItemDropSounds;

	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0))
	float ItemDropSoundVolume = 1.0f;

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

	/** 콤보 수가 바뀔 때마다(증가 또는 0으로 리셋) Broadcast - InGameUI의 CoinComboWidget 연동에 사용 */
	UPROPERTY(BlueprintAssignable, Category="Combo")
	FOnCPComboCountChanged OnComboCountChanged;

	/** 콤보 게이지 값이 바뀔 때마다 Broadcast(CurrentValue=남은 시간, MaxValue=ComboWindowSeconds) -
	 *  InGameUI의 CoinComboWidget 게이지 연동에 사용 */
	UPROPERTY(BlueprintAssignable, Category="Combo")
	FOnCPComboGaugeChanged OnComboGaugeChanged;

	//������ ���� ���� ��ȯ
	UFUNCTION(BlueprintPure, Category="Drop Zone")
	int32 GetCollectedCoinCount() const { return CollectedCoinCount; }

	/** 현재 콤보 수를 반환 */
	UFUNCTION(BlueprintPure, Category="Combo")
	int32 GetComboCount() const { return ComboCount; }

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
	//재생성 요청 + GameMode로 드랍 정보 전달. WorldLocation은 그 아이템이 떨어진 위치(보통 호출부의
	//GetActorLocation())로, 코인과 마찬가지로 OnCoinDropped 델리게이트에 실려 Broadcast된다(코인
	//전용이 아니라 "무언가 떨어진 위치" 델리게이트로 함께 쓰임 - CoinPointUI 등 위치 기반 UI 연출용)
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void RecordCollectedItem(FName ItemCode, FVector WorldLocation = FVector::ZeroVector);

	//������(ACPCoinPusher)�� ȣ�� - ������ ��� �� ������� ���� Dispenser�� ����
	UFUNCTION(BlueprintCallable, Category="Drop Zone")
	void SetItemRespawnDispenser(ACPDispenser* NewItemRespawnDispenser);

protected:

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 코인/아이템이 하나 떨어질 때마다 AddCollectedCoins/RecordCollectedItem에서 호출 - ComboCount를
	 *  1 증가시키고 OnComboCountChanged를 Broadcast한 뒤, ComboWindowTimerHandle을 ComboWindowSeconds로
	 *  새로 시작한다(그 안에 다음 히트가 들어오면 여기서 다시 새로 시작되므로 자연스럽게 "시간
	 *  초기화"가 됨). SetActorTickEnabled(true)로 매 틱 게이지 갱신을 켠다 */
	void RegisterComboHit();

	/** ComboWindowTimerHandle 만료 시 호출 - ComboWindowSeconds 안에 다음 드랍이 없어 콤보가 끊긴
	 *  것으로 보고 ComboCount를 0으로 리셋하고 OnComboCountChanged/OnComboGaugeChanged(0)를 Broadcast한
	 *  뒤 SetActorTickEnabled(false)로 매 틱 게이지 갱신을 끈다 */
	void HandleComboWindowExpired();

	/** Tick()에서 매 프레임 호출되어, ComboWindowStartTime 기준 남은 시간을 계산해 OnComboGaugeChanged로
	 *  Broadcast한다 - 게이지가 매 틱 눈에 보이게 줄어드는 이유 */
	void UpdateComboGaugeDisplay();

	/** ItemDropSounds 중 하나를 FMath::RandRange로 균등하게 골라 Location에서 재생 (비어있으면 아무것도 안 함).
	 *  OnVolumeBeginOverlap이 Normal/Passive/HP 코인 수집 시에만 호출 */
	void PlayRandomItemDropSound(const FVector& Location) const;

public:

	FORCEINLINE UBoxComponent* GetCollectionVolume() const { return CollectionVolume; }

	FORCEINLINE ACPDispenser* GetItemRespawnDispenser() const { return ItemRespawnDispenser; }
};
