// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinTypes.h"
#include "CPDropZone.h"
#include "CPCoinPusher.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UChildActorComponent;
class USpringArmComponent;
class UCPCoinPusherViewCaptureComponent;
class ACPDispenser;
class ACPPassiveCoinConvertArea;
class ACPCoinThrowArea;
class ACPCoinTowerSpawner;
class ACPPusher;
class ACPNexus;
class ACPRoulette;

/**Broadcast whenever this CoinPusher's health changes as a result of damage */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinPusherDamaged, float, Damage, AActor*, DamageCauser);

/** Broadcast when this CoinPusher's health reaches zero */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoinPusherDestroyed);

UCLASS(abstract)
class CP_API ACPCoinPusher : public AActor
{
	GENERATED_BODY()

	//코인이 놓이는 바닥 (RootComponent, 실제 충돌 담당)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Floor;

	//Body Mesh (콜리전 없음 - 순수 비주얼, Floor에 부착)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Body;

	//왼쪽 벽
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWall;

	//오른쪽 벽
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWall;

	//뒷 벽
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BackWall;

	//앞 벽
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* FrontWall;

	//추가 비주얼 메시 (콜리전 없음, Floor에 부착). 용도는 BP에서 자유롭게 확장
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ExtraBoxMesh;

	//Pusher ActorComponent (컴포넌트를 통한 Has-a)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* PusherComponent;

	//Dispenser ActorComponent (컴포넌트를 통한 Has-a) - 앞으로 코인을 발사하는 Input 연동 Dispenser 2개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DispenserComponentA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DispenserComponentB;

	//천장에서 물건을 뿌리는 Dispenser (컴포넌트를 통한 Has-a) - 5개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UChildActorComponent>> CeilingDispenserComponents;

	//DropZone ActorComponent (컴포넌트를 통한 Has-a)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* DropZoneComponent;

	//PassiveCoinConvertArea ActorComponent (컴포넌트를 통한 Has-a) - 영역 안 코인을 Passive로 전환시키는 트리거 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* PassiveCoinConvertAreaComponent;

	//MonsterCoinConvertArea ActorComponent (컴포넌트를 통한 Has-a) - PassiveCoinConvertAreaComponent와는
	//별개의 인스턴스로, 영역 안 Normal 코인을 Monster로 전환시키는 전용 트리거 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* MonsterCoinConvertAreaComponent;

	//CoinThrowArea ActorComponent (컴포넌트를 통한 Has-a) - ActiveWaveThrow()가 순차적으로 활성화시키는 던지기 볼륨 5개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UChildActorComponent>> CoinThrowAreaComponents;

	//CoinTowerSpawner ActorComponent (컴포넌트를 통한 Has-a) - SpawnTower()로 원형 코인 타워를 스폰/상승시키는 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* CoinTowerSpawnerComponent;

	//ViewCaptureComponent를 붙여서 위치/각도를 잡아주는 SpringArm. ArmLength/각도를 BP나 디테일
	//패널에서 바로 조정할 수 있고, bDoCollisionTest를 켜면 벽 등에 캡처 카메라가 파묻히는 것도 방지 가능
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* ViewCaptureBoom;

	//이 CoinPusher를 비추는 SceneCaptureComponent2D. ViewCaptureBoom 끝(소켓)에 붙어서 동작하며,
	//자체 RenderTarget을 만든다. 화면에 실제로 띄우는 건 PlayerController가 이 RenderTarget을
	//UCPCoinPusherCaptureWidget에 연결해줘야 한다 (UCPCoinPusherViewportClient가 Player 카메라를
	//오른쪽으로 축소해서 왼쪽 자리를 비워둔다). 위치/회전은 ViewCaptureBoom을 통해 BP에서 조정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPCoinPusherViewCaptureComponent* ViewCaptureComponent;

public:
	ACPCoinPusher();

protected:

	//각 Dispenser를 작동시키는 Input 액터. 레벨에 배치한 ACPInput을 여기서 연결하면
	//PostInitializeComponents에서 자동으로 해당 Dispenser의 LinkedInput으로 설정된다
	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPNexus> InputA;

	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPNexus> InputB;

	//DropZone에 아이템이 떨어졌을 때 같은 아이템의 재생성을 맡을 Dispenser.
	// 외부에 Dispenser를선택
	//PostInitializeComponents에서 자동으로 DropZone에 전달된다
	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPDispenser> ItemRespawnDispenser;

