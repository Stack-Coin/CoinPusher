// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Debug/CPDebugTypes.h"
#include "CPDebugCollisionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPDebugCollisionVisibilityChanged, ECPDebugCollisionCategory, Category, bool, bVisible);

/**
 *  UCPDebugCollisionSubsystem
 *  Single source of truth for the F1 debug widget's per-category "show collision" checkboxes. Auto-created
 *  once per world, so any actor can reach it via GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>()
 *  without needing to be told about the debug widget directly. Actors that own a collision shape relevant
 *  to a category read the current state on BeginPlay and subscribe to OnCollisionVisibilityChanged so
 *  toggling a checkbox immediately reaches every live instance (see UCPDebugCollisionShapeComponent).
 */
UCLASS()
class CP_API UCPDebugCollisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:

	/** Current on/off state per category. Missing entries default to false via TMap::FindOrAdd/Find */
	TMap<ECPDebugCollisionCategory, bool> CategoryVisibility;

public:

	/** Sets whether Category's collision shape(s) should currently be drawn, and broadcasts the change if it actually changed */
	UFUNCTION(BlueprintCallable, Category="Debug")
	void SetCategoryVisible(ECPDebugCollisionCategory Category, bool bVisible);

	/** Returns whether Category's collision shape(s) are currently set to be drawn. False until toggled on at least once */
	UFUNCTION(BlueprintPure, Category="Debug")
	bool IsCategoryVisible(ECPDebugCollisionCategory Category) const;

	/** Broadcast whenever SetCategoryVisible actually changes a category's state */
	UPROPERTY(BlueprintAssignable, Category="Debug")
	FOnCPDebugCollisionVisibilityChanged OnCollisionVisibilityChanged;
};
