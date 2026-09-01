// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Engine/TimerHandle.h"
#include "Player/CPStatInterface.h"
#include "Player/CPStatTypes.h"
#include "Player/CPInteractable.h"
#include "Player/CPInteractor.h"
#include "Player/CPCoinWallet.h"
#include "Player/CPItemInventory.h"
#include "Player/CPItemTypes.h"
#include "CPPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UCPWeaponManagerComponent;
class ACPWeaponBase;

DECLARE_LOG_CATEGORY_EXTERN(LogCPPlayerCharacter, Log, All);

/** Broadcast right after the character's level increases by 1. Systems (e.g. the augment UI) should
 *  react to this event instead of polling the Level stat. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPLevelUp, int32, NewLevel);

/** Broadcast right after an item is added to the player's inventory. UI (e.g. the item toast)
 *  should react to this event instead of the item actor touching any UI directly. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPItemAcquired, FCPItemData, AcquiredItem);

/**
 *  Top-down / quarter view action prototype character.
 *  - 8-directional WASD movement relative to the fixed camera
 *  - Mouse cursor directed rectangular (box trace) melee attack
 *  - Directional dash with temporary invincibility
 */
UCLASS(abstract)
class CP_API ACPPlayerCharacter : public ACharacter, public ICPStatInterface, public ICPInteractor, public ICPCoinWallet, public ICPItemInventory
{
	GENERATED_BODY()

	/** Camera boom positioning the camera above/behind the character in a fixed quarter view angle */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Owns weapon equip/swap/unequip and the currently held weapon. See Weapon/CPWeaponManagerComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPWeaponManagerComponent* WeaponManager;

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

	/** Interact Input Action (E key) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

protected:

	/** Core player stats (health, experience, attack power, move speed, attack speed, defense, level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	FCPPlayerStats Stats;

	/** Min/Max bounds for Health. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange HealthRange = FCPStatRange(0.0f, 100.0f);

	/** Min/Max bounds for Experience. SetStat/ModifyStat/AddExperience clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange ExperienceRange = FCPStatRange(0.0f, 999999.0f);

	/** Min/Max bounds for AttackPower. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange AttackPowerRange = FCPStatRange(0.0f, 999.0f);

	/** Min/Max bounds for MoveSpeed. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange MoveSpeedRange = FCPStatRange(0.0f, 1200.0f);

	/** Min/Max bounds for AttackSpeed. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange AttackSpeedRange = FCPStatRange(0.1f, 5.0f);

	/** Min/Max bounds for Defense. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange DefenseRange = FCPStatRange(0.0f, 999.0f);

	/** Min/Max bounds for Level. SetStat/ModifyStat/AddExperience clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange LevelRange = FCPStatRange(1.0f, 99.0f);

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

	/** Current coin balance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coin", meta = (ClampMin = 0))
	int32 CoinAmount = 0;

	/** Items the player has picked up. Never modify directly - go through AddOwnedItem/ICPItemInventory */
	UPROPERTY(BlueprintReadOnly, Category="Item")
	TArray<FCPItemData> OwnedItems;

	/** Broadcast right after an item is added to OwnedItems */
	UPROPERTY(BlueprintAssignable, Category="Item")
	FOnCPItemAcquired OnItemAcquired;

	/** Every ICPInteractable currently in range of at least one registered interactable's collision */
	TArray<TWeakObjectPtr<AActor>> NearbyInteractables;

	/** Closest currently-registered interactable, i.e. what pressing Interact will activate */
	TWeakObjectPtr<AActor> CurrentInteractable;

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

	/** Called for interact input */
	void Interact(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles attack inputs from either controls or UI interfaces. Forwards to the current weapon if one is equipped, otherwise falls back to the legacy unarmed box-trace attack */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void DoAttack();

	/** Unequips the current weapon (if any) and equips WeaponClass. WeaponClass = None just unequips */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass);

	/** Unequips the current weapon and equips NewWeaponClass in its place */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* SwapWeapon(TSubclassOf<ACPWeaponBase> NewWeaponClass);

	/** Returns the currently equipped weapon, or null if unarmed */
	UFUNCTION(BlueprintPure, Category="Weapon")
	virtual ACPWeaponBase* GetCurrentWeapon() const;

	/** Handles dash inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Dash")
	virtual void DoDash();

	/** Interacts with the current registered interactable, if any and if it allows it */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void DoInteract();

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

	/** Recomputes CurrentInteractable as the closest valid entry in NearbyInteractables */
	void RefreshCurrentInteractable();

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

	// ~begin ICPInteractor

	/** Registers Interactable as being in range. It becomes CurrentInteractable if it's the closest */
	virtual void RegisterInteractable(AActor* Interactable) override;

	/** Removes Interactable from range. Clears CurrentInteractable if it was the active one */
	virtual void UnregisterInteractable(AActor* Interactable) override;

	// ~end ICPInteractor

	// ~begin ICPCoinWallet

	/** Adds Amount to the current coin balance */
	virtual void AddCoin(int32 Amount) override;

	/** Returns the current coin balance */
	virtual int32 GetCoinAmount() const override;

	/** Returns true if the current coin balance is at least Amount */
	virtual bool HasEnoughCoin(int32 Amount) const override;

	/** Attempts to spend Amount coins. Deducts and returns true on success, leaves the balance unchanged and returns false otherwise */
	virtual bool TrySpendCoin(int32 Amount) override;

	// ~end ICPCoinWallet

	// ~begin ICPItemInventory

	/** Adds an item to OwnedItems, applies its effect (if any), and broadcasts OnItemAcquired */
	virtual void AddOwnedItem(const FCPItemData& ItemData) override;

	/** Returns true if at least one item with the given code is owned */
	virtual bool HasItem(FName ItemCode) const override;

	/** Returns how many items with the given code are owned */
	virtual int32 GetItemCount(FName ItemCode) const override;

	/** Returns every currently owned item, in acquisition order */
	UFUNCTION(BlueprintPure, Category="Item")
	virtual const TArray<FCPItemData>& GetOwnedItems() const override { return OwnedItems; }

	// ~end ICPItemInventory

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

	/** Returns WeaponManager subobject **/
	FORCEINLINE class UCPWeaponManagerComponent* GetWeaponManager() const { return WeaponManager; }
};
