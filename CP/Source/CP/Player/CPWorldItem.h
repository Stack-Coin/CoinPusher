// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/CPInteractable.h"
#include "Player/CPItemTypes.h"
#include "Debug/CPDebugTypes.h"
#include "CPWorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCPDebugCollisionShapeComponent;
class UNiagaraComponent;

/**
 *  A pickup an ICPInteractor (the player) must walk up to and press the Interact key to collect.
 *  Registers/unregisters itself with any overlapping ICPInteractor purely through interfaces -
 *  it never depends on a concrete player class.
 */
UCLASS(abstract)
class CP_API ACPWorldItem : public AActor, public ICPInteractable
{
	GENERATED_BODY()

	/** 물리 충돌/블로킹을 담당하는 루트 컴포넌트 - 다른 ACPWorldItem의 CollisionBody와 겹치면 물리적으로
	 *  서로 밀어낸다. 중력 비활성화 + Z축 이동/모든 축 회전 잠금(생성자에서 BodyInstance로 설정)으로
	 *  바닥 높이 그대로 XY 평면 위에서만 미끄러지듯 밀려나며, 충돌 반응으로 임의 회전하지 않는다 -
	 *  ItemMesh의 회전/왕복 애니메이션(로컬 좌표만 갱신)과 서로 간섭하지 않는다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionBody;

	/** Detects nearby interactors. CollisionBody에 부착되어, 밀려나도 상호작용 판정 범위가 함께 이동한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

	/** Purely visual, no collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ItemMesh;

	/** Draws InteractionRange's wireframe while the F1 debug widget's ItemPickup checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPDebugCollisionShapeComponent* DebugPickupShape;

	/** 이 아이템이 존재하는 동안 계속 재생되는 후광 이펙트. Niagara System Asset을 비워두면 아무것도
	 *  재생하지 않는다. BP 자식 클래스(BP_CPGrowItem_* 등)마다 이 컴포넌트에서 직접 System Asset과
	 *  상대 위치/회전/스케일(Transform)을 지정하면 된다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* HaloEffectComponent;

protected:

	/** Data for the item this pickup grants */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FCPItemData ItemData;

	/** True once this item has already been collected, to guard against duplicate interactions */
	bool bCollected = false;

	/** ItemMesh가 로컬 Z축 기준으로 회전하는 속도 (초당 각도, deg/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Animation", meta = (ClampMin = 0))
	float RotationSpeed = 45.0f;

	/** ItemMesh가 기준 위치에서 위아래로 오가는 총 거리(cm) - 중심 기준 위/아래로 이 값의 절반씩 움직인다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Animation", meta = (ClampMin = 0, Units = "cm"))
	float BobDistance = 20.0f;

	/** 위아래로 왕복하는 속도 (초당 사이클 수) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Animation", meta = (ClampMin = 0))
	float BobSpeed = 1.0f;

	/** BeginPlay 시점에 한 번만 기록해두는 ItemMesh의 기준 상대 위치/회전 - Tick이 매 프레임 이 값
	 *  기준으로 절대 오프셋을 다시 계산해서 적용한다(ACPPusher와 동일한 패턴 - 오차 누적 방지) */
	FVector StartRelativeLocation = FVector::ZeroVector;
	FRotator StartRelativeRotation = FRotator::ZeroRotator;

	/** 회전/왕복 애니메이션 계산에 쓰는 누적 경과 시간 */
	float ElapsedTime = 0.0f;

	/** 필드(월드)에 드랍된 뒤 이 시간(초)이 지나면 수거되지 않아도 자동으로 사라진다. 0 이하면
	 *  타이머를 걸지 않아 자동으로 사라지지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Lifetime", meta = (ClampMin = 0, Units = "s"))
	float LifetimeSeconds = 30.0f;

	/** LifetimeSeconds 경과 후 HandleLifetimeExpired()를 호출하는 타이머 핸들 */
	FTimerHandle LifetimeExpireTimerHandle;

public:

	/** Constructor */
	ACPWorldItem();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** LifetimeSeconds 타이머 만료 시 호출 - 수거 여부와 무관하게 이 아이템을 Destroy한다 */
	void HandleLifetimeExpired();

	/** Bound to InteractionRange's OnComponentBeginOverlap */
	UFUNCTION()
	void OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to InteractionRange's OnComponentEndOverlap */
	UFUNCTION()
	void OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:

	// ~begin ICPInteractable
	virtual void Interact(AActor* Interactor) override;
	virtual FText GetInteractableDisplayName() const override;
	// ~end ICPInteractable
};
