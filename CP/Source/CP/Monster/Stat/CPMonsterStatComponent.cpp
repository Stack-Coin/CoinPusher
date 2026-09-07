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

	if (!WaveStatTable)
	{
		ResetStat();
		return;
	}

	const FCPMonsterWaveStatRow* WaveStatRow = WaveStatTable->FindRow<FCPMonsterWaveStatRow>(RowName, TEXT("InitWaveStat"));
	if (!WaveStatRow)
	{
		ResetStat();
		return;
	}

	// 같은 Row 안에서 웨이브 번호가 일치하는 항목을 찾음
	const FCPMonsterWaveStat* WaveStat = WaveStatRow->WaveStats.FindByPredicate([InWave](const FCPMonsterWaveStat& Stat) { return Stat.Wave == InWave; });
	if (!WaveStat)
	{
		ResetStat();
		return;
	}

	// 웨이브에 따른 수치 변화 있음
	MaxHealth   = BaseRow->MaxHealth   + WaveStat->AddMaxHealth;
	CurrentHealth = MaxHealth;
	MoveSpeed   = BaseRow->MoveSpeed   + WaveStat->AddMoveSpeed;
	AttackPower = BaseRow->AttackPower + WaveStat->AddAttackPower;

	// 웨이브에 따른 수치 변화 없음
	DefaultStat = BaseRow->DefaultStat;  // 공격속도 + 넉백 + 인식범위 + 콜리전
}
