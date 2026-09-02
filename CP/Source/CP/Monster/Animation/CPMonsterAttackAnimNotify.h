// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CPMonsterAttackAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPMonsterAttackAnimNotify : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UCPMonsterAttackAnimNotify();

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
