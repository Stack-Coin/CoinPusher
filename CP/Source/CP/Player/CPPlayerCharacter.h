// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Engine/TimerHandle.h"
#include "Player/CPStatInterface.h"
#include "Player/CPStatTypes.h"
#include "CPPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogCPPlayerCharacter, Log, All);

/** Broadcast right after the character's level increases by 1. Systems (e.g. the augment UI) should
 *  react to this event instead of polling the Level stat. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPLevelUp, int32, NewLevel);

/**
 *  Top-down / quarter view action prototype character.
 *  - 8-directional WASD movement relative to the fixed camera
 *  - Mouse cursor directed rectangular (box trace) melee attack
 *  - Directional dash with temporary invincibility
 */
UCLASS(abstract)
class CP_API ACPPlayerCharacter : public ACharacter, public ICPStatInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera above/behind the character in a fixed quarter view angle */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Move Input Action (WASD / Axis2D) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Attack Input Action (Mouse Left Button) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AttackAction;

	/** Dash Input Action (Left Shift) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

protected:

	/** Core player stats (health, experience, attack power, move speed, attack speed, defense, level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	FCPPlayerStats Stats;

	/** Experience required to level up from Level 1 to Level 2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Leveling", meta = (ClampMin = 0))
	float BaseRequiredExperience = 100.0f;

	/** Additional experience required per level above the base requirement. 0 keeps the requirement flat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Leveling", meta = (ClampMin = 0))
	float RequiredExperiencePerLevel = 0.0f;

	/** Width (left-right) of the rectangular attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackWidth = 100.0f;

	/** Length (along the attack direction) of the rectangular attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackLength = 150.0f;

	/** Height of the rectangular attack hitbox */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "cm"))
	float AttackHeight = 100.0f;

	/** Distance the attack hitbox is offset in front of the character */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (Units = "cm"))
	float AttackOffset = 50.0f;

	/** Minimum time that must pass between attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Attack", meta = (ClampMin = 0, Units = "s"))
	float AttackCooldown = 0.5f;

	/** How long the debug attack box is drawn for when bDrawDebugAttackBox is enabled */
	UPROPERTY(EditAnywhere, Category="Stats|Attack", meta = (ClampMin = 0, Units = "s"))
	float AttackDuration = 0.1f;

	/** If true, draws the attack hitbox for debugging */
	UPROPERTY(EditAnywhere, Category="Stats|Attack")
	bool bDrawDebugAttackBox = false;

	/** Distance covered by a single dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "cm"))
	float DashDistance = 600.0f;

	/** Duration of the dash movement, and of the invincibility window */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashDuration = 0.2f;

	/** Minimum time that must pass between dashes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashCooldown = 1.0f;

	/** True while the dash movement is in progress */
	bool bIsDashing = false;

	/** True while the character cannot take damage (driven by the dash) */
	bool bIsInvincible = false;

	/** Game time the last attack was performed */
	float LastAttackTime = -1000.0f;

	/** Game time the last dash was performed */
	float LastDashTime = -1000.0f;

	/** Last non-zero WASD input, used as the dash direction and as the fallback dash direction */
	FVector2D LastMoveInputVector = FVector2D(0.0f, 1.0f);

	/** World direction resolved for the dash currently in progress */
	FVector DashDirection = FVector::ForwardVector;

	/** Timer used to end the dash state after DashDuration */
	FTimerHandle DashDurationTimerHandle;

public:

	/** Constructor */
	ACPPlayerCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for attack input */
	void Attack(const FInputActionValue& Value);

	/** Called for dash input */
	void StartDash(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles attack inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void DoAttack();

	/** Handles dash inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Dash")
	virtual void DoDash();

protected:

	/** Resolves a WASD input vector into a world space direction relative to the fixed camera yaw */
	FVector GetWorldDirectionFromInput(const FVector2D& InputVector) const;

	/** Resolves the current attack direction from the character to the mouse cursor's world location */
	FVector GetAttackDirection() const;

	/** Runs the rectangular box trace attack and applies damage to anything hit */
	void PerformAttack();

	/** Ends the dash movement and invincibility window */
	void EndDash();

	/** Pushes current stat values onto the systems that use them (e.g. MoveSpeed -> MaxWalkSpeed) */
	void ApplyStatsToGameplay();

public:

	/** Overrides the default TakeDamage functionality to ignore damage while invincible */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Broadcast right after the character's level increases by 1 (once per level, even on a multi level up) */
	UPROPERTY(BlueprintAssignable, Category="Stats")
	FOnCPLevelUp OnLevelUp;

	/** Adds experience, handling one or multiple level ups if enough is accumulated at once */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddExperience(float Amount);

	/** Returns the experience required to go from the current level to the next */
	UFUNCTION(BlueprintPure, Category="Stats")
	float GetRequiredExperience() const;

	// ~begin ICPStatInterface

	/** Adds Delta to the current value of the given stat */
	virtual void ModifyStat(ECPStatType StatType, float Delta) override;

	/** Sets the given stat to an absolute value */
	virtual void SetStat(ECPStatType StatType, float NewValue) override;

	/** Returns the current value of the given stat */
	virtual float GetStat(ECPStatType StatType) const override;

	// ~end ICPStatInterface

	/** Returns true while the dash movement is in progress */
	UFUNCTION(BlueprintPure, Category="Dash")
	bool IsDashing() const { return bIsDashing; }

	/** Returns true while the character is invincible */
	UFUNCTION(BlueprintPure, Category="Dash")
	bool IsInvincible() const { return bIsInvincible; }

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
