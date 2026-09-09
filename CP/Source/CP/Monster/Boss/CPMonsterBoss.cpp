// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Boss/CPMonsterBoss.h"

ACPMonsterBoss::ACPMonsterBoss()
{
	MonsterType = ECPMonsterType::Boss;
}

void ACPMonsterBoss::ApplyBossWaveStat(float InRoarHealthPercentThreshold, float InSlamCooldown)
{
	RoarHealthPercentThreshold = InRoarHealthPercentThreshold;
	SlamCooldown = InSlamCooldown;
}

void ACPMonsterBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 포효 후 잠겨있던 무장 상태를, 체력이 임계치 이상으로 회복되면 다시 풀어줌(재발동 가능하게)
	if (!bArmedForRoar && !bIsDead)
	{
		const float MaxHealth = GetAIMaxHealth();
		if (MaxHealth > 0.f && (GetAICurrentHealth() / MaxHealth) >= RoarHealthPercentThreshold)
		{
			bArmedForRoar = true;
		}
	}
}

void ACPMonsterBoss::AttackByAI()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;


	// 슬램 쿨타임이 다 찼으면 슬램, 아니면 기본 공격
	if (SlamMontage && (Now - LastSlamTime) >= SlamCooldown)
	{
		LastSlamTime = Now;
		PlayAttackMontage(SlamMontage);
	}
	else
	{
		PlayAttackMontage(AttackMontage);
	}
}

bool ACPMonsterBoss::ShouldRoar() 
{
	if (!bArmedForRoar || bIsDead)
	{
		return false;
	}

	if (GetAIMaxHealth() <= 0.f)
	{
		return false;
	}

	return (GetAICurrentHealth() / GetAIMaxHealth()) < RoarHealthPercentThreshold;
}

void ACPMonsterBoss::RoarByAI()
{
	// 발동 즉시 잠금 - 몽타주가 끝나고 체력이 다시 회복될 때까지는 재발동 안 됨
	bArmedForRoar = false;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && RoarMontage)
	{
		AddCCState(ECPMonsterCCState::Invulnerable);

		AnimInstance->StopAllMontages(0.0f);
		const float Duration = AnimInstance->Montage_Play(RoarMontage, 1.0f);

		if (Duration > 0.0f)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ACPMonsterBoss::HandleRoarMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, RoarMontage);
			return;
		}

		// 재생은 시작했지만 Duration이 0 이하로 나온 예외적인 경우 - 무적 상태로 남지 않도록 바로 해제
		RemoveCCState(ECPMonsterCCState::Invulnerable);
	}

	// 몽타주가 없거나 재생 실패 - 무적 없이 바로 끝난 걸로 처리해서 BT가 멈추지 않게 함
	OnRoarFinished.ExecuteIfBound();
}

void ACPMonsterBoss::HandleRoarMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	RemoveCCState(ECPMonsterCCState::Invulnerable);
	OnRoarFinished.ExecuteIfBound();
}
