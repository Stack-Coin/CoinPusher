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
	MonsterTemplete = FCPMonsterTemplate();

	MaxHealth = 0.f;
	CurrentHealth = 0.f;
	MoveSpeed = 0.f;
	AttackPower = 0.f;

	DefaultStat = FCPMonsterDefaultStat();
}

void UCPMonsterStatComponent::InitStat(ECPMonsterType InMonsterType, int32 InWave)
{
	if (!BaseStatTable)
	{
		ResetStat();
		return;
	}

	const FCPMonsterStatRow* BaseRow = BaseStatTable->FindRow<FCPMonsterStatRow>(GetMonsterTypeRowName(InMonsterType), TEXT("InitDefaultStat"));

	if (!BaseRow)
	{
		ResetStat();
		return;
	}

	const FName WaveRowName = *FString::FromInt(InWave);
	const FCPMonsterWaveStatRow* WaveRow = WaveStatTable->FindRow<FCPMonsterWaveStatRow>(WaveRowName, TEXT("InitWaveStat"));

	if (!WaveRow)
	{
		ResetStat();
		return;
	}

	// 웨이브에 따른 수치 변화 있음
	MaxHealth   = BaseRow->MaxHealth   + WaveRow->AddMaxHealth;
	CurrentHealth = MaxHealth;
	MoveSpeed   = BaseRow->MoveSpeed   + WaveRow->AddMoveSpeed;
	AttackPower = BaseRow->AttackPower + WaveRow->AddAttackPower;

	// 웨이브에 따른 수치 변화 없음
	MonsterTemplete = BaseRow->Preset;       // 이동방식 + 공격방식 + (투사체)
	DefaultStat     = BaseRow->DefaultStat;  // 공격속도 + 넉백 + 인식범위 + 콜리전
}
