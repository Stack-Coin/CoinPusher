// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPStatBoostItemEffect.h"
#include "Player/CPStatInterface.h"

void UCPStatBoostItemEffect::ApplyEffect(TScriptInterface<ICPStatInterface> Target)
{
	if (ICPStatInterface* StatInterface = Target.GetInterface())
	{
		StatInterface->ModifyStat(StatToBoost, Amount);
	}
}
