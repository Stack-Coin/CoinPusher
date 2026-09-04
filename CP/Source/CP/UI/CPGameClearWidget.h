// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPGameClearWidget.generated.h"

/**
 *  게임 클리어 화면 WBP의 베이스. 화면에 띄우는 시점(CreateWidget + AddToViewport)과 실제
 *  비주얼/연출은 이 클래스를 상속하는 Widget Blueprint에서 담당한다 (콘텐츠는 항상 BP에서).
 *  GoToNextLevel은 다음/타이틀로 버튼 등에서 바로 호출해서 쓸 수 있는 공용 진입점
 */
UCLASS(abstract)
class CP_API UCPGameClearWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** GoToNextLevel이 열 레벨 (다음 스테이지, 또는 타이틀로 돌아가는 등) */
	UPROPERTY(EditAnywhere, Category="Game Clear")
	FName NextLevelName;

public:

	/** NextLevelName을 연다 (다음/타이틀로 버튼 등에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Game Clear")
	void GoToNextLevel();
};
