// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CPStartScreenTestGameMode.generated.h"

class UUserWidget;

/**
 *  UCPStartScreenWidget → UCPGameExplanationWidget → (선택된 컨트롤러로) 다음 레벨 흐름을 고립된
 *  레벨에서 테스트하기 위한 최소 구성 GameMode. ACPLobbyGameMode와 동일한 패턴으로,
 *  BeginPlay에서 StartWidgetClass(보통 UCPStartScreenWidget 상속 WBP)를 자동으로 화면에 띄워주므로
 *  레벨 블루프린트 등에서 따로 위젯을 생성/배치할 필요가 없다 - 이 GameMode를 상속하는 BP를 만들어
 *  StartWidgetClass만 Class Defaults에서 지정하면 바로 테스트할 수 있다.
 *  순수 UI 테스트라 DefaultPawnClass는 비워둔다(조종할 Pawn 불필요).
 */
UCLASS()
class CP_API ACPStartScreenTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACPStartScreenTestGameMode();

protected:

	/** 레벨 시작 시 자동으로 화면에 띄울 위젯 (보통 UCPStartScreenWidget 상속 WBP). 그 위젯이
	 *  자신의 NextWidgetClass를 통해 게임 설명 화면(UCPGameExplanationWidget 상속 WBP)으로
	 *  스스로 전환해준다 */
	UPROPERTY(EditDefaultsOnly, Category="UI Test")
	TSubclassOf<UUserWidget> StartWidgetClass;

	virtual void BeginPlay() override;
};
