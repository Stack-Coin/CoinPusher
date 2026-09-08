// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPPlayerJoinWidget.generated.h"

class IInputProcessor;
class UImage;

/**
 *  로컬 2인 플레이 참가 화면에 놓는 "아무 버튼이나 눌러 참가하세요" UI. 포커스나 히트테스트에
 *  의존하지 않고, 전역 입력 프로세서(FCPAnyInputProcessor)로 감지한 입력을
 *  ACPLobbyGameMode::RegisterPlayerInput으로 전달한다 - 이 위젯이 화면 어디를 덮고 있는지,
 *  어떤 위젯이 키보드 포커스를 가졌는지와 무관하게 항상 동작한다 (게임패드 입력은 특히
 *  SetUserFocus 기반 포커스 라우팅이 신뢰할 수 없는 것이 확인됨 - UI/README.md 참고).
 *  ACPLobbyGameMode::OnPlayerJoined를 구독해서 자신이 몇 번째(P1/P2)로 배정됐는지를
 *  OnPlayerSlotAssigned로 알려주는데, 그 기본 구현(OnPlayerSlotAssigned_Implementation)이
 *  Player1Square/Player2Square(둘 다 BindWidgetOptional Image)를 각각 Player1Color/
 *  Player2Color로 칠해준다 - 먼저 입력한 사람(PlayerIndex 0)은 빨간색, 늦게 입력한 사람
 *  (PlayerIndex 1)은 파란색이 기본값. WBP에서 오버라이드해서 다른 연출을 추가할 수도 있다
 *  (BlueprintNativeEvent라 Super 호출 여부는 자유)
 */
UCLASS(abstract)
class CP_API UCPPlayerJoinWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 1P(먼저 입력한 사람)가 배정되면 Player1Color로 칠해질 네모 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Player1Square;

	/** 2P(나중에 입력한 사람)가 배정되면 Player2Color로 칠해질 네모 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Player2Square;

	/** Player1Square에 칠할 색 (기본 빨강) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Join")
	FLinearColor Player1Color = FLinearColor::Red;

	/** Player2Square에 칠할 색 (기본 파랑) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Join")
	FLinearColor Player2Color = FLinearColor::Blue;

	/** 이 위젯이 살아있는 동안 등록해두는 전역 입력 프로세서. NativeDestruct에서 해제한다 */
	TSharedPtr<IInputProcessor> AnyKeyInputProcessor;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 입력이 감지됐을 때 공통으로 처리하는 내부 함수 - 그 입력 장치를 GameMode에 전달 */
	void HandleAnyInputPressed(FInputDeviceId DeviceId);

	/** ACPLobbyGameMode::OnPlayerJoined에 바인딩되는 함수 - 그대로 BP 이벤트로 전달 */
	UFUNCTION()
	void HandlePlayerJoined(int32 PlayerIndex);

public:

	/** 플레이어 슬롯이 배정될 때마다 호출 (배정된 사람이 자신인지 여부와 무관하게, 로비의 모든
	 *  참가 현황을 이 위젯에서 함께 보여주고 싶을 수 있어 매번 알려준다). PlayerIndex는 0부터
	 *  시작 (0 = P1, 1 = P2, ...) */
	UFUNCTION(BlueprintNativeEvent, Category="Player Join")
	void OnPlayerSlotAssigned(int32 PlayerIndex);
	void OnPlayerSlotAssigned_Implementation(int32 PlayerIndex);
};
