// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPUserWidget_NexusHpBar.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPUserWidget_NexusHpBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual bool Initialize() override;
	void UpdateHpBar(float NewCurrentHp);

	FORCEINLINE void SetMaxHp(float NewMaxHp) { MaxHp = NewMaxHp; }

protected:
	UPROPERTY()
	TObjectPtr<class UProgressBar> HpProgressBar;

	UPROPERTY()
	float MaxHp = 100.f;
};
