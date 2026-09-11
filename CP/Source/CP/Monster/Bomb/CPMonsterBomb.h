// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/CPMonsterBase.h"
#include "CPMonsterBomb.generated.h"

/**
 * 폭탄형 몬스터 - 날아다니며 투사체 없이 플레이어에게 직접 돌진해서, 닿으면(공격 사거리 안에 들어오면)
 * 터지면서 죽는 자폭형 몬스터.
 *
 * 기존 BT 흐름(Detect → MoveTo → TurnToTarget → AttackInRange 데코레이터 → Attack 태스크)을 그대로
 * 재사용함 - DT_MonsterStat에서 이 타입의 AttackRange를 0(또는 아주 작은 값)으로 설정하면,
 * CPTDecorator_AttackInRange가 "자기 반경 + 타겟 반경 + AttackRange"만큼만 붙었을 때 조건을 통과시켜
 * 주므로 그게 곧 "닿으면"에 해당하는 판정이 됨. 별도의 오버랩/히트 이벤트를 새로 만들 필요가 없음.
 */
UCLASS()
class CP_API ACPMonsterBomb : public ACPMonsterBase
{
	GENERATED_BODY()

public:
	ACPMonsterBomb();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** AttackMontage의 AnimNotify(CPMonsterAttackAnimNotify)에서 호출됨. 부모의 근접 스윕 판정을
	 *  그대로 재사용해 데미지를 준 뒤, 곧바로 자폭(Explode)함 - 투사체를 스폰하지 않음 */
	virtual void AttackHitCheck() override;

	virtual float GetSpawnHeightOffset() const override;

protected:
	/** 데미지 판정 직후 자폭 처리. Dead()를 그대로 호출해서 기존 사망 처리(코인 드랍/DeadMontage
	 *  재생/파괴)를 재사용함 - DeadMontage를 별도로 두지 않고, AttackMontage 자체를 폭발 애니메이션/
	 *  이펙트로 만들어 쓰는 걸 권장함(그러면 폭발 연출이 끝까지 재생된 뒤 자연스럽게 사라짐) */
	virtual void Explode();

protected:
	/** 비행 몬스터라 지면 캡슐 높이 기준 스폰이 의미 없어서, 스포너 위치로부터 항상 이 높이로 스폰됨
	 *  (ACPMonsterRanged와 동일한 이유로 300 미만 권장 - NavMesh 투영 범위를 벗어나면 MoveTo 실패).
	 *  기본값은 생성자(ACPMonsterBomb())에서 설정함 - 여기 인라인 기본값을 고쳐도 이미 저장된
	 *  블루프린트(BP_Bomb)의 Class Defaults 값은 자동으로 안 바뀌니 주의 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	float FlightSpawnHeight;
};
