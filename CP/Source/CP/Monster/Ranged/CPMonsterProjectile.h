// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

UCLASS(Blueprintable)
class CP_API ACPMonsterProjectile : public AActor
{
	GENERATED_BODY()

public:
	ACPMonsterProjectile();

	virtual void BeginPlay() override;

	void Init(float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser);

protected:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ProcessHit(AActor* OtherActor, const FVector& HitLocation);

	bool IsValidMonsterProjectileTarget(AActor* OtherActor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 투사체 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = 0, Units = "cm/s"))
	float ProjectileSpeed = 200.f;

	// 최대 사거리 (cm) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = 0, Units = "cm"))
	float Range = 1000.f;

	// 투사체 콜라이더 반지름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = 0, Units = "cm"))
	float ProjectileRadius = 25.f;

protected:
	float DamageAmount = 0.f;
	TWeakObjectPtr<AController> InstigatorController;
	TWeakObjectPtr<AActor> DamageCauserActor;
};
