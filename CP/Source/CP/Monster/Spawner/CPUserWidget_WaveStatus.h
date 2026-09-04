// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "CPUserWidget_WaveStatus.generated.h"

/**
 * 화면에 "웨이브 진행 상황"을 텍스트로 보여주는 아주 단순한 위젯.
 * NexusHpBar와 동일하게, 별도 위젯 블루프린트 없이 C++만으로 동작합니다.
 * 단계(Phase)에 따라 표시 형식이 달라집니다.
 *  - Spawning: "웨이브 1/5 - 스폰 5/25초" (지난 스폰 시간 / 이번 웨이브 전체 스폰 소요 시간)
 *  - WaveWait: "웨이브 1/5 - 다음 웨이브 3/10초"
 *  - RoundWait: "라운드 종료 대기 4/15초" (기획서의 라운드 대기시간)
 *  - Finished: "웨이브 종료"
 */
UCLASS()
class CP_API UCPUserWidget_WaveStatus : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	void UpdateWaveStatus(int32 CurrentWave, int32 TotalWaves, ECPWavePhase Phase, int32 ProgressSeconds, int32 TotalSeconds);

protected:
	UPROPERTY()
	TObjectPtr<class UTextBlock> WaveText;
};
