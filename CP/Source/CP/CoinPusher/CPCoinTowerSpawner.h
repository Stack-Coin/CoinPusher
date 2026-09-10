// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinTowerSpawner.generated.h"

class USceneComponent;
class ACPCoin;
class ACPPusher;

/**
 *  SpawnTower(N)을 호출하면 한 층에 CoinsPerFloor(5)개씩 원형으로 배치한 코인을 N개 층으로 SpawnActor로
 *  스폰하고, 스폰이 끝나면 TowerRoot(바닥 지점)에서 CoinTowerPosition(목표 지점)까지 UpTime 동안 통째로
 *  상승시킨 뒤 각 코인을 독립 액터로 Detach하는 연출용 액터.
 *
 *  스폰~상승이 진행되는 동안:
 *   - TargetPusher는 멈추고(SetPusherPaused) BackMoveTime 동안 현재 위치에서 BackPosition까지 이동한다.
 *     상승이 끝나면 PusherReturnTime 동안 BackPosition에서 원래 위치(뒤로 밀리기 전 위치)까지 천천히
 *     되돌아오고, 그 이동이 다 끝난 뒤에야 왕복 운동을 재개한다
 *   - 스폰된 각 코인은 ACPCoin::SetTowerLocked(true)로 잠겨 중력/물리충돌(다른 코인 제외)/Launch/
 *     SetCoinType이 모두 막힌다. 상승이 끝나면 SetTowerLocked(false)로 전부 해제하고 DetachFromActor한다
 *
 *  진행 중(SpawnTower 호출부터 코인 Detach + Pusher가 원래 위치로 복귀해 왕복 운동을 재개할 때까지)에는
 *  SpawnTower()를 다시 호출해도 무시된다.
 */
UCLASS(abstract)
class CP_API ACPCoinTowerSpawner : public AActor
{
	GENERATED_BODY()

	//고정 루트 - BackPosition/CoinTowerPosition 등은 전부 이 컴포넌트(=이 액터) 기준 상대 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnerRoot;

	//타워를 구성하는 코인들이 부착되는 기준점. 스폰 시점에는 SpawnerRoot와 같은 위치(상대 위치 0)에
	//있다가, 상승 애니메이션 동안 CoinTowerPosition까지 상대 위치가 이동한다 - 이 컴포넌트가 움직이면
	//부착된 코인들도 한 덩어리로 함께 움직인다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* TowerRoot;

public:

	ACPCoinTowerSpawner();

protected:

	//SpawnActor로 스폰할 Coin 클래스
	UPROPERTY(EditAnywhere, Category="CoinTower")
	TSubclassOf<ACPCoin> CoinClass;

	//층 사이의 수직 간격
	UPROPERTY(EditAnywhere, Category="CoinTower", meta = (ClampMin = 0))
	float FloorHeight = 50.0f;

	//원형으로 배치할 반지름
	UPROPERTY(EditAnywhere, Category="CoinTower", meta = (ClampMin = 0))
	float TowerRadius = 60.0f;

	//타워가 전부 스폰된 뒤 상승해서 도달할 목표 지점 (이 액터 기준 상대 위치)
	UPROPERTY(EditAnywhere, Category="CoinTower")
	FVector CoinTowerPosition = FVector(0.0f, 0.0f, 300.0f);

	//TowerRoot가 스폰 지점(상대 위치 0)에서 CoinTowerPosition까지 상승하는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category="CoinTower", meta = (ClampMin = 0))
	float UpTime = 2.0f;

