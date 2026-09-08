// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherCaptureTestPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"

ACPCoinPusherCaptureTestPawn::ACPCoinPusherCaptureTestPawn()
{
	if (UFloatingPawnMovement* FloatingMovement = Cast<UFloatingPawnMovement>(GetMovementComponent()))
	{
		FloatingMovement->MaxSpeed = 600.0f;
		FloatingMovement->Acceleration = 4000.0f;
		FloatingMovement->Deceleration = 4000.0f;
	}

	// UCPCoinPusherViewportClient가 이 Pawn의 카메라 뷰포트를 화면 오른쪽 일부로 축소하는데,
	// Aspect Ratio를 강제로 고정해두면 그 축소된 영역 안에서 다시 레터박스(검은 띠)가 생길 수 있다
	if (UCameraComponent* Camera = FindComponentByClass<UCameraComponent>())
	{
		Camera->bConstrainAspectRatio = false;
	}
}
