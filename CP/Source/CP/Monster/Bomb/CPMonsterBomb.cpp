// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Bomb/CPMonsterBomb.h"
#include "NiagaraComponent.h"
#include "Player/CPPlayerCharacter.h"

ACPMonsterBomb::ACPMonsterBomb()
{
	FlightSpawnHeight = 150.f;

	// Niagara System 에셋은 BP_Bomb에서 지정(예: Shooter_VFXPack/P_Hit_Classic_Custom)
	FuseEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FuseEffect"));
	FuseEffect->SetupAttachment(GetMesh(), TEXT("Wick"));
	FuseEffect->bAutoActivate = true;
}

void ACPMonsterBomb::AttackHitCheck()
{
	// 근접 스윕으로 데미지를 주는 기존 로직(ACPMonsterBase::AttackHitCheck)을 그대로 재사용함 -
	// DT_MonsterStat에서 이 타입의 AttackRange를 0(또는 아주 작은 값)으로 두면 "닿아야만" 판정이 남.
	// 투사체는 스폰하지 않음(ACPMonsterRanged와 달리 Fire() 없음)
	Super::AttackHitCheck();

	// 스윕이 실제로 플레이어를 맞췄을 때만 - Explode()는 스윕이 빗나가도(AttackInRange 데코레이터만
	// 통과하면) 항상 호출되는 기존 동작이라, CoinPusher 보상 트리거는 "플레이어에 닿았을 때"만 따로 구분함
	if (Cast<ACPPlayerCharacter>(LastAttackHitActor))
	{
		OnBombExplodedOnPlayer.Broadcast();
	}

	Explode();
}

void ACPMonsterBomb::Explode()
{
	// 폭발 이펙트/애니메이션은 DeadMontage가 아니라 AttackMontage 쪽에 넣는 걸 권장함 - AttackMontage가
	// 끝까지 자연 재생된 뒤(Dead()는 DeadMontage가 비어있으면 현재 재생 중인 몽타주를 끊지 않음)
	// SetLifeSpan(2초)으로 정리됨. Dead()가 코인 드랍/OnMonsterDied 브로드캐스트/파괴까지 전부
	// 처리하므로 여기서는 따로 할 일이 없음
	Dead();
}

void ACPMonsterBomb::OnReturnedToPool()
{
	Super::OnReturnedToPool();

	if (FuseEffect)
	{
		FuseEffect->Deactivate();
	}
}

void ACPMonsterBomb::OnAcquiredFromPool(const FTransform& NewTransform)
{
	Super::OnAcquiredFromPool(NewTransform);

	if (FuseEffect)
	{
		FuseEffect->Activate(true);
	}
}
