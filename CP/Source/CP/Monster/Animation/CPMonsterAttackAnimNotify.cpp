// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Animation/CPMonsterAttackAnimNotify.h"
#include "Monster/CPMonsterAttackInterface.h"

UCPMonsterAttackAnimNotify::UCPMonsterAttackAnimNotify()
{
}

void UCPMonsterAttackAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ICPMonsterAttackInterface* Monster = Cast<ICPMonsterAttackInterface>(MeshComp->GetOwner()))
	{
		Monster->AttackHitCheck();
	}
}
