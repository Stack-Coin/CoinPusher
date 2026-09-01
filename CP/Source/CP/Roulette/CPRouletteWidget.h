// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPRouletteWidget.generated.h"

class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRouletteResultDetermined, int32, ResultIndex);

/**
 *  원형 돌림판 룰렛 UI. PlaySpin()이 호출되면 화면 중앙 위쪽에서 아래로 슬라이드하며 나타난 뒤,
 *  Wheel 이미지를 여러 바퀴 돌려 ResultIndex번째 칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다.
 *  칸이 결정되면 OnResultDetermined를 브로드캐스트하고, PostResultHideDelay 후 스스로 사라진다.
 */
UCLASS(abstract)
class CP_API UCPRouletteWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 8칸이 그려진 회전판 이미지. RenderTransform Angle을 돌려 스핀을 표현한다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* WheelImage;

	/** 화면 중앙에 도달하기까지 걸리는 등장 연출 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0, Units = "s"))
	float EnterDuration = 0.4f;

	/** 등장을 시작하는 위치 (중앙 기준 Y축 오프셋, 화면 위쪽이 음수) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	float EnterStartOffsetY = -600.0f;

	/** 스핀(회전)에 걸리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.1, Units = "s"))
	float SpinDuration = 3.0f;

	/** 스핀 중 완전 회전 최소/최대 횟수 (연출용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0))
	int32 MinFullSpins = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0))
	int32 MaxFullSpins = 6;

	/** 결과가 결정된 후, 위젯이 스스로 사라지기까지 대기하는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0, Units = "s"))
	float PostResultHideDelay = 2.0f;

private:

	enum class ERouletteState : uint8
	{
		None,
		Entering,
		Spinning
	};

	ERouletteState State = ERouletteState::None;

	/** 현재 연출(등장/스핀) 단계의 경과 시간 */
	float StateElapsedTime = 0.0f;

	/** 스핀 시작/도착 각도 (WheelImage RenderTransform 기준) */
	float SpinStartAngle = 0.0f;
	float SpinTargetAngle = 0.0f;

	/** PlaySpin()으로 넘어온, 스핀이 끝나면 확정될 칸 인덱스 */
	int32 PendingResultIndex = 0;

	/** 결과 확정 후 위젯을 숨기는 데 사용하는 타이머 */
	FTimerHandle HideTimerHandle;

public:

	/** 결과 칸이 결정되면 브로드캐스트 (스핀 애니메이션이 멈춘 직후) */
	UPROPERTY(BlueprintAssignable, Category="Roulette")
	FOnRouletteResultDetermined OnResultDetermined;

	/** 위젯을 화면에 표시하고, NumSlots개의 칸 중 ResultIndex번째 칸에서 멈추도록 등장+스핀 연출을 시작 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	void PlaySpin(int32 ResultIndex, int32 NumSlots);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 스핀이 끝났을 때 처리: 결과 브로드캐스트 + 자동 숨김 타이머 시작 */
	void FinishSpin();

	/** 숨김 타이머에 바인딩 */
	void HideRoulette();
};
