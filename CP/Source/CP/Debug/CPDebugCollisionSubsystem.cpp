// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/CPDebugCollisionSubsystem.h"

void UCPDebugCollisionSubsystem::SetCategoryVisible(ECPDebugCollisionCategory Category, bool bVisible)
{
	bool& Current = CategoryVisibility.FindOrAdd(Category);
	if (Current == bVisible)
	{
		return;
	}

	Current = bVisible;

	OnCollisionVisibilityChanged.Broadcast(Category, bVisible);
}

bool UCPDebugCollisionSubsystem::IsCategoryVisible(ECPDebugCollisionCategory Category) const
{
	const bool* Found = CategoryVisibility.Find(Category);
	return Found ? *Found : false;
}
