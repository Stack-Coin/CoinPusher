// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterRanged.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/Ranged/CPMonsterProjectile.h"

ACPMonsterRanged::ACPMonsterRanged()
{
	FlightSpawnHeight = 150.f;
}

void ACPMonsterRanged::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Flying);

		// BT의 MoveTo가 플레이어(지면 위치)를 쫓아가면서 Z까지 같이 끌고 내려가던 게 진짜 원인이었음.
		// 라인트레이스/타이머/틱 보정 대신, 엔진 내장 PlaneConstraint로 이동 자체를 수평면(XY)에만
		// 투영되게 강제함 - AI가 뭘 하든 Z는 스폰 시점 높이(스포너가 GetSpawnHeightOffset()으로 이미
		// 정확히 잡아준 값) 그대로 유지되고, 몹이 아무리 많아도 추가 비용이 전혀 없음(틱/타이머 없음)
		MoveComp->SetPlaneConstraintEnabled(true);
		MoveComp->SetPlaneConstraintNormal(FVector::UpVector);
		MoveComp->SetPlaneConstraintOrigin(GetActorLocation());
	}
}

void ACPMonsterRanged::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;

	SetActorRotation(CurrentRotation);
}

void ACPMonsterRanged::AttackHitCheck()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float Duration = GetAIAttackInterval();

	if (Duration > 0.f && Now - LastFireTime < Duration)
	{
		return;
	}
	LastFireTime = Now;

	Fire();
}

float ACPMonsterRanged::GetSpawnHeightOffset() const
{
	return FlightSpawnHeight;
}

void ACPMonsterRanged::Fire()
{
	if (!ProjectileClass)
	{
		return;
	}

	FVector SpawnLocation = GetActorLocation();
	const FRotator SpawnRotation = GetActorRotation();

	if (GetMesh() && GetMesh()->DoesSocketExist(MuzzleSocketName))
	{
		SpawnLocation = GetMesh()->GetSocketLocation(MuzzleSocketName);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPMonsterProjectile* Projectile = GetWorld()->SpawnActor<ACPMonsterProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Projectile)
	{
		Projectile->Init(GetAIAttackPower(), GetController(), this);
	}
}
