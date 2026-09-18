// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPRouletteWidget.generated.h"

class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRouletteResultDetermined, int32, ResultIndex);

/**
 *  원형 돌림판 룰렛 UI. PlaySpin()이 호출되면 화면 중앙 위쪽에서 아래로 슬라이드하며 나타난 뒤,
 *  Wheel 이미지를 여러 바퀴 돌려 ResultIndex번째 칸이 (고정된) 위쪽 화살표 아래에서 멈추도록 연출한다.
 *  스핀이 멈추면 곧바로 결과를 알리지 않고, 먼저 PickUpImage를 PlaySpin()에 전달된 텍스처로 채워
 *  PickUpDisplayDuration 동안 보여준다 - 이 시간이 지나면 PickUpImage가 사라짐과 동시에
 *  OnResultDetermined를 브로드캐스트하고(Roulette에 보상이 처리됨) 위젯 자신도 함께 Collapsed된다.
 */
UCLASS(abstract)
class CP_API UCPRouletteWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 8칸이 그려진 회전판 이미지. RenderTransform Angle을 돌려 스핀을 표현한다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* WheelImage;

	/** 스핀이 끝난 직후, PickUpDisplayDuration 동안 당첨 아이템 이미지를 보여주는 PickUp 연출용
	 *  이미지. PlaySpin()에 전달된 텍스처로 채워진 뒤 SpinDuration이 지나야 보이기 시작하고,
	 *  PickUpDisplayDuration이 지나면 다시 숨겨진다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* PickUpImage;

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

	/** 스핀이 끝난 뒤 PickUpImage를 보여주는 시간 - 이 시간이 지나면 PickUpImage가 사라짐과 동시에
	 *  결과(OnResultDetermined)가 처리되고 이 위젯도 함께 Collapsed된다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0, Units = "s"))
	float PickUpDisplayDuration = 1.5f;

private:

	enum class ERouletteState : uint8
	{
		None,
		Entering,
		Spinning,
		ShowingPickUp
	};

	ERouletteState State = ERouletteState::None;

	/** 현재 연출(등장/스핀) 단계의 경과 시간 */
	float StateElapsedTime = 0.0f;

	/** 스핀 시작/도착 각도 (WheelImage RenderTransform 기준) */
	float SpinStartAngle = 0.0f;
	float SpinTargetAngle = 0.0f;

	/** PlaySpin()으로 넘어온, 스핀이 끝나면 확정될 칸 인덱스 */
	int32 PendingResultIndex = 0;

	/** PlaySpin()으로 넘어온, 스핀이 끝나면 PickUpImage에 채울 텍스처 */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PendingPickUpTexture;

public:

	/** 결과 칸이 결정되면 브로드캐스트 (스핀 애니메이션이 멈춘 직후) */
	UPROPERTY(BlueprintAssignable, Category="Roulette")
	FOnRouletteResultDetermined OnResultDetermined;

	/** 위젯을 화면에 표시하고, NumSlots개의 칸 중 ResultIndex번째 칸에서 멈추도록 등장+스핀 연출을
	 *  시작한다. PickUpTexture는 스핀이 끝난 뒤 PickUpImage에 채울 당첨 아이템 이미지 (RouletteDataTable
	 *  행의 PickUpImage - 널이면 PickUpImage 텍스처를 갱신하지 않고 기존 상태 그대로 보여준다) */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	void PlaySpin(int32 ResultIndex, int32 NumSlots, UTexture2D* PickUpTexture = nullptr);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 스핀이 끝났을 때 처리: PickUpImage를 PendingPickUpTexture로 채워 보이게 하고 ShowingPickUp
	 *  상태로 전환 (PickUpImage가 없으면 즉시 FinishSpin()으로 넘어감) */
	void ShowPickUp();

	/** PickUpDisplayDuration이 끝났을 때 처리: PickUpImage를 숨기고 결과를 브로드캐스트한 뒤,
	 *  위젯 자신도 곧바로 Collapsed 처리 */
	void FinishSpin();
};
