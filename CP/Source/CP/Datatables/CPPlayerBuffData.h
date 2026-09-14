// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CPPlayerBuffData.generated.h"

class UTexture2D;

/** 플레이어 버프 마스터 데이터 한 행. UI/CPBuffIconWidget.h의 UCPBuffIconWidget::PlayerBuffDataTable의
 *  Row Struct로 쓰인다 */
USTRUCT(BlueprintType)
struct FCPPlayerBuffData : public FTableRowBase
{
	GENERATED_BODY()

	/** 식별용 버프 ID. DataTable의 Row Name과 동일한 값으로 등록해야
	 *  FindRow<FCPPlayerBuffData>(BuffID, ...)로 조회 가능 (실제 조회는 Row Name 기준이며 이
	 *  필드 자체는 참조용/가독성용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Buff Data")
	FName BuffID;

	/** 이 버프의 아이콘으로 표시할 이미지. UCPBuffIconWidget::SetBuffCode()가 BuffID(=BuffCode)로
	 *  이 행을 찾아 BackgroundImage에 적용한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Buff Data")
	TObjectPtr<UTexture2D> BuffImage = nullptr;
};
