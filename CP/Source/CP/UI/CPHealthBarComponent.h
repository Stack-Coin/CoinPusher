// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "CPHealthBarComponent.generated.h"

class UCPHealthBarWidget;

/**
 *  체력이 바뀔 때마다 브로드캐스트하는 델리게이트 시그니처.
 *  체력을 가진 Actor(적, 플레이어 캐릭터, CoinPusher 등)가 이 시그니처와 동일한
 *  BlueprintAssignable 델리게이트를 하나 선언해두고 체력이 바뀔 때마다 Broadcast하면,
 *  그 델리게이트를 이 컴포넌트의 UpdateHealth 함수에 연결(BP의 Bind Event, 또는 C++의 AddDynamic)
 *  하는 것만으로 체력바가 자동으로 갱신된다. 인터페이스 구현이 필요 없어 BP로만 만든 Actor도
 *  똑같이 사용할 수 있다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCPHealthChanged, float, CurrentHealth, float, MaxHealth);

/**
 *  적, 플레이어 캐릭터, CoinPusher 등 어떤 Actor의 BP에나 Add Component로 그냥 붙일 수 있는
 *  월드 스페이스 체력바 컴포넌트. WidgetClass에는 UCPHealthBarWidget을 상속하는
 *  Widget Blueprint(WBP_HealthBar 등)를 지정한다.
 *  대상 Actor 쪽에서 체력이 바뀔 때 UpdateHealth를 호출(또는 FCPHealthChanged 델리게이트를
 *  UpdateHealth에 바인딩)해주기만 하면 위젯에 반영된다.
 */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class CP_API UCPHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCPHealthBarComponent();

protected:

	virtual void BeginPlay() override;

	/** Cached UserWidgetObject, cast once in BeginPlay */
	UPROPERTY()
	TObjectPtr<UCPHealthBarWidget> HealthBarWidget;

public:

	/** 체력이 바뀔 때마다 호출해서 체력바에 반영한다. 대상 Actor의 체력 변경 델리게이트를
	 *  BP에서 Bind Event로 이 함수에 연결해두면, 이후로는 대상 Actor가 체력을 바꾸고
	 *  델리게이트를 Broadcast하기만 해도 체력바가 자동으로 갱신된다 */
	UFUNCTION(BlueprintCallable, Category="Health Bar")
	void UpdateHealth(float CurrentHealth, float MaxHealth);
};
