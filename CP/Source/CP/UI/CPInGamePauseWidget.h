// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPInGamePauseWidget.generated.h"

class UButton;
class UImage;
class UWidget;

/**
 *  게임을 일시정지하고 띄우는 메뉴 UI의 베이스(InGamePause 메뉴 자체, 그리고 이를 상속하는
 *  UCPEndingWidget이 공유). 반투명 배경 이미지(키보드/마우스용과 게임패드용 두 종류 - 현재 선택된
 *  ECPControllerType에 맞춰 하나만 보임)와 "게임 종료"/"타이틀로 돌아가기" 두 버튼으로 구성된다.
 *  버튼 선택은 두 가지 입력 경로를 하나의 SelectedButtonIndex로 통합해서 처리한다:
 *  키보드/마우스는 포인터가 버튼 위로 올라오면(OnHovered) 그 버튼이 선택되고 클릭(OnClicked)하면
 *  즉시 실행되며, 게임패드는 ACPTopDownPlayerController가 L-Stick 입력을 MoveSelection()으로,
 *  A버튼 입력을 ConfirmSelection()으로 전달해 조작한다. 선택된 버튼은 그에 대응하는 Outline 위젯을
 *  보이게 해서 윤곽선으로 표시된다.
 */
UCLASS(abstract)
class CP_API UCPInGamePauseWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 키보드/마우스 사용 중일 때 보이는 반투명 배경 이미지 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> KeyboardMouseBackgroundImage;

	/** 게임패드 사용 중일 때 보이는 반투명 배경 이미지 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> GamePadBackgroundImage;

	/** 게임을 종료하는 버튼 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndGameButton;

	/** EndGameButton이 선택됐을 때 보이는 윤곽선 위젯 (Border/Image 등 자유롭게 사용) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EndGameButtonOutline;

	/** 타이틀 레벨(TitleLevelName)로 돌아가는 버튼 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnToTitleButton;

	/** ReturnToTitleButton이 선택됐을 때 보이는 윤곽선 위젯 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ReturnToTitleButtonOutline;

	/** ReturnToTitleButton이 열 레벨 (타이틀 화면) */
	UPROPERTY(EditAnywhere, Category="Menu")
	FName TitleLevelName;

	/** NativeConstruct에서 채워지는, 존재하는 버튼만 모은 목록 (MoveSelection이 이 순서로 순환) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> NavigableButtons;

	/** NavigableButtons와 같은 순서의 윤곽선 위젯 목록 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> ButtonOutlines;

	/** 현재 선택된(윤곽선이 표시되는) 버튼의 NavigableButtons 인덱스 */
	int32 SelectedButtonIndex = 0;

	virtual void NativeConstruct() override;

	/** 현재 UCPControllerTypeSubsystem에 기록된 컨트롤러 종류에 맞춰 배경 이미지를 전환 */
	void RefreshBackgroundForControllerType();

	/** SelectedButtonIndex에 해당하는 윤곽선만 보이고 나머지는 숨기도록 갱신 */
	void UpdateSelectionVisuals();

	/** NavigableButtons 상의 인덱스로 선택을 옮기고 시각효과를 갱신 (범위를 벗어나면 무시) */
	void SetSelectedButtonIndex(int32 NewIndex);

	/** Index번째 버튼의 기능을 실행 (EndGameButton/ReturnToTitleButton 중 어느 쪽인지에 따라 분기) */
	void ExecuteButtonAction(int32 Index);

	/** EndGameButton의 OnHovered에 바인딩 - 그 버튼을 선택 상태로 만듦 */
	UFUNCTION()
	void HandleEndGameButtonHovered();

	/** EndGameButton의 OnClicked에 바인딩 - 선택 후 즉시 실행 */
	UFUNCTION()
	void HandleEndGameButtonClicked();

	/** ReturnToTitleButton의 OnHovered에 바인딩 - 그 버튼을 선택 상태로 만듦 */
	UFUNCTION()
	void HandleReturnToTitleButtonHovered();

	/** ReturnToTitleButton의 OnClicked에 바인딩 - 선택 후 즉시 실행 */
	UFUNCTION()
	void HandleReturnToTitleButtonClicked();

	/** 게임을 종료 (EndGameButton의 실제 동작) */
	virtual void EndGame();

	/** TitleLevelName을 연다 (ReturnToTitleButton의 실제 동작) */
	virtual void ReturnToTitle();

public:

	/** 이 위젯이 화면에 다시 표시될 때마다(재사용되는 인스턴스일 수 있으므로) 배경 이미지와 선택
	 *  상태를 초기화하기 위해 호출 - ACPTopDownPlayerController가 SetVisibility(Visible) 직전에 호출 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void RefreshForDisplay();

	/** 게임패드 L-Stick 입력 - Delta만큼(보통 -1/+1) NavigableButtons 상에서 선택을 순환 이동 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void MoveSelection(int32 Delta);

	/** 게임패드 A버튼 입력 - 현재 선택된 버튼의 기능을 실행 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void ConfirmSelection();
};
