// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPUserWidget_WaveStatus.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

bool UCPUserWidget_WaveStatus::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	WaveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtWaveStatus"));
	WidgetTree->RootWidget = WaveText;
	WaveText->SetJustification(ETextJustify::Center);

	return true;
}

void UCPUserWidget_WaveStatus::UpdateWaveStatus(int32 CurrentWave, int32 TotalWaves, ECPWavePhase Phase, int32 ProgressSeconds, int32 TotalSeconds)
{
	if (!WaveText)
	{
		return;
	}

	FString Message;
	switch (Phase)
	{
	case ECPWavePhase::Spawning:
		Message = FString::Printf(TEXT("웨이브 %d/%d - 스폰 %d/%d초"), CurrentWave, TotalWaves, ProgressSeconds, TotalSeconds);
		break;

	case ECPWavePhase::WaveWait:
		Message = FString::Printf(TEXT("웨이브 %d/%d - 다음 웨이브 %d/%d초"), CurrentWave, TotalWaves, ProgressSeconds, TotalSeconds);
		break;

	case ECPWavePhase::RoundWait:
		Message = FString::Printf(TEXT("라운드 종료 대기 %d/%d초"), ProgressSeconds, TotalSeconds);
		break;

	case ECPWavePhase::Finished:
	default:
		Message = TEXT("웨이브 종료");
		break;
	}

	WaveText->SetText(FText::FromString(Message));
}
