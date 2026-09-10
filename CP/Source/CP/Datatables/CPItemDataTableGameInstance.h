// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CPItemDataTableGameInstance.generated.h"

class UDataTable;

/**
 *  아이템 마스터 데이터(FItemData 행) 테이블을 들고 있는 GameInstance. Project Settings > Maps &
 *  Modes > Game Instance Class에 이 클래스를 상속한 BP를 지정해두면, GameInstance는 OpenLevel로
 *  레벨이 바뀌어도 살아남으므로 어디서든 GetGameInstance()로 접근해 ItemDataTable을 조회할 수 있다.
 */
UCLASS(abstract)
class CP_API UCPItemDataTableGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:

	/** 아이템 마스터 데이터 테이블. Row Struct는 FItemData여야 한다 */
	UPROPERTY(EditDefaultsOnly, Category="Item Data")
	TObjectPtr<UDataTable> ItemDataTable;

public:

	FORCEINLINE UDataTable* GetItemDataTable() const { return ItemDataTable; }
};
