// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Player/CPInteractable.h"
#include "Debug/CPDebugTypes.h"
#include "CPNexus.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UCPUserWidget_NexusHpBar;
class UCPDebugCollisionShapeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNexusInteracted, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNexusHpChanged, ACPNexus*, Nexus, float, NewCurrentHp);

UCLASS()
class CP_API ACPNexus : public AActor, public ICPInteractable
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ACPNexus();

	virtual void Tick(float DeltaTime) override;

public:
	//ICPInteractable
	virtual void Interact(AActor* Interactor) override;

	/** Bound to CollisionSphere's OnComponentBeginOverlap. Registers this as an interact target while the player is in range */
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to CollisionSphere's OnComponentEndOverlap. Unregisters this once the player leaves range */
	UFUNCTION()
	void OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	void Dead();
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	float GetMaxHp() const { return MaxHp; }
	float GetCurrentHp() const { return CurrentHp; }

	bool IsDead() const { return bIsDead; }

	// 월드에 있는 Nexus 중 파괴되지 않은 것을 대상으로
	static ACPNexus* FindClosestLivingNexus(const UObject* WorldContextObject, const FVector& FromLocation);

	//static void ResetGlobalRespawnBudget() { GlobalRemainingRespawns = 2; }

protected:
	virtual void BeginPlay() override;
	void UpdateHpBar();

	UFUNCTION(BlueprintImplementableEvent, Category = "Input", meta = (DisplayName = "On Interacted"))
	void BP_OnInteracted(AActor* Interactor);

public:
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FOnNexusInteracted OnInteracted;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnNexusHpChanged OnNexusHpChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collider")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Draws CollisionSphere's wireframe while the F1 debug widget's CoinNexus checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCPDebugCollisionShapeComponent* DebugCollisionShape;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HpBar;

	UPROPERTY(Transient)
	TObjectPtr<UCPUserWidget_NexusHpBar> HpBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HealAmount = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float RespawnOffsetDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHp = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHp = 100.f;

	/** 코인 투입(Interact) 1회당 소모할 코인 수 (상호작용한 플레이어 개인 보유 코인 기준). 코인이
	 *  이 값보다 적으면 상호작용이 무시되고(OnInteracted/BP_OnInteracted도 호출되지 않음) 아무
	 *  일도 일어나지 않는다. 0이면 코인 소모 없이 항상 상호작용됨 (기존 동작) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin", meta = (ClampMin = 0))
	int32 CoinCostPerInteract = 1;

	static int32 GlobalRemainingRespawns;
	bool bIsDead = false;
};