	//게임 시작 시 천장 Dispenser 하나당 드롭할 코인 개수
	UPROPERTY(EditAnywhere, Category="CoinPusher", meta = (ClampMin = 0))
	int32 InitialCoinDropCount = 10;

	//SpawnBigCoin()이 스폰할 코인의 ItemID (천장 Dispenser의 ItemDataTable에 Big 코인 행이 등록돼 있어야 함)
	UPROPERTY(EditAnywhere, Category="CoinPusher")
	FName BigCoinItemID = TEXT("4C");

	//SpawnMonsterCoin()이 스폰할 코인의 ItemID (천장 Dispenser의 ItemDataTable에 Monster 코인 행이 등록돼 있어야 함)
	UPROPERTY(EditAnywhere, Category="CoinPusher")
	FName MonsterCoinItemID = TEXT("5C");

	//이 CoinPusher와 연동할 Roulette. 레벨에서 직접 연결해야 하며(InputA/InputB와 동일한 방식의 수동
	//연결), BeginPlay에서 자동으로 이 Roulette의 OnPickedUp에 HandleRoulettePickedUp()을 등록해
	//룰렛에서 아이템이 뽑힐 때마다(bRouletteToCoinPusher인 경우에만) 천장 Dispenser에서 그 아이템이 나오게 한다
	UPROPERTY(EditInstanceOnly, Category="CoinPusher")
	TObjectPtr<ACPRoulette> LinkedRoulette;

	//게임 시작 후 FrontWall을 제거하기까지 대기하는 시간(초)
	UPROPERTY(EditAnywhere, Category="CoinPusher", meta = (ClampMin = 0))
	float FrontWallRemovalDelay = 3.0f;

	//FrontWall 제거 타이머 핸들
	FTimerHandle FrontWallRemovalTimerHandle;

	//ActiveWaveThrow()가 CoinThrowAreaComponents를 순차적으로 활성화할 때, 각 CoinThrowArea 사이의 대기 시간(초)
	UPROPERTY(EditAnywhere, Category="CoinPusher", meta = (ClampMin = 0))
	float WaveThrowInterval = 0.15f;

	//ActiveWaveThrow() 진행 중 다음에 활성화할 CoinThrowAreaComponents의 인덱스
	int32 WaveThrowIndex = 0;

	//ActiveWaveThrow() 순차 실행 타이머 핸들
	FTimerHandle WaveThrowTimerHandle;

protected:

	//최대 체력
	UPROPERTY(EditAnywhere, Category="Health", meta = (ClampMin = 0))
	float MaxHealth = 100.0f;

	//현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 0.0f;

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

	//DispenserComponentA/B가 스폰된 직후 InputA/InputB를 각 Dispenser에 연결하고,
	//DropZone에 ItemRespawnDispenser를 전달하고, CoinTowerSpawner에 Pusher를 전달
	virtual void PostInitializeComponents() override;

	//천장 Dispenser들이 게임 시작 시 코인을 드롭
	virtual void BeginPlay() override;

	//UGameplayStatics::ApplyDamage(및 ApplyPointDamage/ApplyRadialDamage)로 들어오는 데미지 처리
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

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

	// 체력이 0이 되었을 때 호출
	virtual void HandleDestroyed();

	//FrontWallRemovalDelay 경과 후 호출되어 FrontWall을 비활성화
	void RemoveFrontWall();

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
	FORCEINLINE UBoxComponent* GetFrontWall() const { return FrontWall; }
	FORCEINLINE UStaticMeshComponent* GetExtraBoxMesh() const { return ExtraBoxMesh; }
	FORCEINLINE UChildActorComponent* GetPusherComponent() const { return PusherComponent; }
	FORCEINLINE UChildActorComponent* GetCoinTowerSpawnerComponent() const { return CoinTowerSpawnerComponent; }
	FORCEINLINE UChildActorComponent* GetDispenserComponentA() const { return DispenserComponentA; }
	FORCEINLINE UChildActorComponent* GetDispenserComponentB() const { return DispenserComponentB; }
	FORCEINLINE UChildActorComponent* GetDropZoneComponent() const { return DropZoneComponent; }
	FORCEINLINE UChildActorComponent* GetPassiveCoinConvertAreaComponent() const { return PassiveCoinConvertAreaComponent; }
	FORCEINLINE UChildActorComponent* GetMonsterCoinConvertAreaComponent() const { return MonsterCoinConvertAreaComponent; }
	FORCEINLINE const TArray<TObjectPtr<UChildActorComponent>>& GetCeilingDispenserComponents() const { return CeilingDispenserComponents; }
	FORCEINLINE const TArray<TObjectPtr<UChildActorComponent>>& GetCoinThrowAreaComponents() const { return CoinThrowAreaComponents; }
	FORCEINLINE USpringArmComponent* GetViewCaptureBoom() const { return ViewCaptureBoom; }
	FORCEINLINE UCPCoinPusherViewCaptureComponent* GetViewCaptureComponent() const { return ViewCaptureComponent; }

