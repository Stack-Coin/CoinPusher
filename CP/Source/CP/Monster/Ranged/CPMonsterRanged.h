// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/CPMonsterBase.h"
#include "CPMonsterRanged.generated.h"

class ACPMonsterProjectile;

UCLASS()
class CP_API ACPMonsterRanged : public ACPMonsterBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void AttackHitCheck() override;

private:
	void Fire();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TSubclassOf<ACPMonsterProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName MuzzleSocketName = TEXT("Muzzle");

private:
	float LastFireTime = -1000.f;
};
