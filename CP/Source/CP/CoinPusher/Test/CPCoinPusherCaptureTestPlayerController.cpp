// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherCaptureTestPlayerController.h"
#include "CPCoinPusherCaptureTestActor.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "CoinPusher/CPCoinPusherCaptureWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
	// 전용 테스트 액터(ACPCoinPusherCaptureTestActor)가 없는 레벨(예: 실제 BP_CoinPusher가 배치된
	// 테스트 레벨)에서도 동작하도록, 없으면 실제 ACPCoinPusher를 대신 찾는다
	AActor* FindCaptureSourceActor(const UObject* WorldContextObject)
	{
		if (AActor* TestActor = UGameplayStatics::GetActorOfClass(WorldContextObject, ACPCoinPusherCaptureTestActor::StaticClass()))
		{
			return TestActor;
		}

		return UGameplayStatics::GetActorOfClass(WorldContextObject, ACPCoinPusher::StaticClass());
	}

	UCPCoinPusherViewCaptureComponent* GetCaptureComponent(AActor* SourceActor)
	{
		if (ACPCoinPusherCaptureTestActor* TestActor = Cast<ACPCoinPusherCaptureTestActor>(SourceActor))
		{
			return TestActor->GetViewCaptureComponent();
		}

		if (ACPCoinPusher* CoinPusher = Cast<ACPCoinPusher>(SourceActor))
		{
			return CoinPusher->GetViewCaptureComponent();
		}

		return nullptr;
	}
}

ACPCoinPusherCaptureTestPlayerController::ACPCoinPusherCaptureTestPlayerController()
{
	CaptureWidgetClass = UCPCoinPusherCaptureWidget::StaticClass();
}

void ACPCoinPusherCaptureTestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	// Pawn 위치는 건드리지 않는다 - 레벨에 배치된 PlayerStart를 그대로 신뢰한다 (예전엔 여기서 Pawn을
	// 캡처 대상 쪽으로 강제로 이동시켰는데, 그러면 PlayerStart에 스폰된 것처럼 안 보이는 문제가 있었다)

	// 캡처 대상의 ViewCaptureComponent가 자신의 BeginPlay에서 RenderTarget을 만드는데, 액터
	// 간 BeginPlay 순서는 보장되지 않으므로 한 틱 미뤄서 항상 RenderTarget이 준비된 뒤에 연결한다
	GetWorldTimerManager().SetTimerForNextTick(this, &ACPCoinPusherCaptureTestPlayerController::SetupCaptureWidget);
}

void ACPCoinPusherCaptureTestPlayerController::SetupCaptureWidget()
{
	if (!CaptureWidgetClass)
	{
		return;
	}

	UCPCoinPusherViewCaptureComponent* CaptureComponent = GetCaptureComponent(FindCaptureSourceActor(this));
	if (!CaptureComponent)
	{
		return;
	}

	UCPCoinPusherCaptureWidget* CaptureWidget = CreateWidget<UCPCoinPusherCaptureWidget>(this, CaptureWidgetClass);
	if (!CaptureWidget)
	{
		return;
	}

	// UCPCoinPusherViewportClient가 Player 카메라를 오른쪽으로 축소해뒀으므로, 이 위젯은
	// AddToPlayerScreen이 아니라 뷰포트 전체 기준으로 추가되어야 왼쪽 영역까지 그릴 수 있다
	CaptureWidget->AddToViewport(0);
	CaptureWidget->SetCaptureTexture(CaptureComponent->GetViewRenderTarget());
}
