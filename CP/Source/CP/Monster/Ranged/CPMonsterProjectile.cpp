// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Player/CPPlayerCharacter.h"
#include "Nexus/CPNexus.h"

ACPMonsterProjectile::ACPMonsterProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(25.f);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Damageable
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel5, ECR_Overlap); // Nexus
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);      // todo. 플레이어에 Damageable 설정 필요.
	// 벽 등 월드 지오메트리엔 부딪혀서 파괴됨
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); 

	CollisionComp->OnComponentHit.AddDynamic(this, &ACPMonsterProjectile::OnProjectileHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ACPMonsterProjectile::OnProjectileOverlap);
	RootComponent = CollisionComp;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void ACPMonsterProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComp)
	{
		CollisionComp->SetSphereRadius(ProjectileRadius);
	}

	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileSpeed;

	// 사거리만큼 이동하는 데 걸리는 시간이 지나면 자동 파괴
	// 거리 % 속도 = 시간
	if (ProjectileSpeed > 0.f)
	{
		SetLifeSpan(Range / ProjectileSpeed);
	}
}

void ACPMonsterProjectile::Init(float InDamageAmount, float InKnockbackDistance, AController* InInstigatorController, AActor* InDamageCauser)
{
	DamageAmount = InDamageAmount;
	KnockbackDistance = InKnockbackDistance;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;
}

bool ACPMonsterProjectile::IsValidMonsterProjectileTarget(AActor* OtherActor) const
{
	return OtherActor && (OtherActor->IsA<ACPPlayerCharacter>() || OtherActor->IsA<ACPNexus>());
}

void ACPMonsterProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ProcessHit(OtherActor, bFromSweep ? FVector(SweepResult.Location) : GetActorLocation());
}

void ACPMonsterProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ProcessHit(OtherActor, Hit.Location);
	Destroy();
}

void ACPMonsterProjectile::ProcessHit(AActor* OtherActor, const FVector& HitLocation)
{
	if (!IsValidMonsterProjectileTarget(OtherActor) || OtherActor == GetOwner())
	{
		return;
	}

	UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, InstigatorController.Get(), DamageCauserActor.Get(), UDamageType::StaticClass());

	if (ICPKnockbackable* KnockbackTarget = Cast<ICPKnockbackable>(OtherActor))
	{
		const FVector Direction = ProjectileMovement ? ProjectileMovement->Velocity.GetSafeNormal() : GetActorForwardVector();
		KnockbackTarget->ApplyKnockback(Direction, KnockbackDistance, DamageCauserActor.Get());
	}

	Destroy();
}
