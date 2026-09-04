// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "CPPlayerJoinWidget.generated.h"

/**
 *  로컬 2인 플레이 참가 화면에 놓는 "아무 버튼이나 눌러 참가하세요" UI. 포커스를 가진 동안 감지한
 *  입력을 ACPLobbyGameMode::RegisterPlayerInput으로 전달하고, ACPLobbyGameMode::OnPlayerJoined를
 *  구독해서 자신이 몇 번째(P1/P2)로 배정됐는지를 OnPlayerSlotAssigned로 BP에 알려준다.
 *  실제 "참가 완료" 비주얼(텍스트/이미지 변경 등)은 OnPlayerSlotAssigned를 받는 WBP 그래프에서 구현
 */
UCLASS(abstract)
class CP_API UCPPlayerJoinWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	// TODO: FReply 사용 시 "unresolved external symbol FReply::FReply(bool)" 링크 에러 발생
	// (CP.Build.cs에 SlateCore 모듈 의존성 누락이 원인) - 해결 전까지 주석 처리
	//virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** ACPLobbyGameMode::OnPlayerJoined에 바인딩되는 함수 - 그대로 BP 이벤트로 전달 */
	UFUNCTION()
	void HandlePlayerJoined(int32 PlayerIndex);

public:

	/** 플레이어 슬롯이 배정될 때마다 호출 (배정된 사람이 자신인지 여부와 무관하게, 로비의 모든
	 *  참가 현황을 이 위젯에서 함께 보여주고 싶을 수 있어 매번 알려준다). PlayerIndex는 0부터
	 *  시작 (0 = P1, 1 = P2, ...) */
	UFUNCTION(BlueprintImplementableEvent, Category="Player Join")
	void OnPlayerSlotAssigned(int32 PlayerIndex);
};
