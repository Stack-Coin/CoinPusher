// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/CPMonsterBase.h"
#include "CPMonsterBoss.generated.h"

/** 보스의 공격이 플레이어에게 실제로 명중했을 때 Broadcast (넥서스를 맞췄거나 빗나간 경우는 제외) -
 *  UCPMonsterSpawnManagerComponent가 구독해서 CoinPusher의 활성 코인을 몬스터 코인으로 전환시키는 데 사용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossAttackedPlayer);

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

	/** 보스 이름 - InGameUI의 BossInfoWidget에 표시하는 용도(UCPMonsterSpawnManagerComponent::SpawnBoss
	 *  참고). 기본값은 아직 이 보스 전용 이름이 기획되지 않아 임시로 넣어둔 "쿠에르노" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	FName BossName = TEXT("쿠에르노");

	FORCEINLINE FName GetBossName() const { return BossName; }

	/** 스포너가 SpawnBoss()에서 RoundInfoTable(FCPMonsterRoundInfoRow)의 해당 Round 행을 찾은 직후 호출:
	 *  그 행의 RoarHealthPercentThreshold/SlamCooldown/SlamRadius/RoarDuration 값으로 덮어쓰고, AddBossMaxHealth/
	 *  AddBossMoveSpeed/AddBossAttackPower/AddBossAttackRange를 (스폰 시 이미 적용된) 기본 스탯 위에 추가로
	 *  더합니다. 보스 관련 라운드 보정치는 DT_RoundStat이 아니라 전부 여기(RoundInfo) 한 곳에서만 관리됨.
	 *  호출되지 않으면(레벨에 직접 배치해서 테스트하는 경우 등) 아래 Blueprint 디테일 패널에 넣어둔
	 *  기본값을 그대로 사용함 */
	void ApplyBossWaveStat(float InRoarHealthPercentThreshold, float InSlamCooldown, float InSlamRadius, float InRoarDuration,
		float InAddMaxHealth = 0.f, float InAddMoveSpeed = 0.f, float InAddAttackPower = 0.f, float InAddAttackRange = 0.f);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void AttackByAI() override;

	/** bIsSlamAttack이면 자기 위치 중심 원형 AOE로, 아니면 Super(정면 스윕)로 판정함.
	 *  판정 후 LastAttackHitActor가 플레이어면 OnBossAttackedPlayer를 Broadcast함 */
	virtual void AttackHitCheck() override;

	/** 공격범위 표시 NotifyState가 슬램 판정 반경과 항상 같은 값을 쓰도록 오버라이드 */
	virtual float GetAIAOERadius() override { return SlamRadius; }

	/** 한 번도 포효하지 않은 채로(bArmedForRoar가 true인 채로) 죽는 경우(예: 큰 데미지를 한 번에
	 *  맞아 50% 임계치 구간을 그냥 건너뛰고 죽는 경우), 죽기 직전에 포효를 강제로 한 번 재생하고
	 *  그게 끝난 뒤에야 실제 사망 처리(Super::Dead())를 하도록 오버라이드함.
	 *  이미 한 번이라도 포효했다면(bArmedForRoar==false) 평소처럼 바로 죽음 */
	virtual void Dead() override;

public:
	/** BT의 ShouldRoar 데코레이터가 매 틱 확인: 무장 상태(bArmedForRoar)이고, 체력비율이 임계치 밑이면 true */
	bool ShouldRoar();

	/** BT의 Roar 태스크가 호출. 포효 몽타주를 RoarDuration에 맞춰 재생 속도를 조절해 재생하고,
	 *  몽타주 종료 이벤트가 아니라 RoarDuration 자체를 타이머로 써서 그 시간만큼 무적을 부여함.
	 *  끝나면 델리게이트 실행 */
	void RoarByAI();

	/** BT의 Roar 태스크가 실행 전에 호출해서, 포효(몽타주)가 끝났을 때 알림받을 델리게이트를 등록 */
	void SetRoarDelegate(const FAICharacterAttackFinished& InOnRoarFinished) { OnRoarFinished = InOnRoarFinished; }

	/** BT의 Roar 태스크가 AbortTask로 강제 중단될 때 호출 - RoarByAI()가 시작하자마자 bArmedForRoar를
	 *  false로 바꾸는 탓에 ShouldRoar 데코레이터의 Self-Abort가 거의 즉시 걸릴 수 있는데, 이때 몽타주
	 *  정지/무적 해제/타이머 정리를 안 해주면 포효 몽타주가 다음 공격 전까지 계속 재생됨 */
	void CancelRoar();

	/** 보스의 공격이 플레이어에게 실제로 명중할 때마다 Broadcast (see AttackHitCheck) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBossAttackedPlayer OnBossAttackedPlayer;

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

	/** 슬램(내려찍기) 판정 반경(cm) - 보스 위치 중심 원형 AOE. 공격범위 표시 NotifyState도
	 *  GetAIAOERadius() 통해 이 값을 그대로 씀. RoundInfoTable(FCPMonsterRoundInfoRow::SlamRadius)에서
	 *  라운드별로 덮어씀 - ApplyBossWaveStat 참고 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Slam")
	float SlamRadius = 300.f;

	/** 포효(무적) 지속시간(초) - RoundInfoTable(FCPRoundInfoRow::RoarDuration)에서 덮어씀.
	 *  RoarMontage는 원본 길이와 무관하게 이 시간에 딱 맞도록 재생 속도가 자동 조절됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Roar", meta = (ClampMin = 0))
	float RoarDuration = 2.f;

private:
	FAICharacterAttackFinished OnRoarFinished;
	bool bArmedForRoar = true;
	float LastSlamTime = -1000.f;

	/** AttackByAI()가 이번에 고른 몽타주가 슬램인지 기억해뒀다가, 그 뒤에 노티파이로 불리는
	 *  AttackHitCheck()에서 판정 모양(원형 AOE vs 정면 스윕)을 분기하는 데 씀 */
	bool bIsSlamAttack = false;

	/** Dead()가 죽기 직전 강제 포효를 재생 중인 동안 true - 그 포효의 종료 델리게이트가 다시
	 *  Dead()를 부르므로, 재진입을 막기 위한 가드 */
	bool bFinalRoarPlaying = false;

	/** RoarByAI가 RoarDuration만큼 무적을 유지하기 위해 거는 타이머 - 몽타주 종료 이벤트 대신
	 *  이 타이머가 무적 해제 시점을 결정함 */
	FTimerHandle RoarDurationTimerHandle;
};
