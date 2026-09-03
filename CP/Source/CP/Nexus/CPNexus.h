// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Player/CPInteractable.h"
#include "CPNexus.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UCPUserWidget_NexusHpBar;

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

	/** 이미 죽은(파괴된 척 하는) 넥서스인지. 죽어도 Destroy()하지 않고 메시는 남겨두므로, AI가 다시 이 넥서스를 타겟으로 고르지 않도록 이 값으로 걸러야 합니다. */
	bool IsDead() const { return bIsDead; }

	/** 게임 전체에서 넥서스가 재스폰될 수 있는 총 횟수를 초기값으로 되돌립니다. 플레이 시작 시 GameMode가 딱 한 번 호출합니다. */
	static void ResetGlobalRespawnBudget() { GlobalRemainingRespawns = 2; }

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HpBar;

	UPROPERTY(Transient)
	TObjectPtr<UCPUserWidget_NexusHpBar> HpBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HealAmount = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float RespawnOffsetDistance = 1000.f;

	// 넥서스가 몇 개든, 게임 전체에서 재스폰 가능한 총 남은 횟수 (모든 넥서스가 공유하는 스태틱 변수)
	static int32 GlobalRemainingRespawns;

	float MaxHp = 100.f;
	float CurrentHp = 100.f;

	// 죽은 뒤에도 메시는 화면에 남겨둬야 해서 Destroy()하지 않으므로, Dead()가 중복 실행되지 않도록 막는 플래그
	bool bIsDead = false;
};
