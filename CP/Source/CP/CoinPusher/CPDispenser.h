// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPDispenser.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ACPInput;

UCLASS(abstract)
class CP_API ACPDispenser : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnPoint;

public:

	ACPDispenser();

protected:

	//Spawn할 오브젝트 클래스. ICPCoinPusherItem을 구현하는 Actor여야 함 (ACPCoin, ACPItem 등)
	UPROPERTY(EditAnywhere, Category="Dispenser")
	TSubclassOf<AActor> ItemClass;

	//앞 방향 발사 속도 cm/s
	UPROPERTY(EditAnywhere, Category="Dispenser", meta = (ClampMin = 0, Units = "cm/s"))
	float LaunchForwardSpeed = 500.0f;

	//위 방향 발사 속도 cm/s
	UPROPERTY(EditAnywhere, Category="Dispenser", meta = (ClampMin = 0, Units = "cm/s"))
	float LaunchUpwardSpeed = 150.0f;

	//디스펜서를 작동 시키는 입력 엑터. ChildActorComponent로 스폰되므로 직접 편집하지 않고
	//소유자인 ACPCoinPusher가 SetLinkedInput()을 통해 설정한다. 천장 디스펜서처럼 Input 없이
	//코드로만 작동하는 경우에는 비워둘 수 있다
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Dispenser")
	TObjectPtr<ACPInput> LinkedInput;

public:

	virtual void BeginPlay() override;

	//ItemClass를 SpawnPoint에서 하나 생성하고 발사
	UFUNCTION(BlueprintCallable, Category="Dispenser")
	virtual void DispenseItem();

	//ItemClass를 Count개 연속으로 생성하고 발사
	UFUNCTION(BlueprintCallable, Category="Dispenser")
	void DispenseItems(int32 Count);

	//소유자(ACPCoinPusher)가 LinkedInput을 설정할 때 사용. 기존에 연결되어 있던 Input의 델리게이트는 해제하고 새 Input에 다시 바인딩한다
	UFUNCTION(BlueprintCallable, Category="Dispenser")
	void SetLinkedInput(ACPInput* NewLinkedInput);

protected:

	//연결된 Input이 상호작용 하는 멤버 함수
	UFUNCTION()
	void HandleInputInteracted(AActor* Interactor);

	//LinkedInput의 OnInteracted 델리게이트에 (중복 없이) 바인딩
	void BindToLinkedInput();

public:

	FORCEINLINE UStaticMeshComponent* GetBody() const { return Body; }

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }

	FORCEINLINE ACPInput* GetLinkedInput() const { return LinkedInput; }
};
