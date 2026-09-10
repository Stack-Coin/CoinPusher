// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherItem.h"
#include "CPCoinTypes.h"
#include "CPCoin.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPhysicalMaterial;
class UChildActorComponent;
class UPrimitiveComponent;
class ACPCoinThrowArea;
class ACPCoinPusher;

//CoinType별로 지정할 수 있는 Mesh/Material/PhysicsMaterial. 비워두면(nullptr) 해당 항목은 바꾸지 않는다
USTRUCT(BlueprintType)
struct FCPCoinTypeVisual
{
	GENERATED_BODY()

	//지정 시 해당 CoinType으로 전환될 때 Mesh에 SetStaticMesh()로 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	//지정 시 해당 CoinType으로 전환될 때 Mesh 슬롯 0에 SetMaterial()로 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	//지정 시 해당 CoinType으로 전환될 때 Mesh에 SetPhysMaterialOverride()로 적용 (마찰/반발 등 물리 충돌 속성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	TObjectPtr<UPhysicalMaterial> PhysicsMaterial = nullptr;
};

UCLASS(abstract)
class CP_API ACPCoin : public AActor, public ICPCoinPusherItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	//이 코인이 스케일 연출 중 최대 크기에 도달했을 때 활성화하는 CoinThrowArea (컴포넌트를 통한 Has-a).
	//실제 사용할 BP 서브클래스는 이 컴포넌트의 Child Actor Class에 지정 (BP_CPCoinThrowArea 등)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* CoinThrowAreaComponent;

public:

	ACPCoin();

protected:

	//Coin 가치
	UPROPERTY(EditAnywhere, Category="Coin")
	float CoinValue = 1.0f;

	//Coin의 Item ID. ItemDataTable(FItemData) / Dispenser가 ItemID로 구분할 때 쓰는 식별자
	UPROPERTY(EditAnywhere, Category="Coin")
	FName ItemID;

	bool bCollected = false;

	//Launch() 이후 이 시간(초) 동안은 bIsLaunched가 true로 유지되어 추가 Launch() 호출을 무시한다 - 짧은 시간
	//안에 여러 CoinThrowArea 등이 같은 코인을 중복으로 발사해 속도가 비정상적으로 누적되는 것을 막기 위함
	UPROPERTY(EditAnywhere, Category="Coin", meta = (ClampMin = 0))
	float LaunchCooldown = 1.0f;

	//Launch()로 날아가고 있는 중이면 true. LaunchCooldown 경과 후 자동으로 다시 false로 돌아와 재발사 가능해진다
	bool bIsLaunched = false;

	//bIsLaunched를 다시 false로 되돌리는 타이머 핸들
	FTimerHandle LaunchCooldownTimerHandle;

	//Coin의 종류. SetCoinType()으로 변경하면 타입별 연출/행동이 트리거된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coin", meta = (AllowPrivateAccess = "true"))
	ECPCoinType CoinType = ECPCoinType::Normal;

