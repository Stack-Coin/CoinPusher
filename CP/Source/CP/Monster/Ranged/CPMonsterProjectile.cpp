// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Player/CPPlayerCharacter.h"
#include "Landscape.h"

ACPMonsterProjectile::ACPMonsterProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(25.f);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 기본값인 WorldDynamic 대신 이 투사체 전용 오브젝트 타입(GameTraceChannel7 "MonsterProjectile")을 사용함
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel7);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Damageable
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel5, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);      // todo. 플레이어에 Damageable 설정 필요.
	// 벽 등 월드 지오메트리 - Block으로 받으면 원거리 몹이 지면 가까이서 날 때 지형에 막혀 더 못 날아가므로(물리적으로 이동이 정지됨), Overlap으로 받고 실제 파괴 여부는 OnProjectileOverlap에서 판단함 
	// (Landscape는 통과, 그 외 WorldStatic은 파괴)
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

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

	if (ProjectileSpeed > 0.f)
	{
		SetLifeSpan(Range / ProjectileSpeed);
	}
}

void ACPMonsterProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor->IsA<ALandscapeProxy>())
	{
		return;
	}

	ProcessHit(OtherActor, Hit.Location);
	Destroy();
}

void ACPMonsterProjectile::Init(float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser)
{
	DamageAmount = InDamageAmount;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;
}

bool ACPMonsterProjectile::IsValidMonsterProjectileTarget(AActor* OtherActor) const
{
	return OtherActor && OtherActor->IsA<ACPPlayerCharacter>();
}

void ACPMonsterProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 원거리 몹이 지면 가까이서 날아다녀서 발사체가 지형을 스치듯 지나가는 경우가 많음 -
	// Landscape는 그냥 통과시키고 파괴하지 않음
	if (OtherActor && OtherActor->IsA<ALandscapeProxy>())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s OnProjectileOverlap - OtherActor=%s"),
		*GetName(),
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	ProcessHit(OtherActor, bFromSweep ? FVector(SweepResult.Location) : GetActorLocation());

	// 플레이어(유효 타겟)가 아닌 다른 월드 지오메트리(벽 등)에 닿았으면 데미지 없이 그냥 파괴
	if (!IsValidMonsterProjectileTarget(OtherActor))
	{
		Destroy();
	}
}

void ACPMonsterProjectile::ProcessHit(AActor* OtherActor, const FVector& HitLocation)
{
	if (!IsValidMonsterProjectileTarget(OtherActor) || OtherActor == GetOwner())
	{
		return;
	}

	UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, InstigatorController.Get(), DamageCauserActor.Get(), UDamageType::StaticClass());

	Destroy();
}
