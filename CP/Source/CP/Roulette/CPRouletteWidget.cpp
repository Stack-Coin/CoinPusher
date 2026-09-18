// Fill out your copyright notice in the Description page of Project Settings.

#include "CPRouletteWidget.h"
#include "Components/Image.h"
#include "Log/CPLogCategories.h"

void UCPRouletteWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	if (PickUpImage)
	{
		PickUpImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCPRouletteWidget::NativeDestruct()
{
	UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] NativeDestruct - State at destruction: %d"), *GetNameSafe(this), static_cast<int32>(State));

	Super::NativeDestruct();
}

void UCPRouletteWidget::PlaySpin(int32 ResultIndex, int32 NumSlots, UTexture2D* PickUpTexture)
{
	UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] PlaySpin called - ResultIndex: %d, NumSlots: %d, prev State: %d, Visibility: %d"),
		*GetNameSafe(this), ResultIndex, NumSlots, static_cast<int32>(State), static_cast<int32>(GetVisibility()));

	if (NumSlots <= 0)
	{
		UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] PlaySpin aborted - NumSlots <= 0"), *GetNameSafe(this));
		return;
	}

	PendingResultIndex = ResultIndex;
	PendingPickUpTexture = PickUpTexture;

	if (PickUpImage)
	{
		PickUpImage->SetVisibility(ESlateVisibility::Collapsed);
	}

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
			UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] State -> Spinning"), *GetNameSafe(this));
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
			StateElapsedTime = 0.0f;
			ShowPickUp();
		}

		return;
	}

	if (State == ERouletteState::ShowingPickUp)
	{
		if (StateElapsedTime >= PickUpDisplayDuration)
		{
			FinishSpin();
		}
	}
}

void UCPRouletteWidget::ShowPickUp()
{
	UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] ShowPickUp - PickUpImage: %s"), *GetNameSafe(this), *GetNameSafe(PickUpImage));

	if (!PickUpImage)
	{
		// PickUp 연출용 이미지가 없으면 보여줄 것이 없으므로 대기 없이 바로 결과를 처리한다
		FinishSpin();
		return;
	}

	if (PendingPickUpTexture)
	{
		PickUpImage->SetBrushFromTexture(PendingPickUpTexture, false);
	}

	PickUpImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	State = ERouletteState::ShowingPickUp;
	UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] State -> ShowingPickUp"), *GetNameSafe(this));
}

void UCPRouletteWidget::FinishSpin()
{
	UE_LOG(LogRoulette, Warning, TEXT("[UCPRouletteWidget:%s] FinishSpin - broadcasting OnResultDetermined(%d)"), *GetNameSafe(this), PendingResultIndex);

	State = ERouletteState::None;

	if (PickUpImage)
	{
		PickUpImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnResultDetermined.Broadcast(PendingResultIndex);

	// OnResultDetermined 처리 체인(연쇄 자동 롤 등)이 브로드캐스트 도중 같은 위젯으로 곧장 다음
	// 스핀을 시작시켰을 수 있으므로, 그 경우 여기서 다시 Collapsed로 되돌리면 안 된다
	if (State == ERouletteState::None)
	{
		// PickUpImage가 사라지는 시점에 룰렛 UI 전체도 함께 비활성화한다
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
