// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusher.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UChildActorComponent;
class ACPDispenser;
class ACPDropZone;
class ACPInput;

/**Broadcast whenever this CoinPusher's health changes as a result of damage */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinPusherDamaged, float, Damage, AActor*, DamageCauser);

/** Broadcast when this CoinPusher's health reaches zero */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoinPusherDestroyed);

UCLASS(abstract)
class CP_API ACPCoinPusher : public AActor
{
	GENERATED_BODY()

	//코인이 놓이는 바닥 (RootComponent, 실제 충돌 담당). BoxExtent는 항상 (1,1,1)로 고정하고
	//실제 크기는 Scale로 표현한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Floor;

	//Body Mesh (콜리전 없음 - 순수 비주얼, Floor에 부착)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Body;

	//왼쪽 벽 (BoxExtent (1,1,1) 고정, Scale로 실제 크기 표현)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWall;

	//오른쪽 벽 (BoxExtent (1,1,1) 고정, Scale로 실제 크기 표현)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWall;

	//뒷 벽 (BoxExtent (1,1,1) 고정, Scale로 실제 크기 표현)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BackWall;

	//Pusher ActorComponent (컴포넌트를 통한 Has-a)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* PusherComponent;

	//Dispenser ActorComponent (컴포넌트를 통한 Has-a) - 2개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DispenserComponentA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DispenserComponentB;

	//DropZone ActorComponent (컴포넌트를 통한 Has-a)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DropZoneComponent;

public:
	ACPCoinPusher();

protected:

	//각 Dispenser를 작동시키는 Input 액터. 레벨에 배치한 ACPInput을 여기서 연결하면
	//PostInitializeComponents에서 자동으로 해당 Dispenser의 LinkedInput으로 설정된다
	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPInput> InputA;

	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPInput> InputB;

protected:

	//최대 체력
	UPROPERTY(EditAnywhere, Category="Health", meta = (ClampMin = 0))
	float MaxHealth = 100.0f;

	//현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 0.0f;

	// 수정 필요
	// 적이 IDamage 인터페이스를 사용해 Damage(flaot DamageRate)를 호출 하도록 수정
	/** Damage applied when an actor tagged as an enemy interacts (overlaps) with this machine */
	UPROPERTY(EditAnywhere, Category="Health", meta = (ClampMin = 0))
	float EnemyContactDamage = 10.0f;

	/** Actor tag used to identify enemies that damage this machine on contact */
	UPROPERTY(EditAnywhere, Category="Health")
	FName EnemyActorTag = FName("Enemy");
	//

	/** True once health has reached zero, disables further damage */
	bool bIsDestroyed = false;

public:

	//데미지 입었을 때 BrodCast
	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnCoinPusherDamaged OnDamaged;

	//체력이 0이 되었을 때 Brodcast
	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnCoinPusherDestroyed OnCoinPusherDestroyed;

public:

	//DispenserComponentA/B가 스폰된 직후 InputA/InputB를 각 Dispenser에 연결
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	//데미지 입는 함수
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual void ApplyDamage(float Damage, AActor* DamageCauser);

	//현재 체력 반환
	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	//최대 체력 반환
	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

protected:

	//OvelapDamage 방식은 변경
	UFUNCTION()
	void OnActorOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	// 체력이 0이 되었을 때 호출
	virtual void HandleDestroyed();

	/** Passes control to BP to play effects when the machine is destroyed */
	UFUNCTION(BlueprintImplementableEvent, Category="Health", meta = (DisplayName = "On Destroyed"))
	void BP_OnDestroyed();

public:

	//CoinPusher 구성체 반환
	FORCEINLINE UBoxComponent* GetFloor() const { return Floor; }
	FORCEINLINE UStaticMeshComponent* GetBody() const { return Body; }
	FORCEINLINE UBoxComponent* GetLeftWall() const { return LeftWall; }
	FORCEINLINE UBoxComponent* GetRightWall() const { return RightWall; }
	FORCEINLINE UBoxComponent* GetBackWall() const { return BackWall; }
	FORCEINLINE UChildActorComponent* GetPusherComponent() const { return PusherComponent; }
	FORCEINLINE UChildActorComponent* GetDispenserComponentA() const { return DispenserComponentA; }
	FORCEINLINE UChildActorComponent* GetDispenserComponentB() const { return DispenserComponentB; }
	FORCEINLINE UChildActorComponent* GetDropZoneComponent() const { return DropZoneComponent; }

	//ChildActorComponent가 실제로 스폰한 액터 인스턴스 반환 (BP에서 Child Actor Class를 지정해야 유효함)
	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDispenser* GetDispenserA() const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDispenser* GetDispenserB() const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDropZone* GetDropZone() const;
};
