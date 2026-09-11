// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPHorizonGuageBarWidget.generated.h"

class UImage;
class UTextBlock;

/**
 *  가로 방향으로 채워지는 범용 게이지 바 위젯의 베이스 클래스. 체력, 경험치, 콤보 게이지 등
 *  0~1 비율로 표현 가능한 어떤 값에도 재사용할 수 있다. Level에 배치된 Actor 위(월드 스페이스,
 *  UCPHealthBarComponent) 또는 화면 뷰포트(스크린 스페이스, UCPViewportHealthBarComponent)
 *  어느 쪽에든 동일하게 쓸 수 있다.
 *
 *  배경(BackgroundImage) 위에 실제 값을 나타내는 FillImage가 겹쳐 있는 구조. FillImage는
 *  Current/Max 비율만큼 가로 폭이 자동으로 줄어든다(SetPercent 기본 구현이 RenderScale로
 *  처리) - WBP에서 별도 그래프 작업 없이도 바로 동작한다. 왼쪽 끝을 고정한 채 오른쪽에서
 *  왼쪽으로 줄어들도록, NativeConstruct에서 FillImage의 Render Transform Pivot을 (0.0, 0.5)로
 *  코드에서 직접 맞춰준다 (WBP 디자이너에서 따로 설정할 필요 없음).
 *  색상 변화, 애니메이션 등 커스텀 연출이 필요하면 SetPercent를 WBP에서 오버라이드해서
 *  Super 호출 여부를 직접 결정하면 된다 (BlueprintNativeEvent).
 *
 *  ValueText(선택 사항)에는 SetValues 기본 구현이 DisplayFormat("{0} / {1}")으로 Current/Max
 *  값을 그대로 표시해준다 - WBP에서 별도 그래프 작업 없이도 "37 / 100" 같은 텍스트가 바로 나온다.
 *
 *  값을 갖고 있는 쪽의 변경 델리게이트를 Update에 바인딩해두면 자동으로 갱신된다.
 */
UCLASS(abstract)
class CP_API UCPHorizonGuageBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 게이지 틀 역할을 하는 배경 이미지 (선택 사항 - 없어도 동작에는 지장 없음) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** 실제 값을 나타내는 앞쪽 이미지. Current/Max 비율만큼 가로로 줄어든다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> FillImage;

	/** Current/Max 값을 텍스트로 표시할 TextBlock (선택 사항 - 없으면 텍스트 갱신만 생략) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	/** ValueText에 적용할 표시 형식. {0}=Current, {1}=Max. 기본값은 자리표시자일 뿐 - 실제 문구는
	 *  이 클래스를 상속하는 Widget Blueprint의 Class Defaults에서 지정한다 (소스 코드에 한글
	 *  리터럴을 직접 넣으면 컴파일러 소스 인코딩에 따라 깨질 수 있어 피한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gauge Bar")
	FText DisplayFormat = FText::FromString(TEXT("{0} / {1}"));

	/** FillImage의 Render Transform Pivot을 (0.0, 0.5)로 맞춰, 왼쪽 끝을 고정한 채 오른쪽에서
	 *  왼쪽으로 줄어들게 한다 */
	virtual void NativeConstruct() override;

public:

	/** 값을 갖고 있는 쪽의 변경 델리게이트에 바인딩해서 쓰는 진입점. Percent를 계산해
	 *  SetPercent/SetValues를 호출해준다 */
	UFUNCTION(BlueprintCallable, Category="Gauge Bar")
	void Update(float CurrentValue, float MaxValue);

	/** 게이지 바를 0-1 Percent에 맞춰 갱신한다. 기본 구현은 FillImage의 RenderScale.X를 Percent로
	 *  설정해 가로 폭을 줄인다 - WBP에서 오버라이드해 커스텀 연출(색상 변화 등)을 추가할 수 있다 */
	UFUNCTION(BlueprintNativeEvent, Category="Gauge Bar")
	void SetPercent(float Percent);
	virtual void SetPercent_Implementation(float Percent);

	/** Current/Max 값을 그대로 전달. 기본 구현은 ValueText가 있으면 DisplayFormat으로 채워준다 -
	 *  WBP에서 오버라이드해 다른 표시 방식(예: 퍼센트 표시)으로 바꿀 수 있다 (Super 호출 여부는 자유) */
	UFUNCTION(BlueprintNativeEvent, Category="Gauge Bar")
	void SetValues(float CurrentValue, float MaxValue);
	virtual void SetValues_Implementation(float CurrentValue, float MaxValue);
};
