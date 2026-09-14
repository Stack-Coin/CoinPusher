// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPBuffIconWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UDataTable;

/**
 *  UCPCharacterInfoWidget의 버프 목록(HorizontalBox)에 버프 하나당 하나씩 배치되는 아이콘 위젯.
 *  BorderImage(테두리) + BackgroundImage(배경 - 버프 아이콘 텍스처를 여기 지정) 위에 MaskImage
 *  (원형 마스크로 남은 시간을 깎아내는 오버레이 - UCPRadialGaugeWidget과 동일한, Material의
 *  Scalar Parameter를 갱신하는 방식)와 CountDownText(남은 시간 숫자)를 겹쳐 보여준다.
 *
 *  UpdateBuff(CurrentTime, MaxTime)을 호출부(주로 버프를 관리하는 게임플레이 코드)가 주기적으로
 *  불러서 갱신한다. CurrentTime이 0 이하가 되면 이 위젯 스스로 RemoveFromParent()로 사라지므로,
 *  호출부가 별도로 제거할 필요가 없다.
 *
 *  WBP에서 준비해야 하는 것: MaskImage의 Brush(Image)에 UCPRadialGaugeWidget과 마찬가지로 각도
 *  기반 마스크 Material을 지정해야 한다(Radial Gradient Exponential 노드 등으로 만들 수 있음).
 *  Material 없이 텍스처만 지정하면 파라미터를 갱신할 대상이 없어 아무 효과도 나지 않는다.
 */
UCLASS(abstract)
class CP_API UCPBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 아이콘 테두리 이미지 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BorderImage;

	/** 버프 아이콘 텍스처를 보여줄 배경 이미지 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** 남은 시간을 원형으로 깎아내는 마스크 이미지 - Brush에 각도 기반 마스크 Material이 지정돼
	 *  있어야 UpdateBuff()가 갱신하는 Percent 파라미터가 실제로 보인다 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> MaskImage;

	/** 남은 시간(초)을 숫자로 보여줄 TextBlock (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountDownText;

	/** MaskImage에 지정한 Material의 Scalar Parameter 중, 남은 비율(0~1)을 나타내는 파라미터
	 *  이름. Material 쪽 파라미터 이름과 반드시 일치해야 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff Icon")
	FName PercentParameterName = TEXT("Percent");

	/** CountDownText에 적용할 표시 형식. {0}=남은 시간(초, 정수로 올림). 기본값은 자리표시자일
	 *  뿐 - 실제 문구는 이 클래스를 상속하는 Widget Blueprint의 Class Defaults에서 지정한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff Icon")
	FText CountDownDisplayFormat = FText::FromString(TEXT("{0}"));

	/** BuffCreate()로 이 아이콘을 만들 때 전달된 BuffCode - 어떤 버프의 아이콘인지 식별용 */
	UPROPERTY(BlueprintReadOnly, Category="Buff Icon")
	FName BuffCode;

	/** BuffCode(RowName)로 FCPPlayerBuffData 행을 조회할 때 쓰는 데이터 테이블 (Row Struct는
	 *  FCPPlayerBuffData여야 함) - SetBuffCode()가 이 테이블에서 찾은 행의 BuffImage를
	 *  BackgroundImage에 적용한다 */
	UPROPERTY(EditAnywhere, Category="Buff Icon")
	TObjectPtr<UDataTable> PlayerBuffDataTable;

	/** MaskImage의 Material로부터 만든 Dynamic Material Instance. NativeConstruct에서 한 번만
	 *  생성해서 캐싱해두고, 이후로는 파라미터 값만 갱신한다 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MaskMaterialInstance;

	/** MaskImage의 Brush에 지정된 Material로부터 MaskMaterialInstance를 만든다 */
	virtual void NativeConstruct() override;

public:

	/** 버프 상태를 갱신한다: MaskImage를 CurrentTime/MaxTime 비율로 갱신하고, CountDownText를
	 *  CurrentTime으로 갱신한다. CurrentTime이 0 이하이면 갱신 대신 이 위젯을 RemoveFromParent()로
	 *  제거한다(호출부가 별도로 지울 필요 없음) */
	UFUNCTION(BlueprintCallable, Category="Buff Icon")
	void UpdateBuff(float CurrentTime, float MaxTime);

	/** UCPCharacterInfoWidget::BuffCreate()가 생성 직후 호출 - 이 아이콘이 어떤 버프인지 기록하고,
	 *  PlayerBuffDataTable에서 NewBuffCode(RowName)에 해당하는 FCPPlayerBuffData 행을 찾아 그
	 *  BuffImage를 BackgroundImage에 적용한다(SetBrushFromTexture). PlayerBuffDataTable이 없거나,
	 *  행을 못 찾거나, BackgroundImage/BuffImage가 없으면 아이콘 텍스처는 그대로 둔다(BuffCode
	 *  기록은 항상 이뤄짐) */
	UFUNCTION(BlueprintCallable, Category="Buff Icon")
	void SetBuffCode(FName NewBuffCode);

	/** BuffCreate()에 전달됐던 BuffCode를 반환 */
	UFUNCTION(BlueprintPure, Category="Buff Icon")
	FName GetBuffCode() const { return BuffCode; }
};
