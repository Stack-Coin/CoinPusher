// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterRanged.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/Ranged/CPMonsterProjectile.h"

void ACPMonsterRanged::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
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
		// [임시 디버그] 발사 간격 쿨다운에 걸려서 스킵된 경우 - 몽타주는 재생되는데 실제 발사는 안 되는
		// 케이스를 구분하기 위한 로그
		UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s AttackHitCheck 스킵 - Now=%.2f, LastFireTime=%.2f, Duration=%.2f"),
			*GetName(), Now, LastFireTime, Duration);
		return;
	}
	LastFireTime = Now;

	UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s Fire() 호출 - Now=%.2f, Duration=%.2f"), *GetName(), Now, Duration);

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

	if (ACPMonsterProjectile* Projectile = GetWorld()->SpawnActor<ACPMonsterProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		Projectile->Init(GetAIAttackPower(), GetController(), this);
	}
}
