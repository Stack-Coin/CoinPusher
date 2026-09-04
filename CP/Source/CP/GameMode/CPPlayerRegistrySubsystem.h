// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "CPPlayerRegistrySubsystem.generated.h"

class APlayerController;

/**
 *  ACPLobbyGameMode가 로비에서 "몇 번째 플레이어(PlayerIndex)가 어떤 PlatformUserId인지"
 *  배정한 정보를 GameInstance에 붙어서 보관한다. GameInstance(따라서 이 서브시스템과 로컬
 *  플레이어들)는 OpenLevel로 레벨이 바뀌어도 살아남으므로, 다음(실제 게임플레이) 레벨에서
 *  "이 입력 장치 / 이 PlayerIndex가 조종하는 Actor가 무엇인지"를 이 서브시스템을 통해
 *  질의할 수 있다. GameInstanceSubsystem은 Project Settings에 별도로 등록할 필요 없이
 *  자동으로 생성된다.
 */
UCLASS()
class CP_API UCPPlayerRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	/** Join된 순서대로 저장된 PlatformUserId. 인덱스 = PlayerIndex (0 = P1, 1 = P2, ...) */
	UPROPERTY()
	TArray<FPlatformUserId> JoinedPlayerOrder;

public:

	/** ACPLobbyGameMode가 플레이어 슬롯을 배정할 때마다 호출 */
	void RegisterJoinedPlayer(FPlatformUserId PlatformUserId);

	/** 참가 기록을 초기화한다 (새로 로비를 시작할 때 등) */
	UFUNCTION(BlueprintCallable, Category="Player Registry")
	void ResetRegistry();

	/** PlayerIndex(0=P1, 1=P2, ...)에 해당하는 PlatformUserId를 반환. 없으면 PLATFORMUSERID_NONE */
	UFUNCTION(BlueprintCallable, Category="Player Registry")
	FPlatformUserId GetPlatformUserIdForPlayerIndex(int32 PlayerIndex) const;

	/** PlayerController가 몇 번째로 참가한 플레이어인지 반환 (0=P1, 1=P2, ...). 못 찾으면 -1 */
	UFUNCTION(BlueprintCallable, Category="Player Registry")
	int32 GetPlayerIndexForController(APlayerController* PlayerController) const;

	/** PlayerIndex(0=P1, 1=P2, ...)가 현재 조종하고 있는 Actor(Pawn)를 반환.
	 *  아직 스폰 전이거나 없으면 nullptr */
	UFUNCTION(BlueprintCallable, Category="Player Registry")
	AActor* GetControlledActorForPlayerIndex(int32 PlayerIndex) const;

	/** 특정 입력 장치(DeviceId)가 현재 조종하고 있는 Actor(Pawn)를 반환. FInputDeviceId가 UHT
	 *  리플렉션 대상이 아니라 BlueprintCallable로 노출할 수 없어 순수 C++ 함수로 둔다 */
	AActor* GetControlledActorForInputDevice(FInputDeviceId DeviceId) const;
};
