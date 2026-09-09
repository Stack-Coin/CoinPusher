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

	//�ִ� ������ ���� �Ÿ�
	UPROPERTY(EditAnywhere, Category="Pusher", meta = (ClampMin = 0, Units = "cm"))
	float PushDistance = 100.0f;

	//�պ� � �ӵ�
	UPROPERTY(EditAnywhere, Category="Pusher", meta = (ClampMin = 0))
	float CycleSpeed = 0.5f;

	//���� ��ġ
	FVector StartRelativeLocation = FVector::ZeroVector;

	//�պ� � ������ ���� �ð�
	float ElapsedTime = 0.0f;

	//���� �����ӿ� ������ �и� �Ÿ�. AddLocalOffset���� �̵��� ��Ÿ ��꿡 ���
	//true면 Tick에서 왕복 운동 로직(ElapsedTime 누적 포함)을 완전히 건너뜀. CoinTowerSpawner 등 외부에서
	//Pusher를 잠시 멈추고 다른 곳으로 옮겨야 할 때 사용
	bool bIsPaused = false;

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	//Pusher의 왕복 운동을 멈추거나 재개. 매 틱 PushPlate 위치를 StartRelativeLocation(왕복 운동의 기준
	//위치, BeginPlay 시점에 한 번만 기록됨) 기준 절대 위치로 다시 계산해서 적용하므로, 일시정지 중
	//외부에서 액터를 다른 곳(CoinTowerSpawner의 BackPosition 등)으로 옮겨놓았더라도 재개하는 순간
	//자동으로 기준 위치를 기준으로 한 올바른 왕복 지점으로 들어가 이어서 진행된다 - 재개 전에 액터를
	//원래 위치로 되돌려 놓을 필요가 없음
	UFUNCTION(BlueprintCallable, Category="Pusher")
	void SetPusherPaused(bool bPaused) { bIsPaused = bPaused; }

	UFUNCTION(BlueprintPure, Category="Pusher")
	bool IsPusherPaused() const { return bIsPaused; }

public:
	//������Ʈ ��ȯ
	FORCEINLINE UStaticMeshComponent* GetPushPlate() const { return PushPlate; }
};
