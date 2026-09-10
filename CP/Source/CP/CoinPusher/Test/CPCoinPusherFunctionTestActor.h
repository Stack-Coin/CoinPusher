// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherFunctionTestActor.generated.h"

class ACPCoinPusher;
class ACPRoulette;

/**
 *  레벨에 배치'만' 하면 그 레벨에서 어떤 Pawn을 조작 중이든(플레이어가 다른 Pawn을 possess하고
 *  있어도) 상관없이 넘버패드 입력으로 ACPCoinPusher의 기능들을 테스트할 수 있게 해주는 순수
 *  Actor(Pawn이 아님 - possess 불필요). BeginPlay()에서 EnableInput()으로 첫 번째 로컬
 *  PlayerController의 입력 스택에 이 Actor 자신의 InputComponent를 직접 얹어두기 때문에, 이
 *  PlayerController가 무엇을 possess하고 있는지와 무관하게 항상 넘버패드 입력을 받는다
 *  (ACPCoinPusherItemSpawnTestPawn과 달리 possess/DefaultPawnClass 지정이 전혀 필요 없음).
 *
 *  Num Lock이 꺼져 있으면 물리 키보드의 숫자 패드 키가 다른 키(Insert/End 등)로 인식될 수 있으니
 *  Num Lock을 켜둘 것.
 *
 *  Numpad 1: TargetCoinPusher->ItemSpawn(NormalCoinItemID, 1) - 일반 코인 1개 스폰
 *  Numpad 2: TargetCoinPusher->SpawnBigCoin(BigCoinItemID, 1) - Big 코인 1개 스폰
 *  Numpad 3: TargetCoinPusher->ConvertActive(PassiveConvertItemID, 5) - Passive로 5개 변환
 *  Numpad 4: TargetCoinPusher->HPConvertActive(HPConvertItemID, 5) - HP로 5개 변환
 *  Numpad 5: TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, 5) - Monster로 5개 변환
 *  Numpad 6: TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, 5) - Monster 코인 5개 스폰
 *  Numpad 7: TargetRoulette->Roll() - 룰렛 동작
 *  Numpad 8: TargetCoinPusher->SpawnTower(CoinTowerItemID, 25) - 25층 코인 타워 소환
 *
 *  각 랩퍼 함수(ConvertActive/HPConvertActive/MonsterConvertActive/SpawnBigCoin/SpawnMonsterCoin/
 *  SpawnTower)는 내부적으로 TargetCoinPusher->ItemDataTable에서 해당 ItemID의 CoinType을 검증하므로,
 *  각 ItemID 프로퍼티는 반드시 그에 맞는 CoinType(Big/Passive/HP/Monster/CoinTower)의 행을 가리켜야
 *  실제로 동작한다 (ACPCoinPusher/README.md 참고).
 */
UCLASS()
class CP_API ACPCoinPusherFunctionTestActor : public AActor
{
	GENERATED_BODY()

public:

	ACPCoinPusherFunctionTestActor();

protected:

	/** 테스트 대상 CoinPusher. 직접 지정하거나, 비워두면 BeginPlay에서 레벨에 배치된 아무
	 *  ACPCoinPusher나 자동으로 찾아 사용한다 */
	UPROPERTY(EditInstanceOnly, Category="CoinPusher Test")
	TObjectPtr<ACPCoinPusher> TargetCoinPusher;

	/** Numpad 7이 굴릴 룰렛. 직접 지정하거나, 비워두면 BeginPlay에서 레벨에 배치된 아무
	 *  ACPRoulette나 자동으로 찾아 사용한다 */
	UPROPERTY(EditInstanceOnly, Category="CoinPusher Test")
	TObjectPtr<ACPRoulette> TargetRoulette;

	/** Numpad 1 - ItemSpawn()에 넘길 ItemID (기본은 "Normal Coin" 행의 ID "1C") */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName NormalCoinItemID = TEXT("1C");

	/** Numpad 2 - SpawnBigCoin()에 넘길 ItemID (ItemDataTable에서 CoinType이 Big인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName BigCoinItemID = TEXT("4C");

	/** Numpad 3 - ConvertActive()에 넘길 ItemID (CoinType이 Passive인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName PassiveConvertItemID = TEXT("2C");

	/** Numpad 3 - ConvertActive()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 PassiveConvertCount = 5;

	/** Numpad 4 - HPConvertActive()에 넘길 ItemID (CoinType이 HP인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName HPConvertItemID = TEXT("3C");

	/** Numpad 4 - HPConvertActive()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 HPConvertCount = 5;

	/** Numpad 5 - MonsterConvertActive()에 넘길 ItemID (CoinType이 Monster인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName MonsterConvertItemID = TEXT("5C");

	/** Numpad 5 - MonsterConvertActive()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterConvertCount = 5;

	/** Numpad 6 - SpawnMonsterCoin()에 넘길 ItemID (CoinType이 Monster인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName MonsterCoinItemID = TEXT("5C");

	/** Numpad 6 - SpawnMonsterCoin()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterCoinSpawnCount = 5;

	/** Numpad 8 - SpawnTower()에 넘길 ItemID (CoinType이 CoinTower인 행이어야 함) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName CoinTowerItemID = TEXT("6C");

	/** Numpad 8 - SpawnTower()에 넘길 층 수 */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 CoinTowerFloorCount = 25;

	/** TargetCoinPusher/TargetRoulette를 못 찾았을 때 자동으로 채우고, EnableInput()으로 넘버패드
	 *  바인딩을 등록 */
	virtual void BeginPlay() override;

	void HandleSpawnNormalCoin();
	void HandleSpawnBigCoin();
	void HandleConvertPassive();
	void HandleConvertHP();
	void HandleConvertMonster();
	void HandleSpawnMonsterCoin();
	void HandleRollRoulette();
	void HandleSpawnCoinTower();
};
