// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Debug/CPDebugTypes.h"
#include "CPDebugWidget.generated.h"

class UTextBlock;
class UCheckBox;

/**
 *  UCPDebugWidget
 *  F1 debug overlay - replaces the old UCPStatWidget for this purpose. Two panel areas:
 *   - Player info: Player1InfoText/Player2InfoText show every stat (ICPStatInterface) and the currently
 *     equipped weapon name (ICPWeaponEquipper) for local players 0 and 1.
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

	/** Player 1 (local player index 0)'s stats + current weapon */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* Player1InfoText;

	/** Player 2 (local player index 1)'s stats + current weapon */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* Player2InfoText;

	/** Toggles ECPDebugCollisionCategory::PlayerHitbox - both players' capsule collision */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerHitboxCheckBox;

	/** Toggles ECPDebugCollisionCategory::PlayerWeapon - both players' weapon hit-scan/projectile collision */
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

	/** Toggles ECPDebugCollisionCategory::PlayerRevive - both players' revive detection range */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* PlayerReviveCheckBox;

	/** Toggles ECPDebugCollisionCategory::CoinNexus - the Coin Nexus's collision sphere */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* CoinNexusCheckBox;

	/** Toggles ECPDebugCollisionCategory::ItemPickup - world item/coin pickup range */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* ItemPickupCheckBox;

protected:

	/** Binds every checkbox and applies UCPDebugCollisionSubsystem's current state to them */
	virtual void NativeConstruct() override;

	/** Refreshes the player info text every frame while visible */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Pulls current stats/weapon for local players 0 and 1 into Player1InfoText/Player2InfoText */
	void RefreshPlayerInfo();

	/** Builds the multi-line stat/weapon readout for the local player at PlayerIndex (0 or 1) */
	FString BuildPlayerInfoString(int32 PlayerIndex) const;

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
};
