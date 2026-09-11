// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Boss/CPMonsterBoss.h"
#include "Monster/CPMonsterAIController.h"
#include "Player/CPPlayerCharacter.h"
#include "TimerManager.h"

ACPMonsterBoss::ACPMonsterBoss()
{
	MonsterType = ECPMonsterType::Boss;
}

void ACPMonsterBoss::ApplyBossWaveStat(float InRoarHealthPercentThreshold, float InSlamCooldown, float InRoarDuration,
	float InAddMaxHealth, float InAddMoveSpeed, float InAddAttackPower)
{
	RoarHealthPercentThreshold = InRoarHealthPercentThreshold;
	SlamCooldown = InSlamCooldown;
	RoarDuration = InRoarDuration;

	// 보스 라운드 스탯 보정치 - DT_RoundStat이 아니라 RoundInfo에서만 관리됨. ApplyWaveStat(BaseStat만
	// 반영된 상태)이 스폰 시 이미 호출된 뒤이므로, 여기서는 그 위에 그대로 더해주기만 하면 됨
	if (UCPMonsterStatComponent* StatComp = GetAIStatComponent())
	{
		StatComp->MaxHealth += InAddMaxHealth;
		StatComp->CurrentHealth = StatComp->MaxHealth;
		StatComp->MoveSpeed += InAddMoveSpeed;
		StatComp->AttackPower += InAddAttackPower;
	}
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

void ACPMonsterBoss::AttackHitCheck()
{
	Super::AttackHitCheck();

	// 방금 스윕이 실제로 플레이어를 맞췄을 때만 - 넥서스를 맞췄거나 빗나간 경우는 제외
	if (Cast<ACPPlayerCharacter>(LastAttackHitActor))
	{
		OnBossAttackedPlayer.Broadcast();
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
	if (AnimInstance && RoarMontage && RoarDuration > 0.f)
	{
		AddCCState(ECPMonsterCCState::Invulnerable);

		// 몽타주 원본 길이와 무관하게 RoarDuration(RoundInfoTable에서 온 값)에 정확히 맞춰
		// 재생되도록 재생 속도를 역산함 - 기획자가 라운드마다 포효 시간을 조절하면 애니메이션도
		// 그 시간에 맞춰 빠르게/느리게 재생됨
		const float NativeLength = RoarMontage->GetPlayLength();
		const float PlayRate = (NativeLength > 0.f) ? (NativeLength / RoarDuration) : 1.f;

		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(RoarMontage, PlayRate);

		// 무적 해제 시점은 몽타주 종료 이벤트가 아니라 RoarDuration 자체를 타이머로 써서 결정함 -
		// 재생 속도 계산이 조금 어긋나거나 몽타주가 중간에 인터럽트되어도 포효 지속시간은
		// 항상 데이터(RoundInfoTable)로 정확히 통제됨
		TWeakObjectPtr<ACPMonsterBoss> WeakThis(this);
		GetWorldTimerManager().SetTimer(RoarDurationTimerHandle, [WeakThis]()
		{
			if (ACPMonsterBoss* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleRoarMontageEnded(nullptr, false);
			}
		}, RoarDuration, false);
		return;
	}

	// 몽타주가 없거나 RoarDuration이 0 이하인 경우 - 무적 없이 바로 끝난 걸로 처리해서 BT가 멈추지 않게 함
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
