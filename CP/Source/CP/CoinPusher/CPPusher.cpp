// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPPusher.h"
#include "Components/StaticMeshComponent.h"

ACPPusher::ACPPusher()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = PushPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PushPlate"));

	//�浹 ó��
	PushPlate->SetMobility(EComponentMobility::Movable);
	PushPlate->SetCollisionProfileName(FName("BlockAll"));
	PushPlate->SetSimulatePhysics(false);

	//NaveMesh ���� ����
	PushPlate->bNavigationRelevant = false;
}

void ACPPusher::BeginPlay()
{
	Super::BeginPlay();

	//�����ġ�� ���� �ڷ� ���� ���� �� ��ġ�� ���
	StartRelativeLocation = PushPlate->GetRelativeLocation();
}

void ACPPusher::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsPaused)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	// 0..1 oscillation using a sine wave so the plate eases in and out at the ends of its stroke
	const float Alpha = (FMath::Sin(ElapsedTime * CycleSpeed * 2.0f * PI) + 1.0f) * 0.5f;
	const float PushOffset = PushDistance * Alpha;

	// StartRelativeLocation(왕복 운동의 기준 위치) 기준 절대 위치로 매 틱 다시 계산해서 적용 - 일시정지
	// 중 외부에서 액터가 다른 곳으로 옮겨져 있었더라도, 재개되는 순간 이 기준 위치를 바탕으로 한 올바른
	// 지점으로 자동 복귀하며 진행된다
	PushPlate->SetRelativeLocation(StartRelativeLocation + FVector(PushOffset, 0.0f, 0.0f));
}
