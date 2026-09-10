// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPStartScreenTestPlayerController.generated.h"

/**
 *  시작 화면/게임 설명 화면 테스트용 최소 구성 PlayerController. ACPLobbyPlayerController와 동일한
 *  이유로 Input Mapping Context를 추가하지 않는다 - 이 화면의 입력은 UCPStartScreenWidget/
 *  UCPGameExplanationWidget이 각자 FCPAnyInputProcessor로 직접 가로채므로 PlayerController가
 *  입력 모드/포커스를 따로 관리해줄 필요가 없다.
 *  디버그용으로 EKeys::AnyKey를 걸어 입력이 감지될 때마다 UCPControllerTypeSubsystem에 기록된
 *  현재 선택 컨트롤러를 로그로 찍어준다 - 위젯의 이미지 토글이 눈으로 확인하기 어려운 환경(원격
 *  데스크톱 등)에서도 Output Log만으로 동작을 확인할 수 있다.
 */
UCLASS()
class CP_API ACPStartScreenTestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ACPStartScreenTestPlayerController();

protected:

	/** 마우스 커서를 끄고, UI가 입력을 받을 수 있도록 기본 입력 모드를 세팅해두는 안전장치
	 *  (위젯이 자기 NativeConstruct에서 FCPAnyInputProcessor를 등록하므로 필수는 아니지만,
	 *  혹시 위젯이 이 컨트롤러가 소유하지 않은 다른 경로로 표시되는 경우에도 최소 동작을 보장) */
	virtual void BeginPlay() override;

	/** 디버그 전용 - EKeys::AnyKey 바인딩 */
	virtual void SetupInputComponent() override;

	/** 디버그용 - 아무 입력이나 눌렸을 때 UCPControllerTypeSubsystem의 현재 선택 컨트롤러를 로그로 표시 */
	void HandleAnyKeyPressed_Debug();
};
