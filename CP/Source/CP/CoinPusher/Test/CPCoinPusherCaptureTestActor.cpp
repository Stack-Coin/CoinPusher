// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherCaptureTestActor.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"

ACPCoinPusherCaptureTestActor::ACPCoinPusherCaptureTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	RootComponent = DisplayMesh;
	// 실제 표시할 Static Mesh는 BP/에디터에서 지정 (예: 기본 큐브 등 아무 메시나 확인용으로 충분)

	// ViewCaptureComponent는 SpringArm 소켓에 붙어서 동작. 기본값은 위에서 내려다보는 구도이고,
	// ArmLength/각도는 ViewCaptureBoom을 통해 BP/디테일 패널에서 조정
	ViewCaptureBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ViewCaptureBoom"));
	ViewCaptureBoom->SetupAttachment(DisplayMesh);
	ViewCaptureBoom->TargetArmLength = 400.0f;
	ViewCaptureBoom->SetRelativeRotation(FRotator(-70.0f, 0.0f, 0.0f));
	ViewCaptureBoom->bUsePawnControlRotation = false;
	ViewCaptureBoom->bDoCollisionTest = false;

	ViewCaptureComponent = CreateDefaultSubobject<UCPCoinPusherViewCaptureComponent>(TEXT("ViewCaptureComponent"));
	ViewCaptureComponent->SetupAttachment(ViewCaptureBoom, USpringArmComponent::SocketName);
}
