// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterWaypointArea.generated.h"

class USphereComponent;

UCLASS()
class CP_API ACPMonsterWaypointArea : public AActor
{
	GENERATED_BODY()

public:
	ACPMonsterWaypointArea();

public:
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	bool GetRandomPointInArea(FVector& OutPoint, const AActor* RequestingActor = nullptr) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
	TObjectPtr<USphereComponent> AreaShape;
};
