// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPCoin.h"
#include "CPDropZone.h"
#include "CPCoinThrowArea.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "TimerManager.h"

ACPCoin::ACPCoin()
{
	// 패시브 코인 스케일 연출 중에만 Tick이 실제로 돌아야 하므로, Tick 자체는 등록해두고(bCanEverTick)
	// 평소에는 꺼둔 채(bStartWithTickEnabled) 연출이 시작될 때만 SetActorTickEnabled(true)로 켠다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));
	Mesh->SetSimulatePhysics(true);
	Mesh->bNavigationRelevant = false;

	// 컴포넌트를 통한 Has-a - 실제 사용할 BP 서브클래스는 Child Actor Class에서 지정
	CoinThrowAreaComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CoinThrowAreaComponent"));
	CoinThrowAreaComponent->SetupAttachment(Mesh);
}

void ACPCoin::Launch(const FVector& LaunchVelocity)
{
	// 이미 발사되어 날아가고 있는 중이면 추가 Launch()는 무시 - 속도가 중복으로 누적되는 것을 방지
	if (bIsLaunched)
	{
		return;
	}

	bIsLaunched = true;
	Mesh->SetPhysicsLinearVelocity(LaunchVelocity);

	GetWorldTimerManager().SetTimer(LaunchCooldownTimerHandle, this, &ACPCoin::ClearLaunchedState, LaunchCooldown, false);
}

void ACPCoin::ClearLaunchedState()
{
	bIsLaunched = false;
}

void ACPCoin::Collect()
{
	if (!bCollected)
	{
		bCollected = true;

		BP_OnCollected();

		Destroy();
	}
}

void ACPCoin::OnDroppedInZone(ACPDropZone* DropZone)
{
	if (DropZone)
	{
		DropZone->AddCollectedCoins(1);
	}

	Collect();
}

void ACPCoin::SetCoinType(ECPCoinType NewType)
{
	if (CoinType == NewType)
	{
		return;
	}

	const ECPCoinType OldType = CoinType;
	CoinType = NewType;

	// 다른 타입으로 바뀌는 순간 진행 중이던 스케일 연출은 취소하고 원본 스케일로 즉시 복귀
	CancelScaleAnimation();

	// CoinTypeVisuals에 지정된 Mesh/Material이 있으면 적용
	ApplyCoinTypeVisual(CoinType);

	switch (CoinType)
	{
	case ECPCoinType::Passive:
	case ECPCoinType::HP:
		StartScaleAnimation();
		break;

	case ECPCoinType::Normal:
	case ECPCoinType::Giant:
	default:
		// 추가 타입별 연출/행동은 BP_OnCoinTypeChanged에서 BP로 확장
		break;
	}

	BP_OnCoinTypeChanged(OldType, CoinType);
}

void ACPCoin::StartScaleAnimation()
{
	ScaleAnimOriginalScale = GetActorScale3D();
	ScaleAnimCurrentMultiplier = 1.0f;
	ScaleAnimPhase = EScaleAnimPhase::ShrinkingToMin;

	SetActorTickEnabled(true);
}

void ACPCoin::CancelScaleAnimation()
{
	if (ScaleAnimPhase == EScaleAnimPhase::None)
	{
		return;
	}

	ScaleAnimPhase = EScaleAnimPhase::None;
	SetActorScale3D(ScaleAnimOriginalScale);
	SetActorTickEnabled(false);
}

void ACPCoin::ApplyCoinTypeVisual(ECPCoinType NewType)
{
	if (!Mesh)
	{
		return;
	}

	const FCPCoinTypeVisual* Visual = CoinTypeVisuals.Find(NewType);
	if (!Visual)
	{
		return;
	}

	if (Visual->Mesh)
	{
		Mesh->SetStaticMesh(Visual->Mesh);
	}

	if (Visual->Material)
	{
		Mesh->SetMaterial(0, Visual->Material);
	}
}

void ACPCoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ScaleAnimPhase == EScaleAnimPhase::None)
	{
		return;
	}

	float TargetMultiplier = 1.0f;
	switch (ScaleAnimPhase)
	{
	case EScaleAnimPhase::ShrinkingToMin:
		TargetMultiplier = ScaleAnimMinScale;
		break;

	case EScaleAnimPhase::GrowingToMax:
		TargetMultiplier = ScaleAnimMaxScale;
		break;

	case EScaleAnimPhase::ReturningToOriginal:
		TargetMultiplier = 1.0f;
		break;

	default:
		break;
	}

	ScaleAnimCurrentMultiplier = FMath::FInterpConstantTo(ScaleAnimCurrentMultiplier, TargetMultiplier, DeltaTime, ScaleAnimSpeed);
	SetActorScale3D(ScaleAnimOriginalScale * ScaleAnimCurrentMultiplier);

	if (!FMath::IsNearlyEqual(ScaleAnimCurrentMultiplier, TargetMultiplier, KINDA_SMALL_NUMBER))
	{
		return;
	}

	switch (ScaleAnimPhase)
	{
	case EScaleAnimPhase::ShrinkingToMin:
		ScaleAnimPhase = EScaleAnimPhase::GrowingToMax;
		break;

	case EScaleAnimPhase::GrowingToMax:
		ScaleAnimPhase = EScaleAnimPhase::ReturningToOriginal;
		// 최대 크기에 도달한 순간(Passive/HP 공용) CoinThrowArea를 코인 위치로 옮겨 활성화
		ActivateCoinThrowArea();
		break;

	case EScaleAnimPhase::ReturningToOriginal:
		ScaleAnimPhase = EScaleAnimPhase::None;
		SetActorTickEnabled(false);
		break;

	default:
		break;
	}
}

void ACPCoin::ActivateCoinThrowArea()
{
	ACPCoinThrowArea* CoinThrowArea = GetCoinThrowArea();
	if (!CoinThrowArea)
	{
		return;
	}

	// 위/앞 방향과 마찬가지로 회전과 무관하게 항상 월드 X축을 기준으로 코인 옆에 배치
	CoinThrowArea->SetActorLocation(GetActorLocation() + FVector(CoinThrowAreaOffsetX, 0.0f, 0.0f));
	CoinThrowArea->ActiveThrow();
}

ACPCoinThrowArea* ACPCoin::GetCoinThrowArea() const
{
	return CoinThrowAreaComponent ? Cast<ACPCoinThrowArea>(CoinThrowAreaComponent->GetChildActor()) : nullptr;
}
