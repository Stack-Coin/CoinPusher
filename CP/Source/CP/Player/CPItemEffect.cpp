// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPItemEffect.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Actor.h"

void UCPItemEffect::ApplyEffect(TScriptInterface<ICPStatInterface> Target)
{
	// Intentionally empty - see the class comment. Override in a subclass to add real behavior.
}

void UCPItemEffect::PlayPickupEffect(AActor* TargetActor) const
{
	if (!PickupEffect || !TargetActor)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(PickupEffect, TargetActor->GetRootComponent(), NAME_None,
		PickupEffectLocationOffset, PickupEffectRotationOffset, PickupEffectScale,
		EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
}
