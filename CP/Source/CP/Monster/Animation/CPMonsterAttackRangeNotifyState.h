// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CPMonsterAttackRangeNotifyState.generated.h"

class UMaterialInterface;

UENUM(BlueprintType)
enum class ECPAttackRangeIndicatorShape : uint8
{
	/** 정면 스윕(ACPMonsterBase::GetAttackSweepShape) 모양 - 직사각형 */
	Sweep,
	/** 자기 위치 중심 원형 AOE(ACPMonsterBase::GetAIAOERadius) 모양 */
	Circle,
};

/**
 * 몽타주의 윈드업 구간(공격 판정 노티파이보다 앞)에 걸어두는 공격범위 표시 노티파이 스테이트.
 * NotifyBegin에서 데칼을 스폰하고 NotifyEnd에서 지움 - 크기는 항상 실제 판정 로직
 * (GetAttackSweepShape/GetAIAOERadius)에서 그대로 읽어오므로 사거리 값이 바뀌어도 자동으로 맞음.
 *
 * 주의: NotifyState 객체는 몽타주 애셋에 딸린 공유 인스턴스라 여러 몬스터가 동시에 재생해도
 * 하나만 존재함 - 그래서 스폰한 데칼을 멤버 변수로 들고 있지 않고, 태그로 찾아서 지움.
 */
UCLASS()
class CP_API UCPMonsterAttackRangeNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indicator")
	ECPAttackRangeIndicatorShape Shape = ECPAttackRangeIndicatorShape::Sweep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indicator")
	TObjectPtr<UMaterialInterface> IndicatorMaterial;

	/** 데칼 투영 깊이(cm) - 지면을 확실히 뚫고 지나가도록 여유 있게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indicator")
	float DecalDepth = 50.f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	static const FName DecalComponentTag;
};
