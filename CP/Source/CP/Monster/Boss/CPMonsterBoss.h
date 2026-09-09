// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/CPMonsterBase.h"
#include "CPMonsterBoss.generated.h"

/**
 * 플레이어 추적(기존 MoveTo 재사용) + 일반 공격/슬램(내려찍기) + 체력 50% 이하 포효(무적) 패턴을 갖는 보스.
 *
 * BT 그래프(에디터에서 구성):
 *   Selector
 *   |- [Decorator: ShouldRoar]     -> Task: Roar
 *   |- [Decorator: AttackInRange]  -> TurnToTarget -> Task: Attack   (Slam 여부는 AttackByAI 내부에서 판단)
 *   `- Task: MoveTo (추적)
 */
UCLASS()
class CP_API ACPMonsterBoss : public ACPMonsterBase
{
	GENERATED_BODY()

public:
	ACPMonsterBoss();

	/** 스포너가 SpawnBoss()에서 BossWaveTable(FCPBossWaveRow)의 해당 Round 행을 찾은 직후 호출:
	 *  그 행의 RoarHealthPercentThreshold/SlamCooldown 값으로 덮어씀. 호출되지 않으면(레벨에 직접
	 *  배치해서 테스트하는 경우 등) 아래 Blueprint 디테일 패널에 넣어둔 기본값을 그대로 사용함 */
	void ApplyBossWaveStat(float InRoarHealthPercentThreshold, float InSlamCooldown);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void AttackByAI() override;

public:
	/** BT의 ShouldRoar 데코레이터가 매 틱 확인: 무장 상태(bArmedForRoar)이고, 체력비율이 임계치 밑이면 true */
	bool ShouldRoar();

	/** BT의 Roar 태스크가 호출. 포효 몽타주 재생 + 몽타주 길이만큼 무적 부여. 끝나면 델리게이트 실행 */
	void RoarByAI();

	/** BT의 Roar 태스크가 실행 전에 호출해서, 포효(몽타주)가 끝났을 때 알림받을 델리게이트를 등록 */
	void SetRoarDelegate(const FAICharacterAttackFinished& InOnRoarFinished) { OnRoarFinished = InOnRoarFinished; }

protected:
	void HandleRoarMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Roar")
	TObjectPtr<UAnimMontage> RoarMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Slam")
	TObjectPtr<UAnimMontage> SlamMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Roar", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoarHealthPercentThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Slam")
	float SlamCooldown = 4.f;

private:
	FAICharacterAttackFinished OnRoarFinished;
	bool bArmedForRoar = true;
	float LastSlamTime = -1000.f;
};