protected:

	//Passive/HP 코인으로 전환될 때 줄어드는 최소 스케일 배율 (전환 시점의 원본 스케일 대비)
	UPROPERTY(EditAnywhere, Category="Coin|ScaleAnim", meta = (ClampMin = 0))
	float ScaleAnimMinScale = 0.5f;

	//Passive/HP 코인으로 전환될 때 늘어나는 최대 스케일 배율 (전환 시점의 원본 스케일 대비)
	UPROPERTY(EditAnywhere, Category="Coin|ScaleAnim", meta = (ClampMin = 0))
	float ScaleAnimMaxScale = 2.0f;

	//축소 → 확대 → 원상복구 연출이 진행되는 속도 (스케일 배율/초)
	UPROPERTY(EditAnywhere, Category="Coin|ScaleAnim", meta = (ClampMin = 0))
	float ScaleAnimSpeed = 2.0f;

	//현재 진행 중인 스케일 연출 단계 (Passive/HP 공용)
	enum class EScaleAnimPhase : uint8
	{
		None,
		ShrinkingToMin,
		GrowingToMax,
		ReturningToOriginal
	};

	EScaleAnimPhase ScaleAnimPhase = EScaleAnimPhase::None;

	//SetCoinType(Passive/HP) 호출 시점의 원본 스케일. 연출이 끝나면 이 값으로 복귀
	FVector ScaleAnimOriginalScale = FVector::OneVector;

	//원본 스케일 대비 현재 배율 (연출 진행 상태)
	float ScaleAnimCurrentMultiplier = 1.0f;

	//CoinType별로 전환 시 적용할 Mesh/Material. 항목이 없거나 필드가 비어있으면 해당 부분은 바꾸지 않는다
	UPROPERTY(EditAnywhere, Category="Coin", meta = (AllowPrivateAccess = "true"))
	TMap<ECPCoinType, FCPCoinTypeVisual> CoinTypeVisuals;

	//CoinThrowArea를 활성화할 때, 이 코인의 위치 기준 월드 X방향으로 얼마나 떨어뜨려 배치할지
	UPROPERTY(EditAnywhere, Category="Coin", meta = (AllowPrivateAccess = "true"))
	float CoinThrowAreaOffsetX = 150.0f;

	//Big 코인으로 전환될 때 곱해지는 스케일 배율 (전환 시점의 원본 스케일 대비). Passive/HP와 달리 원상복구되지 않는다
	UPROPERTY(EditAnywhere, Category="Coin|Big", meta = (ClampMin = 0))
	float BigScaleMultiplier = 3.0f;

	//스폰(BeginPlay) 후 이 시간(초)이 지나야 Big 코인의 WaveThrow 트리거가 활성화된다 - 스폰 직후 SpawnPoint/
	//Dispenser 등과의 초기 접촉으로 곧바로 오발동하는 것을 방지하기 위한 유예 시간
	UPROPERTY(EditAnywhere, Category="Coin|Big", meta = (ClampMin = 0))
	float BigWaveThrowArmDelay = 0.05f;

	//BigWaveThrowArmDelay가 지나 Big 코인의 WaveThrow 트리거가 활성화되면 true
	bool bBigWaveThrowArmed = false;

	//bBigWaveThrowArmed를 true로 바꾸는 타이머 핸들
	FTimerHandle BigWaveThrowArmTimerHandle;

	//Big 코인이 무엇과든(어떤 충돌이든) 처음 부딪혔을 때 true로 설정 - ActiveWaveThrow()가 중복 호출되지
	//않도록 한 번만 트리거되게 막는 가드
	bool bHasTriggeredBigWaveThrow = false;

	//이 코인을 스폰한 CoinPusher (Big 코인이 부딪혔을 때 ActiveWaveThrow()를 호출할 대상).
	//ChildActorComponent로 스폰된 Dispenser가 낳은 Coin은 Owner 체인(GetOwner())이 신뢰할 수 없어서
	//(UChildActorComponent가 스폰한 액터에 Owner를 설정해주지 않음) Owner 체인 탐색 대신, 스폰한
	//쪽(ACPCoinPusher::SpawnBigCoin())이 SetOwningCoinPusher()로 직접 알려준다
	UPROPERTY()
	TObjectPtr<ACPCoinPusher> OwningCoinPusher = nullptr;

	//CoinTowerSpawner 등이 SetTowerLocked(true)로 잠갔으면 true - 이 상태에서는 Launch()와 SetCoinType()이
	//모두 조기 반환되어 아무 효과가 없다 (타워가 상승하는 동안 발사되거나 타입이 바뀌는 것을 방지)
	bool bIsTowerLocked = false;

public:

	//Coin 가치 반환
	UFUNCTION(BlueprintPure, Category="Coin")
	float GetCoinValue() const { return CoinValue; }

	//Coin의 Item ID 반환
	UFUNCTION(BlueprintPure, Category="Coin")
	FName GetItemID() const { return ItemID; }

	//현재 Coin 종류 반환
	UFUNCTION(BlueprintPure, Category="Coin")
	ECPCoinType GetCoinType() const { return CoinType; }

	//Coin 종류를 변경. 실제로 값이 바뀔 때만 타입별 연출/행동을 트리거하고 BP_OnCoinTypeChanged를 호출
	UFUNCTION(BlueprintCallable, Category="Coin")
	void SetCoinType(ECPCoinType NewType);

	//발사 실행
	UFUNCTION(BlueprintCallable, Category="Coin")
	void Launch(const FVector& LaunchVelocity);

	//DropZone에 떨어졌을 때 호출
	UFUNCTION(BlueprintCallable, Category="Coin")
	void Collect();

	// ~begin ICPCoinPusherItem
	//DropZone이 떨어진 것을 알릴 때 Collect() 호출
	virtual void OnDroppedInZone(ACPDropZone* DropZone) override;
	// ~end ICPCoinPusherItem

	virtual void Tick(float DeltaTime) override;

