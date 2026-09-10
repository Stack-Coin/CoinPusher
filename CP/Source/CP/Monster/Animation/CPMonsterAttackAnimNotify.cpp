// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Animation/CPMonsterAttackAnimNotify.h"
#include "Monster/CPMonsterAttackInterface.h"

UCPMonsterAttackAnimNotify::UCPMonsterAttackAnimNotify()
{
}

void UCPMonsterAttackAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// [임시 디버그] 이 노티파이가 실제로 몽타주 재생 중 호출되는지, 어떤 몽타주에서 호출되는지 확인용 -
	// 보스 전용 몽타주(Slam/Attack)에 이 노티파이가 안 박혀있으면 여기 로그 자체가 안 찍힘
	UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] AnimNotify 호출됨 - Owner=%s, Animation=%s"),
		MeshComp && MeshComp->GetOwner() ? *MeshComp->GetOwner()->GetName() : TEXT("NULL"),
		Animation ? *Animation->GetName() : TEXT("NULL"));

	if (ICPMonsterAttackInterface* Monster = Cast<ICPMonsterAttackInterface>(MeshComp->GetOwner()))
	{
		Monster->AttackHitCheck();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[임시 디버그] AnimNotify - Owner=%s가 ICPMonsterAttackInterface로 캐스트 실패"),
			MeshComp && MeshComp->GetOwner() ? *MeshComp->GetOwner()->GetName() : TEXT("NULL"));
	}
}
