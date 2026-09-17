// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherItem.h"
#include "CPItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UDataTable;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ACPCoinPusher;
class UCPCoinPusherViewCaptureComponent;
class USoundBase;
class UPrimitiveComponent;

/**
 *  A physics-simulated prize item. Dispensed by ACPDispenser like a coin, but when collected
 *  by ACPDropZone it records its ItemCode instead of incrementing the coin count.
 */
UCLASS(abstract)
class CP_API ACPItem : public AActor, public ICPCoinPusherItem
{
	GENERATED_BODY()

	/** Physics collision shape and RootComponent. A sphere so the item rolls naturally */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Visual sphere mesh, no collision - purely cosmetic. ApplyItemData() swaps its StaticMesh/
	 *  Material(slot 0) from FItemData::ItemMesh/ItemMaterial */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** 3D 메시(Mesh) 대신 평면(Billboard)으로 표시할 때 쓰는 서브 메시(보통 BP에서 사각 플레인을
	 *  StaticMesh로 지정) - Mesh의 "ImagePoint" 소켓(Mesh에 지정된 StaticMesh 에셋에 정의돼 있어야
	 *  함)에 부착되어, 그 소켓 위치가 곧 이 사각 플레인의 중심이 된다. ApplyItemData()가
	 *  BillboardMaterial의 Dynamic Material Instance를 만들어 여기 적용하고, FItemData::ItemImage가
	 *  있을 때만 보이게 한다. 콜리전 없음, 순수 비주얼 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ImageMesh;

public:

	/** Constructor */
	ACPItem();

protected:

	/** Identifies which item this is */
	UPROPERTY(EditAnywhere, Category="Item")
	FName ItemId;

	/** ItemId로 FItemData 행을 조회할 때 사용하는 데이터 테이블 (Row Struct는 FItemData여야 함) -
	 *  ACPDispenser::ItemDataTable 등과 같은 에셋을 공유해서 지정하면 된다 */
	UPROPERTY(EditAnywhere, Category="Item")
	TObjectPtr<UDataTable> ItemDataTable;

	/** ImageMesh에 적용할 베이스 Billboard Material - ApplyItemData()가 이로부터 Dynamic Material
	 *  Instance를 만들어 ItemTextureParameterName 파라미터를 FItemData::ItemImage로 갱신한다 */
	UPROPERTY(EditAnywhere, Category="Item")
	TObjectPtr<UMaterialInterface> BillboardMaterial;

	/** BillboardMaterial의 텍스처 파라미터 중, 아이템 이미지를 나타내는 파라미터 이름. Material
	 *  쪽 파라미터 이름과 반드시 일치해야 한다 */
	UPROPERTY(EditAnywhere, Category="Item")
	FName ItemTextureParameterName = TEXT("ItemTexture");

	/** BillboardMaterial로부터 만든 Dynamic Material Instance. ApplyItemData()에서 한 번만
	 *  생성해서 캐싱해두고, 이후로는 텍스처 파라미터 값만 갱신한다 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ImageMaterialInstance;

	/** If true, this item has already been collected and is awaiting destruction */
	bool bCollected = false;

	/** Launch() 이후 이 시간(초) 동안은 bIsLaunched가 true로 유지되어 추가 Launch() 호출을 무시한다 -
	 *  ACPCoin::LaunchCooldown과 동일한 목적으로, CoinThrowArea 등이 같은 아이템을 중복으로 발사해
	 *  속도가 비정상적으로 누적되는 것을 막기 위함 */
	UPROPERTY(EditAnywhere, Category="Item", meta = (ClampMin = 0))
	float LaunchCooldown = 1.0f;

	/** Launch()로 날아가고 있는 중이면 true. LaunchCooldown 경과 후 자동으로 다시 false로 돌아와
	 *  재발사 가능해진다 (ACPCoin::bIsLaunched와 동일) */
	bool bIsLaunched = false;

	/** bIsLaunched를 다시 false로 되돌리는 타이머 핸들 */
	FTimerHandle LaunchCooldownTimerHandle;

