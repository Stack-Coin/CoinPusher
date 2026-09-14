// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCharacterInfoWidget.generated.h"

class UCPHorizonGuageBarWidget;
class UCPBuffIconWidget;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 *  캐릭터(플레이어, 보스 등) 정보를 한 화면에 모아 보여주는 UI. 체력/경험치 게이지
 *  (둘 다 UCPHorizonGuageBarWidget), 이름(NameText), 레벨(LevelText), 초상화(PortraitImage),
 *  버프 아이콘 목록(BuffHorizontalBox)으로 구성된다. 여섯 컴포넌트 전부 BindWidgetOptional이라,
 *  WBP에 배치하지 않은 항목은 해당 값을 갱신해도 아무 동작도 하지 않는다(에러 없이 조용히 무시 -
 *  표시할 것이 없으면 표시하지 않는다).
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

	/** 버프 아이콘들이 가로로 나열될 박스 (선택 사항) - BuffCreate()가 만든 UCPBuffIconWidget
	 *  인스턴스를 여기에 추가한다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BuffHorizontalBox;

	/** BuffCreate()가 생성할 버프 아이콘 위젯 클래스 (없으면 BuffCreate()는 아무 동작도 하지
	 *  않고 nullptr을 반환한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Info")
	TSubclassOf<UCPBuffIconWidget> BuffIconWidgetClass;

public:

	/** 체력 변경 델리게이트에 바인딩해서 쓰는 진입점 - HealthGaugeWidget이 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	/** 경험치 변경 델리게이트에 바인딩해서 쓰는 진입점 - ExpGaugeWidget이 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void UpdateExp(float CurrentExp, float MaxExp);

	/** HealthGaugeWidget의 ValueText를 DisplayFormat 없이 임의의 문구로 직접 설정 - 없으면 무시 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetHealthText(const FText& Text);

	/** ExpGaugeWidget의 ValueText를 DisplayFormat 없이 임의의 문구로 직접 설정 - 없으면 무시 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetExpText(const FText& Text);

	/** 캐릭터 이름을 설정 - NameText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetCharacterName(const FText& CharacterName);

	/** 캐릭터 레벨을 설정(LevelDisplayFormat으로 포맷) - LevelText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetLevel(int32 Level);

	/** 캐릭터 초상화를 설정 - PortraitImage가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	void SetPortrait(UTexture2D* Portrait);

	/** BuffIconWidgetClass의 인스턴스를 하나 생성해 BuffHorizontalBox에 추가하고 그 참조를
	 *  반환한다 - 반환된 인스턴스의 UpdateBuff(CurrentTime, MaxTime)는 호출부가 직접 관리해야
	 *  한다(예: 버프 지속시간을 관리하는 타이머). BuffHorizontalBox/BuffIconWidgetClass가 없으면
	 *  아무 동작도 하지 않고 nullptr을 반환한다 */
	UFUNCTION(BlueprintCallable, Category="Character Info")
	UCPBuffIconWidget* BuffCreate(FName BuffCode);
};
