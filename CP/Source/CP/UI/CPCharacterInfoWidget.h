// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCharacterInfoWidget.generated.h"

class UCPHorizonGuageBarWidget;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 *  캐릭터(플레이어, 보스 등) 정보를 한 화면에 모아 보여주는 UI. 체력/경험치 게이지
 *  (둘 다 UCPHorizonGuageBarWidget), 이름(NameText), 레벨(LevelText), 초상화(PortraitImage)로
 *  구성된다. 다섯 컴포넌트 전부 BindWidgetOptional이라, WBP에 배치하지 않은 항목은 해당 값을
 *  갱신해도 아무 동작도 하지 않는다(에러 없이 조용히 무시 - 표시할 것이 없으면 표시하지 않는다).
 */
UCLASS(abstract)
class CP_API UCPCharacterInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 체력을 나타내는 게이지 바 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPHorizonGuageBarWidget> HealthGaugeWidget;

	/** 경험치를 나타내는 게이지 바 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPHorizonGuageBarWidget> ExpGaugeWidget;

	/** 캐릭터 이름을 표시할 TextBlock (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** 캐릭터 레벨을 표시할 TextBlock (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** LevelText에 적용할 표시 형식. {0}=Level. 기본값은 자리표시자일 뿐 - 실제 문구는 이 클래스를
	 *  상속하는 Widget Blueprint의 Class Defaults에서 지정한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Info")
	FText LevelDisplayFormat = FText::FromString(TEXT("Lv.{0}"));

	/** 캐릭터 초상화를 표시할 Image (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;

public:

	/** 체력 변경 델리게이트에 바인딩해서 쓰는 진입점 - HealthGaugeWidget이 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	/** 경험치 변경 델리게이트에 바인딩해서 쓰는 진입점 - ExpGaugeWidget이 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void UpdateExp(float CurrentExp, float MaxExp);

	/** 캐릭터 이름을 설정 - NameText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetCharacterName(const FText& CharacterName);

	/** 캐릭터 레벨을 설정(LevelDisplayFormat으로 포맷) - LevelText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetLevel(int32 Level);

	/** 캐릭터 초상화를 설정 - PortraitImage가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetPortrait(UTexture2D* Portrait);
};