	/** LaunchCooldownTimerHandle 만료 시 호출되어 bIsLaunched를 다시 false로 되돌림 (재발사 가능 상태로 복귀) */
	void ClearLaunchedState();

	/** 이 아이템이 코인 푸셔 안에서 무엇이든 처음 부딪혔을 때(=들어간 순간) 재생할 사운드 */
	UPROPERTY(EditAnywhere, Category="Sound")
	TObjectPtr<USoundBase> EnterSound;

	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0))
	float EnterSoundVolume = 1.0f;

	/** 스폰(BeginPlay) 후 이 시간(초)이 지나야 EnterSound 트리거가 활성화된다 - 스폰 직후 디스펜서 등과의
	 *  초기 접촉으로 곧바로 오발동하는 것을 방지 (ACPCoin::BigWaveThrowArmDelay와 동일한 목적) */
	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0))
	float EnterSoundArmDelay = 0.05f;

	/** EnterSoundArmDelay가 지나 EnterSound 트리거가 활성화되면 true */
	bool bEnterSoundArmed = false;

	/** EnterSound를 이미 재생했으면 true - 중복 재생 방지 가드 */
	bool bHasPlayedEnterSound = false;

	/** bEnterSoundArmed를 true로 바꾸는 타이머 핸들 */
	FTimerHandle EnterSoundArmTimerHandle;

	/** ImageMesh(빌보드 평면)가 카메라를 바라보도록 계산한 LookAt 회전에 추가로 더해줄 보정값 -
	 *  플레인 메시 에셋의 정면(텍스처가 그려지는 면)이 로컬 +X가 아닌 다른 축을 향하고 있으면
	 *  Yaw 180 등으로 보정해서 실제로 카메라 쪽을 보이는 면이 맞게 나오도록 조정한다 */
	UPROPERTY(EditAnywhere, Category="Item")
	FRotator BillboardRotationOffset = FRotator::ZeroRotator;

	/** 레벨에서 찾은 ACPCoinPusher를 최초 1회 캐싱 (GetCoinPusher 참고) - UCPCoinPointUI와 동일한
	 *  방식으로 ImageMesh가 바라봐야 할 Screen Capture 카메라를 얻는 데 쓰인다 */
	mutable TWeakObjectPtr<ACPCoinPusher> CachedCoinPusher;

	/** GetCoinPusher()의 Screen Capture 카메라(ViewCaptureComponent)를 최초 1회 캐싱 (GetCaptureComponent 참고) */
	mutable TWeakObjectPtr<UCPCoinPusherViewCaptureComponent> CachedCaptureComponent;

	/** ItemDataTable에서 ItemId로 FItemData 행을 찾아, Mesh의 StaticMesh/Material(슬롯 0/슬롯 1)을
	 *  FItemData::ItemMesh/ItemMaterial/ItemMaterial2로 갱신하고, FItemData::ItemImage가 있으면
	 *  ImageMesh에 BillboardMaterial 기반 Dynamic Material Instance를 만들어(최초 1회)
	 *  ItemTextureParameterName 텍스처 파라미터를 그 이미지로 설정한 뒤 ImageMesh를 보이게
	 *  한다(ItemImage가 없으면 ImageMesh를 숨김). ImageMesh가 보이는 동안에만 Tick으로 카메라를
	 *  바라보도록 켜고(SetActorTickEnabled), 숨겨지면 다시 끈다. ItemDataTable이 없거나 ItemId에
	 *  해당하는 행이 없으면 아무 동작도 하지 않는다 */
	virtual void BeginPlay() override;
	void ApplyItemData();

	/** ImageMesh가 보이는 동안만 매 틱마다 GetCaptureComponent()(CoinPusher의 Screen Capture 카메라)
	 *  쪽을 바라보도록 ImageMesh의 WorldRotation을 갱신한다(BillboardRotationOffset 만큼 추가 보정) -
	 *  카메라를 못 찾으면 아무 동작도 하지 않는다 */
	virtual void Tick(float DeltaTime) override;

	/** 레벨에 배치된 ACPCoinPusher를 찾아 캐싱한다(최초 1회만 탐색). 찾지 못하면 nullptr */
	ACPCoinPusher* GetCoinPusher() const;

	/** GetCoinPusher()의 GetViewCaptureComponent()(Screen Capture 카메라)를 반환하고 캐싱한다
	 *  (최초 1회만 탐색). 찾지 못하면 nullptr */
	UCPCoinPusherViewCaptureComponent* GetCaptureComponent() const;

	/** EnterSoundArmTimerHandle 만료 시 호출되어 bEnterSoundArmed를 true로 설정 */
	void ArmEnterSound();

	/** CollisionSphere->OnComponentHit에 바인딩 - 무엇과든(바닥/벽 포함) 처음 부딪히면(단,
	 *  bEnterSoundArmed가 true인 이후) EnterSound를 1회 재생한다 */
	UFUNCTION()
	void HandleCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:

	/** Returns this item's identifying code */
	UFUNCTION(BlueprintPure, Category="Item")
	FName GetItemId() const { return ItemId; }

	/** ItemId를 NewItemId로 바꾸고 ApplyItemData()를 다시 호출해 Mesh/Material/Image를 그 값에 맞게
	 *  갱신한다 - BP Class Defaults에 고정된 ItemId만으로는 스폰 시점에 실제로 어떤 ItemID로
	 *  스폰됐는지 반영할 수 없으므로, ACPDispenser::SpawnFromItemData()가 스폰 직후 이 함수로 실제
	 *  ItemID를 전달한다(같은 값이면 아무 것도 하지 않음) */
	UFUNCTION(BlueprintCallable, Category="Item")
	void SetItemId(FName NewItemId);

	/** EnterSound가 비어있을 때만(=이 아이템 BP가 직접 지정해두지 않았을 때만) NewEnterSound로 채운다 -
	 *  ACPDispenser::SpawnFromItemData()가 스폰 직후 CoinPusher의 공용 ItemEnterSound를 넘겨줄 때 사용.
	 *  이미 값이 있으면(아이템 BP가 개별 지정) 덮어쓰지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Item")
	void SetEnterSound(USoundBase* NewEnterSound, float NewEnterSoundVolume = 1.0f);

	/** Called by ACPDropZone once it has already recorded this item (RecordCollectedItem) - plays the BP
	 *  collection effect and destroys this actor. Returns false (and does nothing else) if this item was
	 *  already collected, so a duplicate call can't double up */
	UFUNCTION(BlueprintCallable, Category="Item")
	bool Collect();

	/** 발사 실행 - CollisionSphere에 LaunchVelocity를 물리 속도로 부여한다. 이미 발사되어 날아가고
	 *  있는 중이면(LaunchCooldown 이내) 무시한다. ACPCoin::Launch()와 동일한 동작이라, CPCoinThrowArea가
	 *  Coin과 동일하게 Item도 발사할 수 있다 */
	UFUNCTION(BlueprintCallable, Category="Item")
	void Launch(const FVector& LaunchVelocity);

	// ~begin ICPCoinPusherItem interface

	/** Calls Collect() (DropZone already read GetItemCode() and recorded this item directly at the
	 *  overlap, so this no longer reports anything back to it) */
	virtual void OnDroppedInZone(ACPDropZone* DropZone) override;

	// ~end ICPCoinPusherItem interface

protected:

	/** Passes control to BP to play effects on collection */
	UFUNCTION(BlueprintImplementableEvent, Category="Item", meta = (DisplayName = "On Collected"))
	void BP_OnCollected();

public:

	/** Returns the CollisionSphere subobject */
	FORCEINLINE USphereComponent* GetCollisionSphere() const { return CollisionSphere; }

	/** Returns the Mesh subobject */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Returns the ImageMesh(Billboard) subobject */
	FORCEINLINE UStaticMeshComponent* GetImageMesh() const { return ImageMesh; }
};
