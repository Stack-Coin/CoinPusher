// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/CPMonsterBase.h"
#include "CPMonsterFly.generated.h"

/**
 * 비행 몬스터 공통 기반(ACPMonsterBomb/ACPMonsterRanged가 여기서 파생) - 항상 고정 높이로
 * 스폰되고(GetSpawnHeightOffset/ShouldUseFixedSpawnHeight), BeginPlay에서 PlaneConstraint로 Z 이동을
 * 잠근 채 수평으로만 움직임. 파생 클래스는 공격 방식(근접 자폭/투사체)만 추가하면 됨.
 */
UCLASS()
class CP_API ACPMonsterFly : public ACPMonsterBase
{
	GENERATED_BODY()

public:
	ACPMonsterFly();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual float GetSpawnHeightOffset() const override { return FlightSpawnHeight; }
	virtual bool ShouldUseFixedSpawnHeight() const override { return true; }

protected:
	/** 비행 몬스터라 지면 캡슐 높이 기준 스폰이 의미 없어서, 스포너 위치로부터 항상 이 높이로 스폰됨
	 *  (300 미만 권장 - NavMesh 투영 범위를 벗어나면 MoveTo 실패). 서브클래스 생성자에서 원하는 값으로
	 *  덮어써도 됨(예: Bomb=150) - 여기 기본값을 고쳐도 이미 저장된 블루프린트 Class Defaults는
	 *  자동으로 안 바뀌니 주의 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly")
	float FlightSpawnHeight = 120.f;
};
