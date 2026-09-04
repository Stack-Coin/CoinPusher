// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPHealthBarWidget.generated.h"

class UImage;

/**
 *  체력바 위젯의 베이스 클래스. Level에 배치된 Actor 위(월드 스페이스, UCPHealthBarComponent) 또는
 *  화면 뷰포트(스크린 스페이스, UCPViewportHealthBarComponent) 어느 쪽에든 동일하게 쓸 수 있다.
 *
 *  배경(BackgroundImage) 위에 실제 체력을 나타내는 FillImage가 겹쳐 있는 구조. FillImage는
 *  Current/Max 비율만큼 가로 폭이 자동으로 줄어든다(SetHealthPercent 기본 구현이 RenderScale로
 *  처리) - WBP에서 별도 그래프 작업 없이도 바로 동작한다. 왼쪽 끝을 고정한 채 오른쪽에서
 *  왼쪽으로 줄어들도록, NativeConstruct에서 FillImage의 Render Transform Pivot을 (0.0, 0.5)로
 *  코드에서 직접 맞춰준다 (WBP 디자이너에서 따로 설정할 필요 없음).
 *  색상 변화, 애니메이션 등 커스텀 연출이 필요하면 SetHealthPercent를 WBP에서 오버라이드해서
 *  Super 호출 여부를 직접 결정하면 된다 (BlueprintNativeEvent).
 *
 *  대상 Actor의 체력 변경 델리게이트를 UpdateHealth에 바인딩해두면 자동으로 갱신된다.
 */
UCLASS(abstract)
class CP_API UCPHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 체력바 틀 역할을 하는 배경 이미지 (선택 사항 - 없어도 동작에는 지장 없음) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** 실제 체력을 나타내는 앞쪽 이미지. Current/Max 비율만큼 가로로 줄어든다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> FillImage;

	/** FillImage의 Render Transform Pivot을 (0.0, 0.5)로 맞춰, 왼쪽 끝을 고정한 채 오른쪽에서
	 *  왼쪽으로 줄어들게 한다 */
	virtual void NativeConstruct() override;

public:

	/** 대상 Actor의 체력 변경 델리게이트에 바인딩해서 쓰는 진입점. Percent를 계산해
	 *  SetHealthPercent/SetHealthValues를 호출해준다 */
	UFUNCTION(BlueprintCallable, Category="Health Bar")
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	/** 체력바를 0-1 Percent에 맞춰 갱신한다. 기본 구현은 FillImage의 RenderScale.X를 Percent로
	 *  설정해 가로 폭을 줄인다 - WBP에서 오버라이드해 커스텀 연출(색상 변화 등)을 추가할 수 있다 */
	UFUNCTION(BlueprintNativeEvent, Category="Health Bar")
	void SetHealthPercent(float Percent);
	virtual void SetHealthPercent_Implementation(float Percent);

	/** Passes the raw current/max health values through, for widgets that also display them as text */
	UFUNCTION(BlueprintImplementableEvent, Category="Health Bar")
	void SetHealthValues(float CurrentHealth, float MaxHealth);
};
