// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TestActor/CPRadialGaugeTestActor.h"
#include "UI/CPRadialGaugeComponent.h"
#include "TimerManager.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ACPRadialGaugeTestActor::ACPRadialGaugeTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RadialGauge = CreateDefaultSubobject<UCPRadialGaugeComponent>(TEXT("RadialGauge"));
}

void ACPRadialGaugeTestActor::BeginPlay()
{
	Super::BeginPlay();

	// 콘텐츠(Input Action/Mapping Context) 없이 바로 테스트할 수 있도록 레거시 raw key 입력으로
	// A 키를 바인딩한다 - 테스트 전용, 실제 게임플레이 입력은 Enhanced Input 사용
	EnableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ACPRadialGaugeTestActor::ToggleGauge);
	}

	if (RadialGauge)
	{
		RadialGauge->UpdateGauge(CurrentValue, MaxValue);
	}

	if (ChangeInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ValueTimerHandle, this, &ACPRadialGaugeTestActor::HandleValueTick, ChangeInterval, true);
	}
}

void ACPRadialGaugeTestActor::HandleValueTick()
{
	CurrentValue += ChangeAmount;
	if (CurrentValue > MaxValue)
	{
		CurrentValue = 0.0f;
	}

	if (RadialGauge)
	{
		RadialGauge->UpdateGauge(CurrentValue, MaxValue);
	}
}

void ACPRadialGaugeTestActor::ToggleGauge()
{
	bGaugeEnabled = !bGaugeEnabled;

	if (RadialGauge)
	{
		RadialGauge->SetGaugeEnabled(bGaugeEnabled);
	}
}
