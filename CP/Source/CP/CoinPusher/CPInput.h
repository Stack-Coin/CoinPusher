// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPInteractable.h"
#include "CPInput.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputInteracted, AActor*, Interactor);

UCLASS(abstract)
class CP_API ACPInput : public AActor, public ICPInteractable
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:
	ACPInput();

public:

	UPROPERTY(BlueprintAssignable, Category="Input")
	FOnInputInteracted OnInteracted;

	//ICPInteractable
	virtual void Interact(AActor* Interactor) override;

protected:


	UFUNCTION(BlueprintImplementableEvent, Category="Input", meta = (DisplayName = "On Interacted"))
	void BP_OnInteracted(AActor* Interactor);

public:

	//컴포넌트 반환
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
};
