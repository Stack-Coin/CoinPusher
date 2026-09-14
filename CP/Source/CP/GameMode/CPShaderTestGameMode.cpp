// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPShaderTestGameMode.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ACPShaderTestGameMode::ACPShaderTestGameMode()
{
	// 순수 카메라 뷰용 레벨이라 조종할 Pawn이 필요 없음
	DefaultPawnClass = nullptr;
}

void ACPShaderTestGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPShaderTestGameMode::BeginPlay - Player 0의 PlayerController를 찾지 못했습니다."));
		return;
	}

	ACameraActor* TargetCameraActor = FindTargetCameraActor();
	if (!TargetCameraActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPShaderTestGameMode::BeginPlay - 레벨에서 CameraActor를 찾지 못했습니다."));
		return;
	}

	PC->SetViewTarget(TargetCameraActor);
}

ACameraActor* ACPShaderTestGameMode::FindTargetCameraActor() const
{
	TArray<AActor*> FoundCameraActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameraActors);

	if (FoundCameraActors.Num() == 0)
	{
		return nullptr;
	}

	if (!CameraActorTag.IsNone())
	{
		for (AActor* FoundActor : FoundCameraActors)
		{
			if (FoundActor && FoundActor->ActorHasTag(CameraActorTag))
			{
				return Cast<ACameraActor>(FoundActor);
			}
		}
	}

	return Cast<ACameraActor>(FoundCameraActors[0]);
}
