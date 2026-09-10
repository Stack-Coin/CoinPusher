// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPControllerType.generated.h"

/** 플레이어가 사용 중인 입력 장치 종류. 시작 화면에서 감지/선택되어 다음 화면/레벨로 전달된다 */
UENUM(BlueprintType)
enum class ECPControllerType : uint8
{
	KeyboardMouse,
	GamePad
};
