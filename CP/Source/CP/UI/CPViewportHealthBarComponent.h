// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CPViewportHealthBarComponent.generated.h"

class UCPHealthBarWidget;

/**
 *  UCPHealthBarComponent(월드 스페이스, 캐릭터 머리 위)와 달리 화면 뷰포트에 항상 떠 있는
 *  스크린 스페이스 체력바를 만들고 싶을 때 쓰는 컴포넌트. 아무 Actor(주로 플레이어 캐릭터나
 *  PlayerController)에나 Add Component로 붙이면 BeginPlay에서 WidgetClass를 생성해 화면에 띄운다.
 *  대상 Actor 쪽에서 체력이 바뀔 때 UpdateHealth를 호출(또는 델리게이트를 바인딩)해주면
 *  화면의 체력바가 갱신된다. 같은 UCPHealthBarWidget 상속 WBP를 월드/뷰포트 양쪽에서 재사용 가능
 */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class CP_API UCPViewportHealthBarComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCPViewportHealthBarComponent();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** UCPHealthBarWidget을 상속하는 Widget Blueprint (예: WBP_HealthBar) */
	UPROPERTY(EditAnywhere, Category="Health Bar")
	TSubclassOf<UCPHealthBarWidget> WidgetClass;

	/** 뷰포트에 그려질 때의 Z-Order (높을수록 위에 그려짐) */
	UPROPERTY(EditAnywhere, Category="Health Bar")
	int32 ZOrder = 0;

	/** BeginPlay에서 생성해서 화면에 띄운 위젯 인스턴스 */
	UPROPERTY()
	TObjectPtr<UCPHealthBarWidget> HealthBarWidget;

public:

	/** 체력이 바뀔 때마다 호출해서 화면의 체력바에 반영한다. 대상 Actor의 체력 변경 델리게이트를
	 *  BP에서 Bind Event로 이 함수에 연결해두면 자동으로 갱신된다 */
	UFUNCTION(BlueprintCallable, Category="Health Bar")
	void UpdateHealth(float CurrentHealth, float MaxHealth);
};
