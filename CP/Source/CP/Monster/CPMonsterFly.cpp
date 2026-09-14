// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/CPMonsterFly.h"
#include "GameFramework/CharacterMovementComponent.h"

ACPMonsterFly::ACPMonsterFly()
{
}

void ACPMonsterFly::BeginPlay()
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

void ACPMonsterFly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 비행 중 이동 방향에 따라 기울어지지 않도록 Pitch/Roll 고정
	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;
	SetActorRotation(CurrentRotation);
}
