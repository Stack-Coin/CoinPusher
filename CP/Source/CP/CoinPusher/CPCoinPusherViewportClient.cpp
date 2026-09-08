// Fill out your copyright notice in the Description page of Project Settings.

#include "CoinPusher/CPCoinPusherViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"

void UCPCoinPusherViewportClient::LayoutPlayers()
{
	// 스플릿스크린 등 엔진 기본 레이아웃을 먼저 계산한 뒤, 1P의 Origin/Size만 덮어써서 왼쪽
	// CaptureWidthRatio 영역을 비워둔다 (그 자리는 UCPCoinPusherCaptureWidget이 채운다)
	Super::LayoutPlayers();

	UGameInstance* OwningGameInstance = GetGameInstance();
	if (!OwningGameInstance)
	{
		return;
	}

	const TArray<ULocalPlayer*>& LocalPlayers = OwningGameInstance->GetLocalPlayers();
	if (LocalPlayers.IsEmpty())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = LocalPlayers[0];
	if (!LocalPlayer)
	{
		return;
	}

	const float ClampedRatio = FMath::Clamp(CaptureWidthRatio, 0.0f, 1.0f);

	LocalPlayer->Origin = FVector2D(ClampedRatio, 0.0f);
	LocalPlayer->Size = FVector2D(1.0f - ClampedRatio, 1.0f);
}