protected:

	//BigWaveThrowArmDelay 경과 후 bBigWaveThrowArmed가 true가 되도록 타이머를 예약
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Coin", meta = (DisplayName = "On Collected"))
	void BP_OnCollected();

	//CoinType이 실제로 바뀔 때마다 호출 (SetCoinType 참고). 추가 타입별 연출은 BP에서 이 이벤트로 확장 가능
	UFUNCTION(BlueprintImplementableEvent, Category="Coin", meta = (DisplayName = "On Coin Type Changed"))
	void BP_OnCoinTypeChanged(ECPCoinType OldType, ECPCoinType NewType);

	//스케일 연출(축소→확대→원상복구) 시작 - 현재 스케일을 원본으로 기록하고 Tick을 켠다 (Passive/HP 공용)
	void StartScaleAnimation();

	//진행 중이던 스케일 연출을 즉시 취소하고 원본 스케일로 되돌림 (다른 타입으로 바뀌었을 때 호출)
	void CancelScaleAnimation();

	//CoinTypeVisuals에서 NewType에 해당하는 Mesh/Material을 찾아 Mesh 컴포넌트에 적용
	void ApplyCoinTypeVisual(ECPCoinType NewType);

	//CoinThrowAreaComponent가 실제로 스폰한 CoinThrowArea가 있으면 이 코인 위치 기준
	//월드 X방향(CoinThrowAreaOffsetX)으로 옮긴 뒤 ActiveThrow() 호출
	void ActivateCoinThrowArea();

	//LaunchCooldownTimerHandle 만료 시 호출되어 bIsLaunched를 다시 false로 되돌림 (재발사 가능 상태로 복귀)
	void ClearLaunchedState();

	//BigWaveThrowArmTimerHandle 만료 시 호출되어 bBigWaveThrowArmed를 true로 설정
	void ArmBigWaveThrow();

	//Mesh->OnComponentHit에 바인딩 - Big 코인이 무엇과든 처음 부딪히면(단, bBigWaveThrowArmed가 true인 이후)
	//OwningCoinPusher의 ActiveWaveThrow()를 1회 실행시킨다
	UFUNCTION()
	void HandleMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:

	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
	FORCEINLINE UChildActorComponent* GetCoinThrowAreaComponent() const { return CoinThrowAreaComponent; }

	//CoinThrowAreaComponent가 실제로 스폰한 액터 인스턴스 반환 (BP에서 Child Actor Class를 지정해야 유효함)
	UFUNCTION(BlueprintPure, Category="Coin")
	ACPCoinThrowArea* GetCoinThrowArea() const;

	//이 코인을 스폰한 CoinPusher를 설정 (Big 코인이 부딪혔을 때 ActiveWaveThrow()를 호출할 대상).
	//현재는 ACPCoinPusher::SpawnBigCoin()이 스폰 직후 호출
	UFUNCTION(BlueprintCallable, Category="Coin")
	void SetOwningCoinPusher(ACPCoinPusher* NewOwningCoinPusher) { OwningCoinPusher = NewOwningCoinPusher; }

	//true로 설정하면 이 코인을 물리적으로 격리한다: SimulatePhysics를 꺼서(Kinematic) 중력의 영향을 받지
	//않게 하고(Floor/Wall/Pusher 등 스윕 없는 이동으로는 막히지도 않음), 콜리전 프로파일은 그대로
	//BlockAllDynamic이라 다른 Simulate 중인 코인과는 여전히 부딪혀 밀어내는 상호작용이 발생한다.
	//또한 Launch()/SetCoinType()이 아무 효과 없이 무시되게 만든다. false로 되돌리면 SimulatePhysics를
	//다시 켜서 중력/물리충돌/Launch/SetCoinType이 전부 정상으로 복구된다. CoinTowerSpawner가 사용
	UFUNCTION(BlueprintCallable, Category="Coin")
	void SetTowerLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category="Coin")
	bool IsTowerLocked() const { return bIsTowerLocked; }
};
