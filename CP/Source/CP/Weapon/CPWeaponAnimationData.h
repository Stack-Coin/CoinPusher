// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CPWeaponAnimationData.generated.h"

class UAnimMontage;
class UAnimationAsset;

/**
 *  CPWeaponAnimationData
 *  Animation set for a single weapon (attack montage + locomotion animations). A weapon owns one of
 *  these and Player Animation Blueprints read it through the weapon (or the weapon manager component)
 *  instead of branching on weapon type, so equipping/swapping automatically changes the played animations.
 */
UCLASS(BlueprintType)
class CP_API UCPWeaponAnimationData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Montage played when the weapon attacks. Play rate is scaled by the weapon's final attack speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Idle animation/blend space used while this weapon is equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	/** Walk animation/blend space used while this weapon is equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimationAsset> WalkAnimation;

	/** Run animation/blend space used while this weapon is equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimationAsset> RunAnimation;
};
