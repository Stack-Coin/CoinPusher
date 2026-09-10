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
	virtual float GetSpawnHeightOffset() const override;

private:
	void Fire();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TSubclassOf<ACPMonsterProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 비행 몬스터라 지면 캡슐 높이 기준 스폰이 의미 없어서, 스포너 위치로부터 항상 이 높이로 스폰됨
	 *  300 이상으로 두면 NavMesh 투영 범위를 벗어나 MoveTo가 실패해 플레이어를 못 쫓아옴 - 90으로 낮춤 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float FlightSpawnHeight = 120.f;

private:
	float LastFireTime = -1000.f;
};
