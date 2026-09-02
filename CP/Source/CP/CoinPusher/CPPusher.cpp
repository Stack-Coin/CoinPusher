// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPPusher.h"
#include "Components/StaticMeshComponent.h"

ACPPusher::ACPPusher()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = PushPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PushPlate"));

	//충돌 처리
	PushPlate->SetMobility(EComponentMobility::Movable);
	PushPlate->SetCollisionProfileName(FName("BlockAll"));
	PushPlate->SetSimulatePhysics(false);

	//NaveMesh 생성 차단
	PushPlate->bNavigationRelevant = false;
}

void ACPPusher::BeginPlay()
{
	Super::BeginPlay();

	//상대위치를 가장 뒤로 떙겨 졌을 때 위치로 사용
	StartRelativeLocation = PushPlate->GetRelativeLocation();
	LastPushOffset = 0.0f;
}

void ACPPusher::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	// 0..1 oscillation using a sine wave so the plate eases in and out at the ends of its stroke
	const float Alpha = (FMath::Sin(ElapsedTime * CycleSpeed * 2.0f * PI) + 1.0f) * 0.5f;
	const float TargetPushOffset = PushDistance * Alpha;

	//절대 위치를 매 프레임 다시 설정하는 대신, 직전 프레임과의 차이만큼 AddLocalOffset으로 이동
	const float DeltaOffset = TargetPushOffset - LastPushOffset;
	PushPlate->AddLocalOffset(FVector(DeltaOffset, 0.0f, 0.0f));

	LastPushOffset = TargetPushOffset;
}
