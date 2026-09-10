// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Boss/CPMonsterBoss.h"
#include "Monster/CPMonsterAIController.h"

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

void ACPMonsterBoss::Dead()
{
	// [임시 디버그] 죽음 처리가 몇 번이나, 어떤 상태로 호출되는지 확인용 - 강제 포효 없이 바로
	// Super::Dead()로 빠지는지, 강제 포효 경로를 타는지 구분
	UE_LOG(LogTemp, Warning,
		TEXT("[임시 디버그] %s Boss::Dead() 호출 - bIsDead=%d, bFinalRoarPlaying=%d, bArmedForRoar=%d, RoarMontage=%s, CurrentCCState=%d"),
		*GetName(), bIsDead, bFinalRoarPlaying, bArmedForRoar, RoarMontage ? TEXT("Valid") : TEXT("NULL"), static_cast<uint8>(CurrentCCState));

	// 이미 한 번이라도 포효했다면(bArmedForRoar==false) 여기서 더 할 일 없이 평소대로 바로 죽음.
	// 재진입 가드(bFinalRoarPlaying)는 지금 재생 중인 강제 포효가 끝나 다시 이 함수가 호출됐을 때
	// 또 포효를 걸지 않고 바로 Super::Dead()로 넘어가게 해줌
	if (bIsDead || bFinalRoarPlaying || !bArmedForRoar || !RoarMontage)
	{
		Super::Dead();
		return;
	}

	bFinalRoarPlaying = true;

	// 포효가 끝날 때까지는 더 움직이거나 공격하지 않도록 AI/이동을 먼저 멈춤
	if (ACPMonsterAIController* AIController = GetController<ACPMonsterAIController>())
	{
		AIController->StopAI();
	}

	// BT의 Roar 태스크를 거치지 않고 직접 호출하는 것이므로, 델리게이트도 직접 걸어줌 -
	// 포효가 끝나면 바로 실제 사망 처리(Super::Dead())로 이어짐
	FAICharacterAttackFinished FinalRoarFinished;
	FinalRoarFinished.BindLambda([this]()
	{
		Super::Dead();
	});
	SetRoarDelegate(FinalRoarFinished);
	RoarByAI();
}
