// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** CoinPusher(ACPCoinPusher, ACPDispenser, ACPCoinTowerSpawner, 관련 테스트 Actor/Pawn 등)와
 *  관련된 로그. DropZone/Roulette처럼 별도 카테고리가 있는 항목은 제외 */
DECLARE_LOG_CATEGORY_EXTERN(LogCoinPusher, Log, All);

/** ACPDropZone(아이템/코인 드랍 수거, ICPDroppedItemReceiver로의 전달 등)과 관련된 로그 */
DECLARE_LOG_CATEGORY_EXTERN(LogDropZone, Log, All);

/** ACPRoulette(추첨, 스핀, OnPickedUp 등)와 관련된 로그 */
DECLARE_LOG_CATEGORY_EXTERN(LogRoulette, Log, All);

/** 플레이어(ACPPlayerCharacter, 인벤토리, 컨트롤러 등)와 관련된 로그 */
DECLARE_LOG_CATEGORY_EXTERN(LogPlayer, Log, All);
