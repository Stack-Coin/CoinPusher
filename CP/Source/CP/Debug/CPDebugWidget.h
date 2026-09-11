// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Debug/CPDebugTypes.h"
#include "CPDebugWidget.generated.h"

class UTextBlock;
class UCheckBox;
class UEditableText;
class UButton;

/**
 *  UCPDebugWidget
 *  F1 debug overlay - replaces the old UCPStatWidget for this purpose. Two panel areas:
 *   - Player info: PlayerInfoText shows every stat (ICPStatInterface) and the currently
 *     equipped weapon name (ICPWeaponEquipper) for the local player.
 *   - Collision: one checkbox per ECPDebugCollisionCategory. Checking it tells UCPDebugCollisionSubsystem
 *     to draw that category's collision shape(s) as a wireframe; unchecking hides it again.
 *  No layout/design is provided - place the TextBlocks/CheckBoxes (matching these variable names) in the
 *  Widget Blueprint that inherits from this class. Created/shown by ACPTopDownPlayerController on F1.
 */
UCLASS(abstract)
class CP_API UCPDebugWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The local player's stats + current weapon */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* PlayerInfoText;

	/** Toggles ECPDebugCollisionCategory::PlayerHitbox - the player's capsule collision */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerHitboxCheckBox;

	/** Toggles ECPDebugCollisionCategory::PlayerWeapon - the player's weapon hit-scan/projectile collision */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerWeaponCheckBox;

	/** Toggles ECPDebugCollisionCategory::EnemyHitbox - monster capsule collision */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* EnemyHitboxCheckBox;

	/** Toggles ECPDebugCollisionCategory::MonsterAttackRange - monster attack sweep, drawn once per AttackHitCheck */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* MonsterAttackRangeCheckBox;

	/** Toggles ECPDebugCollisionCategory::MonsterDetectRange - monster detection range, redrawn every UCPBTService_Detect tick */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* MonsterDetectRangeCheckBox;

	/** Toggles ECPDebugCollisionCategory::PlayerRevive - the player's revive detection range */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerReviveCheckBox;

	/** Toggles ECPDebugCollisionCategory::CoinNexus - the Coin Nexus's collision sphere */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* CoinNexusCheckBox;

	/** Toggles ECPDebugCollisionCategory::ItemPickup - world item/coin pickup range */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* ItemPickupCheckBox;

	/** Amount added to the team score count when SetTeamCoinButton is clicked (see ACPPlayerCharacter::AddScore) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* TeamCoinInputText;

	/** Adds TeamCoinInputText's value to the local player's score count via ACPPlayerCharacter::AddScore */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* SetTeamCoinButton;

	/** Amount added to the local player's ticket count when SetTeamTicketButton is clicked (see ACPPlayerCharacter::AddTicket) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* TeamTicketInputText;

	/** Adds TeamTicketInputText's value to the local player's ticket count via ACPPlayerCharacter::AddTicket */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* SetTeamTicketButton;

	/** Toggles invincibility for the local player (see ACPPlayerCharacter::SetDebugInvincible) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerInvincibleCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* DamageInputText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* ApplyDamageButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* ActivatePassiveSkillButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* ItemCodeInputText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* ItemCountInputText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* StoreItemButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* RemoveItemButton;

	/** ItemID passed to ACPCoinPusher::SpawnBigCoin() when SpawnBigCoinButton is clicked - must match a row
	 *  (RowName) in the CoinPusher's ItemDataTable whose CoinType is Big, or the wrapper does nothing */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* BigCoinItemIDInputText;

	/** Count passed to ACPCoinPusher::SpawnBigCoin() when SpawnBigCoinButton is clicked */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableText* BigCoinSpawnCountInputText;

	/** Spawns BigCoinSpawnCountInputText's value worth of Big-type coins via the level's ACPCoinPusher */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* SpawnBigCoinButton;

protected:

	/** Binds every checkbox and applies UCPDebugCollisionSubsystem's current state to them */
	virtual void NativeConstruct() override;

	/** Refreshes the player info text every frame while visible */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Pulls current stats/weapon for the local player into PlayerInfoText */
	void RefreshPlayerInfo();

	/** Builds the multi-line stat/weapon readout for the local player */
	FString BuildPlayerInfoString() const;

	/** Sets CheckBox's initial checked state from Subsystem without triggering its OnCheckStateChanged, then binds Handler to it */
	void InitializeCheckBox(UCheckBox* CheckBox, ECPDebugCollisionCategory Category);

	/** Tells UCPDebugCollisionSubsystem to show/hide Category's collision shape(s) */
	void SetCategoryVisible(ECPDebugCollisionCategory Category, bool bVisible);

	UFUNCTION()
	void HandlePlayerHitboxCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandlePlayerWeaponCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandleEnemyHitboxCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandleMonsterAttackRangeCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandleMonsterDetectRangeCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandlePlayerReviveCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandleCoinNexusCheckChanged(bool bIsChecked);

	UFUNCTION()
	void HandleItemPickupCheckChanged(bool bIsChecked);

	/** Bound to SetTeamCoinButton. Parses TeamCoinInputText and adds it via ACPPlayerCharacter::AddCoin */
	UFUNCTION()
	void HandleSetTeamCoinClicked();

	/** Bound to SetTeamTicketButton. Parses TeamTicketInputText and adds it via ACPPlayerCharacter::AddTicket */
	UFUNCTION()
	void HandleSetTeamTicketClicked();

	UFUNCTION()
	void HandlePlayerInvincibleCheckChanged(bool bIsChecked);

	/** Casts UGameplayStatics::GetPlayerPawn(GetWorld(), 0) to ACPPlayerCharacter and calls
	 *  SetDebugInvincible(bEnabled) on it, if valid */
	void SetPlayerDebugInvincible(bool bEnabled);

	UFUNCTION()
	void HandleApplyDamageClicked();

	UFUNCTION()
	void HandleActivatePassiveSkillClicked();

	UFUNCTION()
	void HandleStoreItemClicked();

	UFUNCTION()
	void HandleRemoveItemClicked();

	/** Bound to SpawnBigCoinButton. Parses BigCoinItemIDInputText/BigCoinSpawnCountInputText and calls
	 *  SpawnBigCoin(ItemID, Count) on the first ACPCoinPusher found in the level */
	UFUNCTION()
	void HandleSpawnBigCoinClicked();
};
