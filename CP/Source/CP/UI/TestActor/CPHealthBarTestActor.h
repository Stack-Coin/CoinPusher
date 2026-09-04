// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "CPHealthBarTestActor.generated.h"

class UCPViewportHealthBarComponent;

/**
 *  UCPViewportHealthBarComponent 동작을 확인하기 위한 테스트용 Actor.
 *  Level에 그냥 배치하고 재생하면, 자동으로 체력이 깎였다가 0이 되면 다시 가득 채워지는 것을
 *  반복하면서 화면(뷰포트)의 체력바가 그에 맞춰 갱신되는지 눈으로 확인할 수 있다.
 *  BP 서브클래스 없이 이 클래스 자체로 바로 배치 가능 - 배치 후 ViewportHealthBar 컴포넌트의
 *  WidgetClass에 UCPHealthBarWidget을 상속하는 WBP(예: WBP_HealthBar)를 지정해주면 된다.
 */
UCLASS()
class CP_API ACPHealthBarTestActor : public AActor
{
	GENERATED_BODY()

public:

	ACPHealthBarTestActor();

protected:

	/** 화면(뷰포트)에 체력바를 띄우는 컴포넌트. Details 패널에서 Widget Class를 지정해야 실제로 뜬다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCPViewportHealthBarComponent> ViewportHealthBar;

	/** 테스트용 최대 체력 */
	UPROPERTY(EditAnywhere, Category="Health Bar Test", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;

	/** 테스트용 현재 체력 (에디터에서 직접 바꿔가며 확인할 수도 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health Bar Test")
	float CurrentHealth = 100.0f;

	/** DamageInterval마다 깎을 체력량 */
	UPROPERTY(EditAnywhere, Category="Health Bar Test", meta = (ClampMin = 0))
	float DamageAmount = 10.0f;

	/** 자동 데미지 틱 간격(초). 0 이하로 두면 자동 데미지 없이 CurrentHealth를 직접
	 *  바꾸거나 TakeTestDamage/ResetTestHealth를 수동으로 호출해서 테스트 가능 */
	UPROPERTY(EditAnywhere, Category="Health Bar Test", meta = (ClampMin = 0))
	float DamageInterval = 1.0f;

	FTimerHandle DamageTimerHandle;

	virtual void BeginPlay() override;

	/** 자동 데미지 타이머에 걸리는 함수 - CurrentHealth를 DamageAmount만큼 깎고, 0 이하가 되면
	 *  ResetTestHealth로 다시 가득 채운다. 매번 ViewportHealthBar->UpdateHealth를 호출해 반영 */
	void HandleDamageTick();

public:

	/** CurrentHealth를 Amount만큼 깎고(0 미만으로는 안 내려감) 체력바에 반영한다 */
	UFUNCTION(BlueprintCallable, Category="Health Bar Test")
	void TakeTestDamage(float Amount);

	/** CurrentHealth를 MaxHealth로 되돌리고 체력바에 반영한다 */
	UFUNCTION(BlueprintCallable, Category="Health Bar Test")
	void ResetTestHealth();
};
