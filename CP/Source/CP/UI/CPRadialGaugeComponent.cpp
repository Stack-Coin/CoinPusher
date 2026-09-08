// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPRadialGaugeComponent.h"
#include "UI/CPRadialGaugeWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

UCPRadialGaugeComponent::UCPRadialGaugeComponent()
{
	SetWidgetSpace(EWidgetSpace::World);
	SetDrawSize(FVector2D(80.0f, 80.0f));
	SetPivot(FVector2D(0.5f, 0.5f));
	SetTwoSided(true);
	SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	// 고정 회전값은 더 이상 의미가 없음 - TickComponent가 매 프레임 카메라를 향해 월드 회전을
	// 새로 계산해서 덮어씀
}

void UCPRadialGaugeComponent::BeginPlay()
{
	if (GaugeWidgetClass)
	{
		SetWidgetClass(GaugeWidgetClass);
	}

	Super::BeginPlay();

	GaugeWidget = Cast<UCPRadialGaugeWidget>(GetUserWidgetObject());
}

void UCPRadialGaugeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 소유자가 로컬 플레이어의 Pawn이면 그 플레이어 자신의 카메라를 우선 사용 (예: 각자의 리바이브
	// 게이지는 자기 화면 기준으로 카메라를 바라봐야 함) - 아니면(테스트 액터 등) 0번 로컬 플레이어의
	// 카메라로 대체
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	const APlayerController* OwningPlayerController = OwningPawn ? Cast<APlayerController>(OwningPawn->GetController()) : nullptr;

	APlayerCameraManager* CameraManager = nullptr;
	if (OwningPlayerController && OwningPlayerController->PlayerCameraManager)
	{
		CameraManager = OwningPlayerController->PlayerCameraManager;
	}
	else
	{
		CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	}

	if (!CameraManager)
	{
		return;
	}

	const FVector ToCamera = CameraManager->GetCameraLocation() - GetComponentLocation();
	if (!ToCamera.IsNearlyZero())
	{
		SetWorldRotation(ToCamera.Rotation());
	}
}

void UCPRadialGaugeComponent::UpdateGauge(float CurrentValue, float MaxValue)
{
	if (GaugeWidget)
	{
		GaugeWidget->UpdateGauge(CurrentValue, MaxValue);
	}
}

void UCPRadialGaugeComponent::SetGaugeEnabled(bool bEnabled)
{
	SetVisibility(bEnabled);
	SetActive(bEnabled);
}
