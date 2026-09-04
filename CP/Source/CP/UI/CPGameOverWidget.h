// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPGameOverWidget.generated.h"

/**
 *  게임 오버 화면 WBP의 베이스. 화면에 띄우는 시점(CreateWidget + AddToViewport)과 실제
 *  비주얼/연출은 이 클래스를 상속하는 Widget Blueprint에서 담당한다 (콘텐츠는 항상 BP에서 -
 *  CombatLifeBar와 동일 관례). RestartLevel은 재시작 버튼 등에서 바로 호출해서 쓸 수 있는
 *  공용 진입점
 */
UCLASS(abstract)
class CP_API UCPGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** 현재 레벨을 다시 로드한다 (재시작 버튼 등에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Game Over")
	void RestartLevel();
};
