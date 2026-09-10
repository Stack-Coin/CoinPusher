#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CPWeaponPassiveSkillModule.generated.h"

class ACPWeaponBase;
class ACharacter;
class AController;

struct FCPPassiveSkillActivationContext
{
	ACPWeaponBase* Weapon = nullptr;
	ACharacter* OwnerCharacter = nullptr;
	AController* InstigatorController = nullptr;
	AActor* DamageCauser = nullptr;
	FVector Origin = FVector::ZeroVector;
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Blueprintable)
class CP_API UCPWeaponPassiveSkillModule : public UObject
{
	GENERATED_BODY()

public:

	virtual void Activate(const FCPPassiveSkillActivationContext& Context) PURE_VIRTUAL(UCPWeaponPassiveSkillModule::Activate, );
};
