// Fill out your copyright notice in the Description page of Project Settings.

#include "CPRouletteWidget.h"
#include "Components/Image.h"
#include "TimerManager.h"

void UCPRouletteWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UCPRouletteWidget::PlaySpin(int32 ResultIndex, int32 NumSlots)
{
	if (NumSlots <= 0)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	PendingResultIndex = ResultIndex;

	// 등장 연출은 항상 EnterStartOffsetY에서 다시 시작
	SetRenderTranslation(FVector2D(0.0f, EnterStartOffsetY));

	const float SlotAngle = 360.0f / NumSlots;

	// 이어서 도는 것처럼 보이도록 시작 각도를 현재 값으로 잡는다
	SpinStartAngle = WheelImage ? WheelImage->GetRenderTransform().Angle : 0.0f;

	const int32 FullSpins = FMath::RandRange(MinFullSpins, MaxFullSpins);

	// ResultIndex번째 칸이 (0도 = 위쪽) 화살표 아래로 오도록 정렬
	const float AlignAngle = -(ResultIndex * SlotAngle);

	// 항상 정방향으로만 굴러가도록, 이미 지나온 회전수를 반영해 목표 각도를 보정
	float TargetAngle = FullSpins * 360.0f + AlignAngle;
	while (TargetAngle <= SpinStartAngle)
	{
		TargetAngle += 360.0f;
	}
	SpinTargetAngle = TargetAngle;

	StateElapsedTime = 0.0f;
	State = ERouletteState::Entering;

	SetVisibility(ESlateVisibility::Visible);
}

void UCPRouletteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (State == ERouletteState::None)
	{
		return;
	}

	StateElapsedTime += InDeltaTime;

	if (State == ERouletteState::Entering)
	{
		const float Alpha = EnterDuration > 0.0f ? FMath::Clamp(StateElapsedTime / EnterDuration, 0.0f, 1.0f) : 1.0f;
		const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
		const float CurrentOffsetY = FMath::Lerp(EnterStartOffsetY, 0.0f, EasedAlpha);

		SetRenderTranslation(FVector2D(0.0f, CurrentOffsetY));

		if (Alpha >= 1.0f)
		{
			StateElapsedTime = 0.0f;
			State = ERouletteState::Spinning;
		}

		return;
	}

	if (State == ERouletteState::Spinning)
	{
		const float Alpha = FMath::Clamp(StateElapsedTime / SpinDuration, 0.0f, 1.0f);
		const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
		const float CurrentAngle = FMath::Lerp(SpinStartAngle, SpinTargetAngle, EasedAlpha);

		if (WheelImage)
		{
			WheelImage->SetRenderTransformAngle(CurrentAngle);
		}

		if (Alpha >= 1.0f)
		{
			State = ERouletteState::None;
			FinishSpin();
		}
	}
}

void UCPRouletteWidget::FinishSpin()
{
	OnResultDetermined.Broadcast(PendingResultIndex);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HideTimerHandle, this, &UCPRouletteWidget::HideRoulette, PostResultHideDelay, false);
	}
}

void UCPRouletteWidget::HideRoulette()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
