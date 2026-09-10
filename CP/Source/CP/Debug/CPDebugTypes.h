// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPDebugTypes.generated.h"

/** One checkbox in the F1 debug widget's collision panel - which collision shape(s) get their debug
 *  wireframe drawn while this category is toggled on. Never disables actual collision/gameplay, purely
 *  a visualization aid (see UCPDebugCollisionSubsystem). */
UENUM(BlueprintType)
enum class ECPDebugCollisionCategory : uint8
{
	/** Both players' capsule collision (ACPPlayerCharacter) */
	PlayerHitbox,

	/** Both players' current weapon hit-scan shape (ACPMeleeWeapon), projectile collision (ACPProjectile),
	 *  the legacy unarmed attack box (ACPPlayerCharacter), and passive skill ranges (ACPMeteor,
	 *  ACPMeteorGroundZone, ACPOrbitingCrescent) */
	PlayerWeapon,

	/** Monster capsule collision (ACPMonsterBase) */
	EnemyHitbox,

	/** Monster attack hit-scan sweep, drawn once per AttackHitCheck (ACPMonsterBase) */
	MonsterAttackRange,

	/** Monster detection range, redrawn every tick of UCPBTService_Detect */
	MonsterDetectRange,

	/** Both players' revive detection range (ACPPlayerCharacter::ReviveDetectionRange) */
	PlayerRevive,

	/** Coin Nexus collision sphere (ACPNexus::CollisionSphere) */
	CoinNexus,

	/** World item pickup range (ACPWorldItem::InteractionRange, ACPCoinItem::CollisionSphere) */
	ItemPickup
};
