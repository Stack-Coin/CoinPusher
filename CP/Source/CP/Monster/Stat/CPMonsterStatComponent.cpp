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
	case ECPMonsterType::Bomb: return TEXT("Bomb");
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

void UCPMonsterStatComponent::InitStat(ECPMonsterType InMonsterType, int32 InRound, int32 InWave)
{
	// 기획자가 디테일 패널에서 넣어둔 값을 그대로 테스트하고 싶을 때 사용. DataTable 조회를 아예 건너뜀
	if (bOverrideStat)
	{
		CurrentHealth = MaxHealth;
		return;
	}

	// WaveStatTable과 동일한 방식: 블루프린트에서 수동으로 연결해둔 경로가 있으면
	// 그대로 두고, 비어있을 때만 고정 경로에서 자동으로 찾아서 채움
	if (!BaseStatTable)
	{
		BaseStatTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Monster/Data/DT_MonsterStat.DT_MonsterStat"));
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

	// WaveStatTable도 동일한 방식: 이미 연결돼 있으면 유지, 비어있을 때만 고정 경로에서 자동으로 채움
	if (!WaveStatTable)
	{
		WaveStatTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Monster/Data/DT_WaveStat.DT_WaveStat"));
	}

	if (WaveStatTable)
	{
		const FName WaveRowName = *FString::Printf(TEXT("%s_%d"), *RowName.ToString(), InWave);
		if (const FCPMonsterWaveStatRow* WaveRow = WaveStatTable->FindRow<FCPMonsterWaveStatRow>(WaveRowName, TEXT("InitWaveStat"), /*bWarnIfRowMissing=*/false))
		{
			AddMaxHealth += WaveRow->AddMaxHealth;
			AddMoveSpeed += WaveRow->AddMoveSpeed;
			AddAttackPower += WaveRow->AddAttackPower;
		}
	}

	// RoundStatTable은 더 이상 사용하지 않음 - 이제 마지막 웨이브가 보스와 같은 페이즈에서 함께
	// 스폰되고, 그 몹의 InWave 인자도 마지막 웨이브 번호 그대로 넘어오므로 위 WaveStatTable 조회만으로
	// 충분함 (별도의 라운드 보정치 테이블이 필요 없어짐)

	// 웨이브에 따른 수치 변화 있음
	MaxHealth   = BaseRow->MaxHealth   + AddMaxHealth;
	CurrentHealth = MaxHealth;
	MoveSpeed   = BaseRow->MoveSpeed   + AddMoveSpeed;
	AttackPower = BaseRow->AttackPower + AddAttackPower;

	// 웨이브에 따른 수치 변화 없음 (평탄화된 BaseRow 필드를 DefaultStat 캐시로 복사)
	DefaultStat.AttackInterval = BaseRow->AttackInterval;
	DefaultStat.CollisionRadius = BaseRow->CollisionRadius;
	DefaultStat.CollisionHalfHeight = BaseRow->CollisionHalfHeight;
	DefaultStat.AttackRange = BaseRow->AttackRange;
	DefaultStat.TurnSpeed = BaseRow->TurnSpeed;
	DefaultStat.MoveAcceptableRadius = BaseRow->MoveAcceptableRadius;
	DefaultStat.SeparationPadding = BaseRow->SeparationPadding;
	DefaultStat.SeparationSpeed = BaseRow->SeparationSpeed;
}
