// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPStatInterface.h"
#include "CPItemEffect.generated.h"

class UNiagaraSystem;

/**
 *  Base class for an item's effect. Every current item intentionally uses no effect
 *  (FCPItemData::EffectClass = None). New effects (a move speed buff, an attack power buff,
 *  a heal, etc.) are added as new subclasses that override ApplyEffect - the item pickup and
 *  inventory code never has to change to support a new effect.
 */
UCLASS(Abstract, Blueprintable)
class CP_API UCPItemEffect : public UObject
{
	GENERATED_BODY()

public:

	/** Applies this item's effect to the target. Base implementation intentionally does nothing */
	UFUNCTION(BlueprintCallable, Category="Item")
	virtual void ApplyEffect(TScriptInterface<ICPStatInterface> Target);

protected:

	/** 이 아이템을 획득했을 때(최초 1회) TargetActor에 부착되어 재생할 이펙트. 비워두면 재생하지 않음.
	 *  BP 자식 클래스(BP_Effect_AttackUp 등)마다 다른 값을 지정해 아이템별로 다른 이펙트를 낼 수 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effect")
	TObjectPtr<UNiagaraSystem> PickupEffect;

	/** PickupEffect가 TargetActor의 RootComponent에 부착되어 따라다닐 로컬(상대) 위치 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effect")
	FVector PickupEffectLocationOffset = FVector::ZeroVector;

	/** PickupEffect의 로컬(상대) 회전 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effect")
	FRotator PickupEffectRotationOffset = FRotator::ZeroRotator;

	/** PickupEffect 스폰 스케일 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effect")
	FVector PickupEffectScale = FVector::OneVector;

	/** PickupEffect가 지정되어 있으면 TargetActor의 RootComponent에 부착해(따라다니게) 스폰한다 */
	UFUNCTION(BlueprintCallable, Category="Item|Effect")
	void PlayPickupEffect(AActor* TargetActor) const;
};
