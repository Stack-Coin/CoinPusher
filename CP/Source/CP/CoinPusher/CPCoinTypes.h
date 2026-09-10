// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPCoinTypes.generated.h"

/** Coin의 종류. ACPCoin::SetCoinType()으로 전환되는 순간 타입별 연출/행동이 트리거된다 */
UENUM(BlueprintType)
enum class ECPCoinType : uint8
{
	//일반 코인
	Normal,
	//패시브 코인 - 전환되는 순간 최소 크기로 줄었다가 최대 크기로 늘어난 뒤 원래 크기로 돌아오는 스케일 연출이 재생됨
	Passive,
	//Big(대왕) 코인 - 전환되는 순간 지정된 Mesh/Material로 바뀌고 스케일이 BigScaleMultiplier배로 커짐(원상복구 없음).
	//CoinPusher의 Collision(Floor/Wall)에 처음 부딪히면 그 CoinPusher의 ActiveWaveThrow()를 1회 실행시킴
	Big,
	//HP 코인 - 전환되는 순간 Passive와 동일한 스케일 연출(최소→최대→원래 크기)이 재생됨
	HP,
	//Monster 코인 - 전환되는 순간 Passive/HP와 동일한 스케일 연출(최소→최대→원래 크기)이 재생되지만,
	//최대 크기에 도달해도 CoinThrowArea는 활성화되지 않음
	Monster,

	//코인 타워용
	CoinTower,
};
