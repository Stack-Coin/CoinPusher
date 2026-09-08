// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Stat/CPMonsterStatComponent.h"
#include "Engine/DataTable.h"

UCPMonsterStatComponent::UCPMonsterStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FName UCPMonsterStatComponent::GetMonsterTypeRowName(ECPMonsterType InType)
{
	switch (InType)
	{
	case ECPMonsterType::Normal: return TEXT("Normal");
	case ECPMonsterType::Tanker: return TEXT("Tanker");
	case ECPMonsterType::Ranged: return TEXT("Ranged");
	case ECPMonsterType::Boss: return TEXT("Boss");
	}
	return NAME_None;
}

void UCPMonsterStatComponent::ResetStat()
{
	MaxHealth = 0.f;
	CurrentHealth = 0.f;
	MoveSpeed = 0.f;
	AttackPower = 0.f;

	DefaultStat = FCPMonsterDefaultStat();
}

void UCPMonsterStatComponent::InitStat(ECPMonsterType InMonsterType, int32 InWave)
{
	// 기획자가 디테일 패널에서 넣어둔 값을 그대로 테스트하고 싶을 때 사용. DataTable 조회를 아예 건너뜀
	if (bOverrideStat)
	{
		CurrentHealth = MaxHealth;
		return;
	}

	if (!BaseStatTable)
	{
		ResetStat();
		return;
	}

	// BaseStatTable / WaveStatTable 모두 RowName을 MonsterType(Normal/Tanker/Ranged)으로 통일해서 사용
	const FName RowName = GetMonsterTypeRowName(InMonsterType);

	const FCPMonsterStatRow* BaseRow = BaseStatTable->FindRow<FCPMonsterStatRow>(RowName, TEXT("InitDefaultStat"));
	if (!BaseRow)
	{
		ResetStat();
		return;
	}

	// 웨이브 증가치 찾기: BaseStatTable과 동일하게 RowName("Normal_1", "Boss_3"...)으로 바로 FindRow
	// (해당 웨이브의 증가치 행이 없으면 증가치 0으로 처리하고 기본 스탯을 그대로 사용함 - 웨이브를
	//  못 찾았다고 스탯을 전부 0으로 리셋하던 이전 동작은 의도치 않은 버그였음. 아직 증가치가 없는
	//  웨이브가 대부분이라 bWarnIfRowMissing은 false로 둬서 로그 스팸을 막음)
	float AddMaxHealth = 0.f;
	float AddMoveSpeed = 0.f;
	float AddAttackPower = 0.f;

	if (WaveStatTable)
	{
		const FName WaveRowName = *FString::Printf(TEXT("%s_%d"), *RowName.ToString(), InWave);
		if (const FCPMonsterWaveStatRow* WaveRow = WaveStatTable->FindRow<FCPMonsterWaveStatRow>(WaveRowName, TEXT("InitWaveStat"), /*bWarnIfRowMissing=*/false))
		{
			AddMaxHealth = WaveRow->AddMaxHealth;
			AddMoveSpeed = WaveRow->AddMoveSpeed;
			AddAttackPower = WaveRow->AddAttackPower;
		}
	}

	// 웨이브에 따른 수치 변화 있음
	MaxHealth   = BaseRow->MaxHealth   + AddMaxHealth;
	CurrentHealth = MaxHealth;
	MoveSpeed   = BaseRow->MoveSpeed   + AddMoveSpeed;
	AttackPower = BaseRow->AttackPower + AddAttackPower;

	// 웨이브에 따른 수치 변화 없음 (평탄화된 BaseRow 필드를 DefaultStat 캐시로 복사)
	DefaultStat.AttackSpeed = BaseRow->AttackSpeed;
	DefaultStat.KnockbackPower = BaseRow->KnockbackPower;
	DefaultStat.KnockbackDuration = BaseRow->KnockbackDuration;
	DefaultStat.KnockbackDistance = BaseRow->KnockbackDistance;
	DefaultStat.DetectRange = BaseRow->DetectRange;
	DefaultStat.CollisionRadius = BaseRow->CollisionRadius;
	DefaultStat.PatrolRadius = BaseRow->PatrolRadius;
	DefaultStat.AttackRange = BaseRow->AttackRange;
	DefaultStat.TurnSpeed = BaseRow->TurnSpeed;
	DefaultStat.MoveAcceptableRadius = BaseRow->MoveAcceptableRadius;
	DefaultStat.AvoidanceRadiusMultiplier = BaseRow->AvoidanceRadiusMultiplier;
	DefaultStat.AvoidanceWeight = BaseRow->AvoidanceWeight;
	DefaultStat.SeparationPadding = BaseRow->SeparationPadding;
	DefaultStat.SeparationSpeed = BaseRow->SeparationSpeed;
}
