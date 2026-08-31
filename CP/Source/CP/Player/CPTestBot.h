// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Engine/TimerHandle.h"
#include "CPTestBot.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCPTestBot, Log, All);

/**
 *  Stationary test dummy used to validate the player's basic attack (box trace + damage),
 *  and to test player hit reactions by periodically attacking in front of itself.
 *  No HP or real damage handling is implemented - hits are only logged.
 */
UCLASS(abstract)
class CP_API ACPTestBot : public ACharacter
{
	GENERATED_BODY()

protected:

	/** Time between each of the test bot's attacks */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "s"))
	float AttackInterval = 2.0f;

	/** Width (left-right) of the test bot's attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackWidth = 100.0f;

	/** Length (along the attack direction) of the test bot's attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackLength = 150.0f;

	/** Height of the test bot's attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackHeight = 100.0f;

	/** Distance the attack hitbox is offset in front of the test bot */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (Units = "cm"))
	float AttackOffset = 50.0f;

	/** If true, draws the test bot's attack hitbox for debugging */
	UPROPERTY(EditAnywhere, Category="Stats|Attack")
	bool bDrawDebugAttackBox = false;

	/** Timer handle for the recurring attack */
	FTimerHandle AttackTimerHandle;

public:

	/** Constructor */
	ACPTestBot();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Runs the box trace attack in front of the test bot and logs if the player is hit */
	void PerformBotAttack();

public:

	/** Overrides the default TakeDamage functionality to log incoming hits from the player's attack */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};