	//스폰이 이루어지는 동안(스폰 시작 ~ 상승 완료) 동작을 멈추고 BackPosition으로 옮겨둘 Pusher.
	//ACPPusher는 ACPCoinPusher의 ChildActorComponent로 스폰되는 인스턴스라 여기서 직접 편집할 수 없으므로,
	//같은 CoinPusher가 소유한 경우 ACPCoinPusher::PostInitializeComponents()가 SetTargetPusher()로 자동 연결해준다
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="CoinTower")
	TObjectPtr<ACPPusher> TargetPusher;

	//TargetPusher를 스폰~상승 동안 옮겨둘 위치 (TargetPusher 기준 상대 위치)
	UPROPERTY(EditAnywhere, Category="CoinTower")
	FVector BackPosition = FVector(-200.0f, 0.0f, 0.0f);

	//TargetPusher가 현재 위치에서 BackPosition까지 이동하는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category="CoinTower", meta = (ClampMin = 0))
	float BackMoveTime = 0.5f;

	//상승이 끝난 뒤 TargetPusher가 BackPosition에서 원래 위치까지 되돌아오는 데 걸리는 시간(초).
	//이 이동이 다 끝나야 Pusher의 왕복 운동이 재개된다
	UPROPERTY(EditAnywhere, Category="CoinTower", meta = (ClampMin = 0))
	float PusherReturnTime = 1.0f;

	//한 층에 원형으로 배치할 코인 개수
	static constexpr int32 CoinsPerFloor = 5;

	//SpawnTower() 호출부터 코인 Detach + Pusher 재개까지 진행 중이면 true - 이 동안은 SpawnTower()를
	//다시 호출해도 무시된다 (요구사항: 이전 타워가 완전히 끝나기 전까지 새 타워를 스폰할 수 없음)
	bool bIsTowerActive = false;

	//상승 애니메이션이 진행 중이면 true (Tick에서 사용)
	bool bIsRising = false;

	//상승 애니메이션 경과 시간
	float RiseElapsedTime = 0.0f;

	//TargetPusher가 BackPosition으로 이동하는 애니메이션이 진행 중이면 true (Tick에서 사용)
	bool bIsMovingPusherBack = false;

	//Pusher 후퇴 애니메이션 경과 시간
	float PusherMoveElapsedTime = 0.0f;

	//Pusher 후퇴 애니메이션 시작 위치 (World) - SpawnTower() 호출 시점의 TargetPusher 위치
	FVector PusherMoveStartLocation = FVector::ZeroVector;

	//Pusher 후퇴 애니메이션 목표 위치 (World) - BackPosition을 그 시점 TargetPusher 기준으로 변환한 값
	FVector PusherMoveTargetLocation = FVector::ZeroVector;

	//TargetPusher가 BackPosition에서 원래 위치로 되돌아오는 애니메이션이 진행 중이면 true (Tick에서 사용)
	bool bIsReturningPusher = false;

	//Pusher 복귀 애니메이션 경과 시간
	float PusherReturnElapsedTime = 0.0f;

	//Pusher 복귀 애니메이션 시작 위치 (World) - CompleteRise() 시점의 TargetPusher 위치(=BackPosition).
	//목표 위치는 PusherMoveStartLocation(=뒤로 밀리기 전 원래 위치)을 그대로 재사용
	FVector PusherReturnStartLocation = FVector::ZeroVector;

	//이번에 스폰되어 TowerRoot에 부착돼 있는 코인들 - 상승 완료 시 잠금 해제 + Detach 대상
	UPROPERTY()
	TArray<TObjectPtr<ACPCoin>> TowerCoins;

public:

	virtual void Tick(float DeltaTime) override;

	//한 층에 CoinsPerFloor(5)개씩 원형으로 배치한 코인을 N개 층으로 스폰하고, 다 스폰되면 CoinTowerPosition까지
	//상승시킨다. 이전에 스폰한 타워가 아직 진행 중(코인이 Detach되고 Pusher가 재개되기 전)이면 무시된다
	UFUNCTION(BlueprintCallable, Category="CoinTower")
	void SpawnTower(int32 N);

	//현재 SpawnTower()가 진행 중인지(코인이 전부 Detach되고 Pusher가 재개되었는지) 여부
	UFUNCTION(BlueprintPure, Category="CoinTower")
	bool IsTowerActive() const { return bIsTowerActive; }

	//ACPCoinPusher::PostInitializeComponents()가 호출해 TargetPusher를 전달해준다. 직접 배치해서 테스트할
	//때는 BP/코드에서 이 함수로 직접 지정해도 된다
	UFUNCTION(BlueprintCallable, Category="CoinTower")
	void SetTargetPusher(ACPPusher* NewTargetPusher) { TargetPusher = NewTargetPusher; }

	FORCEINLINE USceneComponent* GetTowerRoot() const { return TowerRoot; }

protected:

	//N개 층 * CoinsPerFloor개의 코인을 원형으로 SpawnActor 스폰, TowerRoot에 부착, SetTowerLocked(true)로 잠금
	void SpawnTowerCoins(int32 FloorCount);

	//상승 완료 시 호출 - 모든 TowerCoins의 잠금 해제 + Detach, TargetPusher를 원래 위치로 되돌리는
	//복귀 애니메이션을 시작 (실제 재개는 그 애니메이션이 끝났을 때 Tick에서 처리)
	void CompleteRise();
};
