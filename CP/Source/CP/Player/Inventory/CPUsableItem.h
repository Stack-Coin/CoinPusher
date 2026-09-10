#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Player/CPItemTypes.h"
#include "CPUsableItem.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPUsableItem : public UInterface
{
	GENERATED_BODY()
};

class ICPUsableItem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Item")
	virtual void UseItem(AActor* User, const FCPItemData& ItemData) = 0;
};
