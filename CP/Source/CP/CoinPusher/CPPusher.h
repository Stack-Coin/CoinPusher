// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPPusher.generated.h"

class UStaticMeshComponent;

UCLASS(abstract)
class CP_API ACPPusher : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* PushPlate;

public:
	ACPPusher();

protected:

	//최대 앞으로 가는 거리
	UPROPERTY(EditAnywhere, Category="Pusher", meta = (ClampMin = 0, Units = "cm"))
	float PushDistance = 100.0f;

	//왕복 운동 속도
	UPROPERTY(EditAnywhere, Category="Pusher", meta = (ClampMin = 0))
	float CycleSpeed = 0.5f;

	//시작 위치
	FVector StartRelativeLocation = FVector::ZeroVector;

	//왕복 운동 구현용 누적 시간
	float ElapsedTime = 0.0f;

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	//컴포넌트 반환
	FORCEINLINE UStaticMeshComponent* GetPushPlate() const { return PushPlate; }
};
