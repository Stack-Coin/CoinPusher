// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CPShaderTestGameMode.generated.h"

class ACameraActor;

/**
 *  셰이더 테스트 레벨에서, 레벨에 배치해 둔 CameraActor를 플레이어 카메라로 사용하기 위한 GameMode.
 *  Pawn을 스폰/Possess하지 않고, BeginPlay에서 레벨에 배치된 ACameraActor를 찾아 PlayerController의
 *  ViewTarget으로 지정한다. CameraActorTag를 지정하면 해당 Tag가 붙은 CameraActor를 우선 사용하고,
 *  지정하지 않으면 레벨에서 찾은 첫 번째 CameraActor를 사용한다.
 */
UCLASS()
class CP_API ACPShaderTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACPShaderTestGameMode();

protected:

	/** 여러 개의 CameraActor가 배치되어 있을 때, 이 Tag가 붙은 CameraActor를 우선 사용한다.
	 *  비워두면 레벨에서 찾은 첫 번째 CameraActor를 사용한다 */
	UPROPERTY(EditDefaultsOnly, Category="Shader Test")
	FName CameraActorTag;

	virtual void BeginPlay() override;

	/** 레벨에서 사용할 CameraActor를 찾는다 (CameraActorTag가 지정되어 있으면 해당 Tag 우선) */
	ACameraActor* FindTargetCameraActor() const;
};
