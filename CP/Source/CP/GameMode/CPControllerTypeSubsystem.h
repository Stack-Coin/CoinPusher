// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CPControllerType.h"
#include "CPControllerTypeSubsystem.generated.h"

/**
 *  시작 화면(UCPStartScreenWidget)에서 감지된 컨트롤러 종류(게임패드/키보드+마우스)를 GameInstance에
 *  붙어서 보관한다. GameInstance(따라서 이 서브시스템)는 위젯 전환은 물론 OpenLevel로 레벨이 바뀌어도
 *  살아남으므로, 게임 설명 화면(UCPGameExplanationWidget)이나 그 다음 실제 게임플레이 레벨에서도
 *  "플레이어가 어떤 컨트롤러를 선택했는지"를 이 서브시스템을 통해 질의할 수 있다.
 *  UCPPlayerRegistrySubsystem과 달리 "누가 몇 번째 플레이어인지"가 아니라 "어떤 장치 종류를 쓰는지"만
 *  다루는 별개의 관심사라 분리된 서브시스템으로 둔다. Project Settings에 별도로 등록할 필요 없이
 *  자동으로 생성된다.
 */
UCLASS()
class CP_API UCPControllerTypeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	ECPControllerType SelectedControllerType = ECPControllerType::KeyboardMouse;

public:

	/** 시작 화면 등에서 감지/확정된 컨트롤러 종류를 기록 */
	UFUNCTION(BlueprintCallable, Category="Controller Type")
	void SetSelectedControllerType(ECPControllerType NewType) { SelectedControllerType = NewType; }

	/** 현재까지 선택된(또는 기본값) 컨트롤러 종류를 반환 */
	UFUNCTION(BlueprintPure, Category="Controller Type")
	ECPControllerType GetSelectedControllerType() const { return SelectedControllerType; }
};
