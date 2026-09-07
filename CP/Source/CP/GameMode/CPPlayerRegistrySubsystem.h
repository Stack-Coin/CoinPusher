// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "CPPlayerRegistrySubsystem.generated.h"

/**
 *  ACPLobbyGameMode가 로비에서 "어떤 입력 장치가 몇 번째 플레이어(PlayerIndex)로 참가했는지"
 *  배정한 정보를 GameInstance에 붙어서 보관한다. GameInstance(따라서 이 서브시스템)는 OpenLevel로
 *  레벨이 바뀌어도 살아남으므로, 다음(실제 게임플레이) 레벨에서 "이 입력 장치가 1P/2P 중 무엇인지"를
 *  이 서브시스템을 통해 질의할 수 있다. 로비 단계에서는 로컬 플레이어를 새로 만들거나 입력 장치를
 *  리매핑하지 않으며, 순전히 "장치 ↔ PlayerIndex" 정보만 저장한다 - 실제 로컬 플레이어 생성/장치
 *  리매핑은 이 정보를 바탕으로 다음 레벨에서 이루어진다. GameInstanceSubsystem은 Project Settings에
 *  별도로 등록할 필요 없이 자동으로 생성된다.
 */
UCLASS()
class CP_API UCPPlayerRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	/** Join된 순서대로 저장된 입력 장치. 인덱스 = PlayerIndex (0 = P1, 1 = P2, ...). FInputDeviceId가
	 *  UHT 리플렉션 대상이 아니라 UPROPERTY로 노출할 수 없다 */
	TArray<FInputDeviceId> JoinedPlayerDeviceIds;

public:

	/** ACPLobbyGameMode가 플레이어 슬롯을 배정할 때마다 호출. FInputDeviceId가 UHT 리플렉션
	 *  대상이 아니라 BlueprintCallable로 노출할 수 없어 순수 C++ 함수로 둔다 */
	void RegisterJoinedPlayer(FInputDeviceId DeviceId);

	/** 참가 기록을 초기화한다 (새로 로비를 시작할 때 등) */
	UFUNCTION(BlueprintCallable, Category="Player Registry")
	void ResetRegistry();

	/** 입력 장치가 몇 번째로 참가했는지(0=P1, 1=P2, ...) 반환. 못 찾으면 -1. FInputDeviceId가 UHT
	 *  리플렉션 대상이 아니라 BlueprintCallable로 노출할 수 없어 순수 C++ 함수로 둔다 */
	int32 GetPlayerIndexForInputDevice(FInputDeviceId DeviceId) const;

	/** PlayerIndex(0=P1, 1=P2, ...)에 해당하는 입력 장치를 반환. 없으면 무효한(IsValid()==false)
	 *  FInputDeviceId. FInputDeviceId가 UHT 리플렉션 대상이 아니라 BlueprintCallable로 노출할 수
	 *  없어 순수 C++ 함수로 둔다 */
	FInputDeviceId GetInputDeviceForPlayerIndex(int32 PlayerIndex) const;
};