	//ChildActorComponent가 실제로 스폰한 액터 인스턴스 반환 (BP에서 Child Actor Class를 지정해야 유효함)
	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPPusher* GetPusher() const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDispenser* GetDispenserA() const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDispenser* GetDispenserB() const;

	//Index번째 천장 Dispenser가 실제로 스폰한 액터 인스턴스 반환
	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDispenser* GetCeilingDispenser(int32 Index) const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPDropZone* GetDropZone() const;

	//GetDropZone()->OnDropped에 대한 포인터 반환 (DropZone이 아직 스폰되지 않았으면 nullptr) -
	//DropZone을 직접 거치지 않고 CoinPusher만으로 바로 바인딩하고 싶은 C++ 코드를 위한 편의 함수.
	//FOnCPDropZoneDropped는 델리게이트 타입이라 반환값으로 BP에 노출할 수 없어 BlueprintCallable로
	//두지 않음 - BP에서 바인딩하려면 GetDropZone()으로 얻은 액터의 OnDropped 핀에 직접 Bind Event
	FOnCPDropZoneDropped* GetDropZoneDroppedDelegate() const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPPassiveCoinConvertArea* GetPassiveCoinConvertArea() const;

	//MonsterCoinConvertAreaComponent가 실제로 스폰한 액터 인스턴스 반환
	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPPassiveCoinConvertArea* GetMonsterCoinConvertArea() const;

	//Index번째 CoinThrowArea가 실제로 스폰한 액터 인스턴스 반환
	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPCoinThrowArea* GetCoinThrowArea(int32 Index) const;

	UFUNCTION(BlueprintPure, Category="CoinPusher")
	ACPCoinTowerSpawner* GetCoinTowerSpawner() const;

	//Roulette 등 외부에서 특정 ItemID를 SpawnCount만큼 생성하고 싶을 때 호출. 천장 Dispenser
	//(CeilingDispenserComponents) 중 하나를 랜덤하게 골라 그 Dispenser의 DispenseItemByID()로
	//위임한다 - 코인 여부/CoinType 적용은 Dispenser가 ItemDataTable을 조회해 알아서 처리하므로
	//여기서는 신경 쓰지 않는다
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	void ItemSpawn(FName ItemID, int32 SpawnCount);

	//LinkedRoulette::OnPickedUp에 자동으로 등록되는 핸들러. ItemID로 ItemDataTable을 조회해
	//FItemData::bRouletteToCoinPusher가 true인 경우에만 ItemSpawn(ItemID, SpawnCount)을 호출한다 -
	//룰렛에서 당첨된 아이템이라도 실제로 CoinPusher에 스폰되어야 하는지는 데이터 테이블 설정에 따른다
	UFUNCTION()
	void HandleRoulettePickedUp(FName ItemID, int32 SpawnCount);

	//천장 Dispenser 중 하나를 랜덤하게 골라(매번 다시 고름) BigCoinItemID로 지정된 코인을 Count개
	//스폰하고 각각 CoinType을 Big으로 전환한다
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	void SpawnBigCoin(int32 Count = 1);

	//천장 Dispenser 중 하나를 랜덤하게 골라(매번 다시 고름) MonsterCoinItemID로 지정된 코인을 Num개
	//스폰하고 각각 CoinType을 Monster로 전환한다
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	void SpawnMonsterCoin(int32 Num = 1);

	//CoinThrowAreaComponents 5개를 WaveThrowInterval 간격으로 순차적으로 ActiveThrow() 시킨다
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	void ActiveWaveThrow();

protected:

	//ActiveWaveThrow() 진행 중 WaveThrowTimerHandle에 의해 반복 호출됨 - 다음 CoinThrowArea를 활성화하고 인덱스를 진행
	void HandleWaveThrowTick();

	//CeilingDispenserComponents 중 실제로 스폰된 ACPDispenser들 가운데 하나를 랜덤하게 골라 반환 (없으면 nullptr).
	//ItemSpawn()/SpawnBigCoin()/SpawnMonsterCoin()이 공유하는 선택 로직
	ACPDispenser* PickRandomValidCeilingDispenser() const;
};
