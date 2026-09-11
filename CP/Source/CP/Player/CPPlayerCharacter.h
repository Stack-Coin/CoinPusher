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
#include "Player/CPItemInventory.h"
#include "Player/CPItemTypes.h"
#include "Player/CPReviveInterface.h"
#include "Weapon/CPAimDirectionInterface.h"
#include "Player/CPWeaponEquipper.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Debug/CPDebugTypes.h"
#include "CPPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UCPRadialGaugeComponent;
class UInputAction;
struct FInputActionValue;
class UCPWeaponManagerComponent;
class ACPWeaponBase;
class ACPRoulette;
class UCPDebugCollisionShapeComponent;
class UCPMonsterSpawnManagerComponent;
class UDataTable;
struct FCPPlayerLevelStatRow;
class UTimelineComponent;
class UCurveFloat;
class UMaterialInstanceDynamic;
class UCameraShakeBase;
class UCPInventoryComponent;
class ACPCoinPusher;

DECLARE_LOG_CATEGORY_EXTERN(LogCPPlayerCharacter, Log, All);

/** Broadcast right after an item is added to the player's inventory. UI (e.g. the item toast)
 *  should react to this event instead of the item actor touching any UI directly. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPItemAcquired, FCPItemData, AcquiredItem);

/** Broadcast the moment this player's Health reaches 0 and it enters the downed (revivable) state */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPPlayerDowned);

/** Broadcast the moment a downed player finishes being revived */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPPlayerRevived);

/** Broadcast whenever Health changes (see SetStat). Bind a UCPHorizonGuageBarWidget's Update (or a
 *  UCPHealthBarComponent/UCPViewportHealthBarComponent's UpdateHealth) here to keep a health bar in
 *  sync - done automatically by ACPGameMode::SetupPlayerHealthBarWidget */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPPlayerHealthChanged, float, CurrentHealth, float, MaxHealth);

/** Broadcast whenever Experience changes (see SetStat's Experience case) - CurrentExp/MaxExp are the
 *  values for the player's *current* level (MaxExp = GetRequiredExperienceForLevel(Stats.Level) after
 *  any level-ups this change caused are resolved). Bind a UCPHorizonGuageBarWidget's Update here the
 *  same way FOnCPPlayerHealthChanged drives a health bar */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPPlayerExpChanged, float, CurrentExp, float, MaxExp);

/** Broadcast whenever Level actually changes (level-up via Experience, or a direct SetStat(Level, ...)) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPPlayerLevelChanged, int32, NewLevel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPPlayerScoreChanged, int32, NewScoreCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPPlayerTicketChanged, int32, NewTicketCount);

/**
 *  Top-down / quarter view action prototype character.
 *  - 8-directional WASD/left-stick movement relative to the fixed camera
 *  - Mouse cursor (or gamepad right stick) directed rectangular (box trace) melee attack - same control
 *    scheme either way: movement never determines aim (see GetAttackDirection)
 *  - Directional dash with temporary invincibility
 *  - Movement is never blocked by attacking: while a combo string is in progress, the character's facing
 *    is locked to the attack direction instead of following movement (see HandleAttackStateChanged),
 *    then returns to following movement PostAttackRotationDelay seconds after the attack's motion ends
 *    (see OrientTowardsAttackDirection/ReorientToMovementDirection). GetMovementDirection() exposes the
 *    current movement direction relative to that facing, for a 4-way movement Blend Space in the Anim BP
 */
UCLASS(abstract)
class CP_API ACPPlayerCharacter : public ACharacter, public ICPStatInterface, public ICPInteractor, public ICPItemInventory, public ICPAimDirectionProvider, public ICPWeaponEquipper, public ICPKnockbackable, public ICPReviveProgressProvider
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

	/** Detects other players standing nearby while this character is downed, to drive the revive
	 *  timer. Always overlap-active; the overlap handlers no-op unless bIsDowned is true */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* ReviveDetectionRange;

	/** Draws GetCapsuleComponent()'s wireframe while the F1 debug widget's PlayerHitbox checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPDebugCollisionShapeComponent* DebugHitboxShape;

	/** 뱀서류 몬스터 웨이브/라운드/보스 스폰을 전담하는 컴포넌트. 매 인스턴스에 자동으로 붙어있고,
	 *  MonsterClassByType/SpawnWaveEntryTable/RoundInfoTable 기본값은 이 컴포넌트의 생성자
	 *  (ConstructorHelpers)에서 자동으로 채워짐 - Details 패널에서 개별적으로 덮어쓸 수 있음.
	 *  See Monster/Spawner/CPMonsterSpawnManagerComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCPMonsterSpawnManagerComponent> MonsterSpawnManager;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* HitFlashTimeline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPInventoryComponent* InventoryComponent;

protected:

	/** Move Input Action (WASD / Axis2D) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Attack Input Action (Mouse Left Button) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AttackAction;

	/** Aim Input Action (Gamepad Right Stick, Axis2D) - lets a gamepad player set the attack direction
	 *  directly and independently of movement, the same way a keyboard/mouse player aims with the mouse
	 *  cursor instead of their movement direction. Left unmapped for keyboard/mouse (mouse cursor is used
	 *  instead - see GetAttackDirection) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AimAction;

	/** Dash Input Action (Left Shift) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

	/** Interact Input Action (E key) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	/** Interact Input Action (R key) */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* RollRouletteAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* UseSlotEastAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* UseSlotNorthAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* UseSlotWestAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* UseSlotSouthAction;

protected:

	/** Core per-player combat stats (health, attack power, move speed, attack speed, experience, level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	FCPPlayerStats Stats;

	/** Min/Max bounds for Health. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange HealthRange = FCPStatRange(0.0f, 100.0f);

	/** Min/Max bounds for AttackPower. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange AttackPowerRange = FCPStatRange(0.0f, 999.0f);

	/** Min/Max bounds for MoveSpeed. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange MoveSpeedRange = FCPStatRange(0.0f, 1200.0f);

	/** Min/Max bounds for AttackSpeed. SetStat/ModifyStat clamp to this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange AttackSpeedRange = FCPStatRange(0.1f, 5.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Ranges")
	FCPStatRange ExperienceRange = FCPStatRange(0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|DataTable")
	TObjectPtr<UDataTable> BaseStatTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|DataTable")
	TObjectPtr<UDataTable> LevelStatTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitFlash")
	TObjectPtr<UCurveFloat> HitFlashCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitFlash", meta = (ClampMin = 0.01))
	float HitFlashSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitFlash")
	FLinearColor HitFlashColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitFlash")
	FName HitFlashAmountParameterName = TEXT("FlashAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitFlash")
	FName HitFlashColorParameterName = TEXT("FlashColor");

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HitFlashMIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitCameraShake")
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|HitCameraShake", meta = (ClampMin = 0))
	float HitCameraShakeIntensity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Wallet")
	int32 ScoreCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Wallet")
	int32 TicketCount = 0;

	/** Score 보유량이 이 개수만큼 늘어날 때마다 티켓 1개 획득 (see AddScore) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wallet", meta = (ClampMin = 1))
	int32 ScorePerTicket = 10;

	/** After an attack's motion actually ends (attack montage finished, or the last combo swing was
	 *  dispatched if no montage is assigned - see ACPWeaponBase::OnAttackStateChanged), how long to keep
	 *  facing the attack direction before rotating back to face the movement direction again. 0 = rotate
	 *  back immediately. Tune this in BP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Attack", meta = (ClampMin = 0, Units = "s"))
	float PostAttackRotationDelay = 0.3f;

	/** Move input below this magnitude (stick axis value in [0, 1]) is treated as zero, so gamepad stick
	 *  drift/noise doesn't register as movement (or, for a gamepad player, as an attack direction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Move", meta = (ClampMin = 0, ClampMax = 1))
	float MoveInputDeadzone = 0.15f;

	/** True while a weapon combo string is in progress. Movement itself is NOT blocked while this is true -
	 *  only the movement-driven rotation is: the character's facing is locked to the attack direction
	 *  instead of following movement input (see HandleAttackStateChanged/OrientTowardsAttackDirection) */
	bool bIsAttackLocked = false;

	/** Distance covered by a single dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "cm"))
	float DashDistance = 600.0f;

	/** Duration of the dash movement, and of the invincibility window */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashDuration = 0.2f;

	/** Minimum time that must pass between dashes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashCooldown = 1.0f;

	/** Converts ApplyKnockback's Distance into a launch speed: Speed = Distance / KnockbackDuration
	 *  (same convention as DashDistance/DashDuration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Knockback", meta = (ClampMin = 0.01, Units = "s"))
	float KnockbackDuration = 0.2f;

	/** Additional vertical launch speed applied on top of the horizontal knockback, for a "popped up" feel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Knockback", meta = (Units = "cm/s"))
	float KnockbackLaunchStrength = 500.0f;

	/** How long another player must stand in ReviveDetectionRange to fully revive this character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Revive", meta = (ClampMin = 0, Units = "s"))
	float ReviveDuration = 5.0f;

	/** Radius of ReviveDetectionRange, i.e. how close another player must be to start/continue a revive */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Revive", meta = (ClampMin = 0, Units = "cm"))
	float ReviveDetectionRadius = 150.0f;

	/** Fraction of max Health restored when this character is revived */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Revive", meta = (ClampMin = 0, ClampMax = 1))
	float ReviveHealthPercent = 0.5f;

	/** How long this character stays invincible immediately after being revived */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Revive", meta = (ClampMin = 0, Units = "s"))
	float PostReviveInvincibilityDuration = 1.5f;

	/** If true, redraws ReviveDetectionRange's sphere at its current location on a short repeating timer.
	 *  Uses DebugReviveRangeColor normally, and turns red automatically while a character is downed */
	UPROPERTY(EditAnywhere, Category="Stats|Revive|Debug")
	bool bDrawDebugReviveRange = false;

	/** Color used to draw ReviveDetectionRange's debug sphere while not downed. Also applied to
	 *  ReviveDetectionRange's own ShapeColor, so the collision wireframe matches too */
	UPROPERTY(EditAnywhere, Category="Stats|Revive|Debug", meta = (EditCondition = "bDrawDebugReviveRange"))
	FColor DebugReviveRangeColor = FColor::Cyan;

	/** True while the dash movement is in progress */
	bool bIsDashing = false;

	/** Number of active invincibility sources (dash, the post-revive window, a debug override via
	 *  SetDebugInvincible). Damage/knockback are ignored whenever this is > 0. Use BeginInvincibility()/
	 *  EndInvincibilityRequest() to add/remove a source instead of tracking a single bool directly, so
	 *  overlapping windows (e.g. a dash ending while a post-revive window is still active) don't cancel
	 *  each other out */
	int32 InvincibilityRequestCount = 0;

	/** True while SetDebugInvincible(true) is active - tracked separately so a redundant call doesn't
	 *  double up (or a mismatched call double-remove) an InvincibilityRequestCount entry */
	bool bIsDebugInvincible = false;

	/** Game time the last dash was performed */
	float LastDashTime = -1000.0f;

	/** Last non-zero WASD/stick input (after the deadzone), used as the dash direction */
	FVector2D LastMoveInputVector = FVector2D(0.0f, 1.0f);

	/** Last non-zero right-stick aim input (after the deadzone). Persists after the stick returns to rest,
	 *  the same way the mouse cursor keeps pointing wherever it was last left - see GetAttackDirection */
	FVector2D LastGamepadAimInputVector = FVector2D(0.0f, 1.0f);

	/** True while the gamepad's right stick is currently being pushed past the deadzone. While true,
	 *  GetAttackDirection() aims with the stick instead of the mouse cursor, and the character continuously
	 *  faces the stick direction (see Aim) even outside of an attack */
	bool bIsGamepadAiming = false;

	/** Which device GetAttackDirection() currently prefers once the right stick isn't being pushed: true =
	 *  attack/face the movement direction (gamepad play - see GetLastMovementWorldDirection), false = the
	 *  mouse cursor. Set true the moment the right stick is used (Aim), set back to false the moment the
	 *  mouse is actually moved (checked in GetAttackDirection) - so whichever device was used most recently
	 *  wins, the same way the mouse cursor and the stick each "own" aiming for their own control scheme.
	 *  Mutable: flipped from the const GetAttackDirection() as it reacts to live input */
	mutable bool bIsUsingGamepadAim = false;

	/** World direction resolved for the dash currently in progress */
	FVector DashDirection = FVector::ForwardVector;

	/** Timer used to end the dash state after DashDuration */
	FTimerHandle DashDurationTimerHandle;

	/** Fires ReorientToMovementDirection() after PostAttackRotationDelay once an attack's motion actually
	 *  ends. Restarted/cleared by HandleAttackStateChanged and DoDash */
	FTimerHandle PostAttackRotationTimerHandle;

	/** True while Health is at 0 and the character is lying down, uncontrollable, waiting to be revived */
	bool bIsDowned = false;

	/** The other player currently standing in ReviveDetectionRange and reviving this character, if any */
	TWeakObjectPtr<AActor> CurrentReviver;

	/** Timer that fires Revive() once ReviveDuration has elapsed with CurrentReviver still in range */
	FTimerHandle ReviveTimerHandle;

	/** World time the current revive attempt started, or -1 if none is in progress. Used to compute
	 *  GetReviveTimeRemaining() without ticking */
	float ReviveStartTime = -1.0f;

	/** Redraws ReviveDetectionRange's sphere at its current location. Bound to DebugReviveRangeTimerHandle
	 *  when bDrawDebugReviveRange is true */
	FTimerHandle DebugReviveRangeTimerHandle;

	/** Ends the post-revive invincibility window (see PostReviveInvincibilityDuration). Started in Revive() */
	FTimerHandle PostReviveInvincibilityTimerHandle;

	/** Items the player has picked up. Never modify directly - go through AddOwnedItem/ICPItemInventory */
	UPROPERTY(BlueprintReadOnly, Category="Item")
	TArray<FCPItemData> OwnedItems;

	/** Broadcast right after an item is added to OwnedItems */
	UPROPERTY(BlueprintAssignable, Category="Item")
	FOnCPItemAcquired OnItemAcquired;

	/** Broadcast when this character is revived out of the downed state */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerRevived OnPlayerRevived;

	/** The RadialGaugeComponent ACPGameMode attaches to this player at creation time (see
	 *  ACPGameMode::AttachReviveGaugeToPlayer), driven during a revive attempt to show progress. May be
	 *  null if none was assigned (e.g. testing this character outside ACPGameMode) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCPRadialGaugeComponent> ReviveGaugeComponent;

	/** Periodically pushes revive progress to ReviveGaugeComponent while a revive attempt is in progress */
	FTimerHandle ReviveGaugeUpdateTimerHandle;

	/** Every ICPInteractable currently in range of at least one registered interactable's collision */
	TArray<TWeakObjectPtr<AActor>> NearbyInteractables;

	/** Closest currently-registered interactable, i.e. what pressing Interact will activate */
	TWeakObjectPtr<AActor> CurrentInteractable;

	//�귿
	TObjectPtr<ACPRoulette> Roulette;

	/** 레벨에 배치된 ACPCoinPusher 참조. PostInitializeComponents()에서 자동으로 찾아 채워지며,
	 *  InventoryComponent가 자신의 BeginPlay()에서(=이 캐릭터의 나머지 BeginPlay 로직보다도 먼저 실행됨)
	 *  GetOwner()를 통해 이 값을 읽어 CoinPusher의 LinkedRoulette에 접근해야 하므로, (RollRoulette()의
	 *  Roulette처럼) 첫 사용 시점에 지연 조회하지 않고 컴포넌트들의 BeginPlay보다 먼저 실행되는
	 *  PostInitializeComponents()에서 미리 채워둔다 */
	TObjectPtr<ACPCoinPusher> CoinPusher;

	/** DropZone에 이 ItemID가 떨어지면 HealthGrantAmount만큼 HP 획득 (see HandleDropZoneItemDropped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FName HealthItemID = FName("3C");

	/** HealthItemID가 떨어졌을 때 ModifyStat(Health, ...)에 넘길 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards", meta = (ClampMin = 0))
	float HealthGrantAmount = 10.0f;

	/** DropZone에 이 ItemID가 떨어지면 현재 무기의 패시브 스킬 실행 (see HandleDropZoneItemDropped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FName PassiveSkillItemID = FName("2C");

	/** ItemDataTable에서 조회한 FItemData::Category가 이 값과 같으면 코인으로 취급해 그 행의
	 *  ExperienceAmount/ScoreAmount만큼 경험치/Score를 지급한다 (see HandleDropZoneItemDropped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FName CoinCategoryName = FName("Coin");

	/** 필드 코인(ACPCoinItem)을 먹었을 때 CoinPusher->ItemSpawn()에 넘길 ItemID (see HandleFieldCoinCollected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Field Coin")
	FName FieldCoinSpawnItemID = FName("1C");

public:

	/** Constructor */
	ACPPlayerCharacter();

	/** 레벨에 배치된 ACPCoinPusher 참조 반환 (PostInitializeComponents()에서 자동으로 채워짐, 없으면 nullptr) */
	FORCEINLINE ACPCoinPusher* GetCoinPusher() const { return CoinPusher; }

protected:

	/** CoinPusher를 (이 캐릭터의 BeginPlay보다, 그리고 InventoryComponent 등 소유 컴포넌트들의
	 *  BeginPlay보다도 먼저) 미리 찾아 채워둔다 */
	virtual void PostInitializeComponents() override;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for attack input */
	void Attack(const FInputActionValue& Value);

	/** Called while the gamepad right stick is pushed past the deadzone - updates LastGamepadAimInputVector,
	 *  sets bIsGamepadAiming/bIsUsingGamepadAim true, and immediately (and continuously, every time this
	 *  fires) rotates the character to face the stick direction - even while not attacking */
	void Aim(const FInputActionValue& Value);

	/** Called when the gamepad right stick returns to rest (AimAction's Completed trigger). Just forwards
	 *  to StopGamepadAiming() - Aim()'s own deadzone check can also detect the stick being released (analog
	 *  drift/noise can keep AimAction "actuated" - Triggered - well past our own deadzone, so Completed
	 *  isn't guaranteed to be what actually catches the release) */
	void EndAim(const FInputActionValue& Value);

	/** Called for dash input */
	void StartDash(const FInputActionValue& Value);

	/** Called for interact input */
	void Interact(const FInputActionValue& Value);

	/** Called for RollRoulette input */
	void RollRoulette(const FInputActionValue& Value);

	void UseSlotEast(const FInputActionValue& Value);

	void UseSlotNorth(const FInputActionValue& Value);

	void UseSlotWest(const FInputActionValue& Value);

	void UseSlotSouth(const FInputActionValue& Value);

	/** Bound to WeaponManager->OnWeaponChanged. Subscribes to the newly equipped weapon's OnAttackStateChanged */
	UFUNCTION()
	void HandleWeaponChanged(ACPWeaponBase* NewWeapon);

	/** Bound to the current weapon's OnAttackStateChanged. Engages/releases the attack movement/rotation lock */
	UFUNCTION()
	void HandleAttackStateChanged(bool bIsAttacking);

	/** Bound to ReviveDetectionRange's OnComponentBeginOverlap. Starts a revive attempt if downed and
	 *  no revive is already in progress */
	UFUNCTION()
	void OnReviveRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to ReviveDetectionRange's OnComponentEndOverlap. Resets the in-progress revive attempt if
	 *  the reviving player leaves range before it completes */
	UFUNCTION()
	void OnReviveRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Puts the character into the downed (uncontrollable, lying down) state. Called when Health reaches 0 */
	void EnterDownedState();

	/** Starts (or restarts) a revive attempt credited to Reviver. Resets on OnReviveRangeEndOverlap,
	 *  completes into Revive() after ReviveDuration */
	void StartRevive(AActor* Reviver);

	/** Ends the downed state: restores movement/mesh rotation and partially restores Health */
	void Revive();

	/** Draws ReviveDetectionRange's sphere at its current location. Called on DebugReviveRangeTimerHandle
	 *  while bDrawDebugReviveRange is true */
	void DrawDebugReviveRangeShape() const;

	/** Bound to UCPDebugCollisionSubsystem::OnCollisionVisibilityChanged. Reacts to PlayerRevive
	 *  (ReviveDetectionRange) - the other categories are handled by DebugHitboxShape directly */
	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);

	/** Bound to CoinPusher->GetDropZoneDroppedDelegate() in BeginPlay. Activates the current weapon's
	 *  passive skill when ItemID matches PassiveSkillItemID, grants HealthGrantAmount health when it
	 *  matches HealthItemID, and separately looks the ItemID up in CoinPusher's ItemDataTable - if the
	 *  row's Category matches CoinCategoryName, grants that row's ExperienceAmount experience and
	 *  ScoreAmount score. No-ops for any ItemID that matches none of these */
	UFUNCTION()
	void HandleDropZoneItemDropped(FName ItemID);

	/** Bound to InventoryComponent->OnItemUsed in BeginPlay. Forwards the used item's ID/Count to all
	 *  four of CoinPusher's SpawnBigCoin/SpawnTower/ConvertActive/HPConvertActive - each of those already
	 *  validates the ItemID's own CoinType internally and no-ops if it doesn't match, so calling all four
	 *  unconditionally is safe; only the one matching the used item's actual CoinType does anything */
	UFUNCTION()
	void HandleInventoryItemUsed(FName ItemID, int32 Count);

	/** Starts/stops DebugReviveRangeTimerHandle and updates bDrawDebugReviveRange to match */
	void SetReviveRangeDebugDrawEnabled(bool bEnabled);

	/** Pushes current revive progress (elapsed time out of ReviveDuration) to ReviveGaugeComponent. Bound
	 *  to ReviveGaugeUpdateTimerHandle while a revive attempt is in progress */
	void UpdateReviveGaugeDisplay();

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles attack inputs from either controls or UI interfaces. Forwards to the current weapon if one is equipped, otherwise falls back to the legacy unarmed box-trace attack */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void DoAttack();

	// ~begin ICPWeaponEquipper

	/** Unequips the current weapon (if any) and equips WeaponClass. WeaponClass = None just unequips */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass) override;

	/** Returns the currently equipped weapon, or null if unarmed */
	UFUNCTION(BlueprintPure, Category="Weapon")
	virtual ACPWeaponBase* GetCurrentWeapon() const override;

	// ~end ICPWeaponEquipper

	/** Unequips the current weapon and equips NewWeaponClass in its place */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* SwapWeapon(TSubclassOf<ACPWeaponBase> NewWeaponClass);

	/** Handles dash inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Dash")
	virtual void DoDash();

	/** Interacts with the current registered interactable, if any and if it allows it */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void DoInteract();

protected:

	/** Resolves a WASD input vector into a world space direction relative to the fixed camera yaw */
	FVector GetWorldDirectionFromInput(const FVector2D& InputVector) const;

	/** Resolves the current attack/aim direction, in priority order:
	 *  1. While the gamepad's right stick is pushed (bIsGamepadAiming, see Aim/EndAim) - aims with
	 *     LastGamepadAimInputVector, converted to a world direction the same way movement input is.
	 *  2. Otherwise, whichever of mouse/gamepad was used most recently (bIsUsingGamepadAim, flipped to
	 *     false here the moment the mouse actually moves): if gamepad, aims in the current movement
	 *     direction (see GetLastMovementWorldDirection) instead of the mouse cursor - there's no reason to
	 *     aim with a cursor the player isn't touching.
	 *  3. Otherwise (mouse), aims at the cursor's world location (deprojected against a horizontal plane at
	 *     the character's height - not a collision trace, so it doesn't depend on the floor blocking any
	 *     particular channel), falling back to the movement direction if that fails.
	 *  This lets the same controller freely mix mouse aiming and gamepad right-stick aiming rather than
	 *  being locked to one, while movement (WASD/left stick) itself never affects aim */
	FVector GetAttackDirection() const;

	/** Converts LastMoveInputVector to a world-space direction, falling back to the character's current
	 *  forward vector if there's no movement input yet. Used for the dash direction */
	FVector GetLastMovementWorldDirection() const;


	/** Snaps the character to face the current attack/aim direction (see GetAttackDirection). Called once
	 *  when a combo string starts (see HandleAttackStateChanged) - movement itself is left untouched, so
	 *  the player can keep moving freely while the character's facing stays locked onto the attack direction */
	void OrientTowardsAttackDirection();

	/** Re-enables normal movement-driven rotation (CharacterMovementComponent turns the character to face
	 *  its movement direction again). Fires on PostAttackRotationTimerHandle, PostAttackRotationDelay
	 *  seconds after an attack's motion ends (see HandleAttackStateChanged) */
	void ReorientToMovementDirection();

	/** Shared by Aim()'s deadzone branch and EndAim(): if bIsGamepadAiming is already false, does nothing
	 *  (so repeated calls - e.g. every frame of residual stick noise below our deadzone - don't keep
	 *  resetting the countdown below and it never fires). Otherwise sets bIsGamepadAiming false and starts
	 *  the PostAttackRotationDelay countdown to ReorientToMovementDirection() */
	void StopGamepadAiming();

	/** Ends the dash movement and invincibility window */
	void EndDash();

	/** Adds one invincibility source (dash starting, a post-revive window, a debug override) */
	void BeginInvincibility();

	/** Removes one invincibility source. Clamped at 0, so a mismatched extra call is harmless */
	void EndInvincibilityRequest();

	/** Pushes current stat values onto the systems that use them (e.g. MoveSpeed -> MaxWalkSpeed) */
	void ApplyStatsToGameplay();

	void InitStatsFromDataTable();

	const FCPPlayerLevelStatRow* FindLevelStatRow(int32 InLevel) const;

	float GetRequiredExperienceForLevel(int32 InLevel) const;

	void PlayHitFlash();

	UFUNCTION()
	void HandleHitFlashUpdate(float Value);

	void PlayHitCameraShake();

	/** Recomputes CurrentInteractable as the closest valid entry in NearbyInteractables */
	void RefreshCurrentInteractable();

public:

	/** Overrides the default TakeDamage functionality to ignore damage while invincible */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ~begin ICPStatInterface

	/** Adds Delta to the current value of the given stat */
	virtual void ModifyStat(ECPStatType StatType, float Delta) override;

	/** Sets the given stat to an absolute value */
	virtual void SetStat(ECPStatType StatType, float NewValue) override;

	/** Returns the current value of the given stat */
	virtual float GetStat(ECPStatType StatType) const override;

	// ~end ICPStatInterface

	UFUNCTION(BlueprintCallable, Category="Wallet")
	void AddScore(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Wallet")
	int32 GetScoreAmount() const { return ScoreCount; }

	UFUNCTION(BlueprintPure, Category="Wallet")
	bool HasEnoughScore(int32 Amount) const { return ScoreCount >= Amount; }

	UFUNCTION(BlueprintCallable, Category="Wallet")
	bool TrySpendScore(int32 Amount);

	/** ACPCoinItem(필드 코인)이 Interact()에서 호출 - Amount만큼 Score를 지급하고, CoinPusher가 있으면
	 *  CoinPusher->ItemSpawn(FieldCoinSpawnItemID, 1)을 실행해 코인 1개를 CoinPusher에 스폰한다 */
	UFUNCTION(BlueprintCallable, Category="Wallet")
	void HandleFieldCoinCollected(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Wallet")
	void AddTicket(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category="Wallet")
	int32 GetTicketCount() const { return TicketCount; }

	UFUNCTION(BlueprintCallable, Category="Wallet")
	bool TrySpendTicket(int32 Amount = 1);

	// ~begin ICPInteractor

	/** Registers Interactable as being in range. It becomes CurrentInteractable if it's the closest */
	virtual void RegisterInteractable(AActor* Interactable) override;

	/** Removes Interactable from range. Clears CurrentInteractable if it was the active one */
	virtual void UnregisterInteractable(AActor* Interactable) override;

	// ~end ICPInteractor

	// ~begin ICPItemInventory

	/** Adds an item to OwnedItems, applies its effect (if any), and broadcasts OnItemAcquired */
	virtual void AddOwnedItem(const FCPItemData& ItemData) override;

	/** Returns true if at least one item with the given code is owned */
	virtual bool HasItem(FName ItemCode) const override;

	/** Returns how many items with the given code are owned */
	virtual int32 GetItemCount(FName ItemCode) const override;

	/** Broadcasts OnItemAcquired without touching OwnedItems - see ICPItemInventory */
	virtual void NotifyItemAcquired(const FCPItemData& ItemData) override;

	/** Returns every currently owned item, in acquisition order */
	UFUNCTION(BlueprintPure, Category="Item")
	virtual const TArray<FCPItemData>& GetOwnedItems() const override { return OwnedItems; }

	// ~end ICPItemInventory

	// ~begin ICPAimDirectionProvider

	/** Returns the current attack/aim direction (mouse cursor or gamepad right stick) - see GetAttackDirection() */
	virtual FVector GetAimDirection() const override { return GetAttackDirection(); }

	// ~end ICPAimDirectionProvider

	// ~begin ICPKnockbackable

	/** Pushes the character Distance units along Direction (converted to a launch speed via
	 *  KnockbackDuration, plus KnockbackLaunchStrength upward). No-ops while invincible (e.g. mid-dash) */
	virtual void ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor) override;

	// ~end ICPKnockbackable

	// ~begin ICPReviveProgressProvider

	/** Returns true while this character is downed and waiting to be revived */
	virtual bool IsDowned() const override { return bIsDowned; }

	/** Returns how many seconds of revive time remain, or 0 if no revive is currently in progress */
	virtual float GetReviveTimeRemaining() const override;

	/** Returns ReviveDuration, for UI progress-bar normalization */
	virtual float GetReviveDuration() const override { return ReviveDuration; }

	// ~end ICPReviveProgressProvider

	/** Returns the upper bound of the Health stat, for UI that needs Max as well as Current */
	UFUNCTION(BlueprintPure, Category="Stats")
	float GetMaxHealth() const { return HealthRange.Max; }

	/** Returns the experience required to level up from the player's *current* level (i.e. the "Max" to
	 *  pair with GetStat(Experience)'s "Current" for UI) */
	UFUNCTION(BlueprintPure, Category="Stats")
	float GetMaxExperience() const { return GetRequiredExperienceForLevel(Stats.Level); }

	/** Returns the player's current level */
	UFUNCTION(BlueprintPure, Category="Stats")
	int32 GetPlayerLevel() const { return Stats.Level; }

	/** Broadcast when this character becomes downed (Health reached 0). Public (moved out of the
	 *  protected block above) so C++ outside this class - e.g. ACPGameMode::BeginPlay binding its
	 *  HandlePlayerDowned - can AddDynamic to it directly (Blueprint's own Bind Event doesn't care
	 *  about C++ access specifiers, but a plain AddDynamic() call from another class's C++ does) */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerDowned OnPlayerDowned;

	/** Broadcast whenever Health changes (see SetStat). Bind a UCPHorizonGuageBarWidget's Update (or a
	 *  UCPHealthBarComponent/UCPViewportHealthBarComponent's UpdateHealth) here to keep a health bar
	 *  in sync - done automatically by ACPGameMode::SetupPlayerHealthBarWidget */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerHealthChanged OnHealthChanged;

	/** Broadcast whenever Experience changes - see FOnCPPlayerExpChanged's comment */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerExpChanged OnExpChanged;

	/** Broadcast whenever Level actually changes */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerLevelChanged OnLevelChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCPPlayerTicketChanged OnTicketChanged;

	/** Assigns the RadialGaugeComponent this character drives to show revive progress. Called once by
	 *  ACPGameMode right after this character is created (see ACPGameMode::AttachReviveGaugeToPlayer) */
	void SetReviveGaugeComponent(UCPRadialGaugeComponent* InComponent) { ReviveGaugeComponent = InComponent; }

	/** Returns the RadialGaugeComponent assigned via SetReviveGaugeComponent, or null if none yet */
	UCPRadialGaugeComponent* GetReviveGaugeComponent() const { return ReviveGaugeComponent; }

	/** Returns true while the dash movement is in progress */
	UFUNCTION(BlueprintPure, Category="Dash")
	bool IsDashing() const { return bIsDashing; }

	/** Returns true while a weapon combo string is in progress (facing is locked to the attack direction -
	 *  see bIsAttackLocked) */
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAttackLocked() const { return bIsAttackLocked; }

	/** Current movement direction in degrees relative to the character's own current facing (0 = forward,
	 *  +90 = right, -90 = left, ±180 = backward) - feed this into a 4-way (forward/left/right/backward)
	 *  movement Blend Space in the Animation Blueprint. Based on current velocity and is always relative to
	 *  however the character is currently facing, so it stays correct even while attacking has rotated the
	 *  character away from its movement direction (e.g. moving backward relative to the attack facing while
	 *  swinging a weapon correctly reads as ~180 / backward) */
	UFUNCTION(BlueprintPure, Category="Animation")
	float GetMovementDirection() const;

	/** Returns true while the character is invincible, for any reason (dash, post-revive window, or a
	 *  debug override) */
	UFUNCTION(BlueprintPure, Category="Dash")
	bool IsInvincible() const { return InvincibilityRequestCount > 0; }

	/** Debug-only: force this character invincible (or not) regardless of dash/revive state, until
	 *  toggled off again. Used by the F1 debug widget's per-player invincibility checkbox */
	UFUNCTION(BlueprintCallable, Category="Dash")
	void SetDebugInvincible(bool bEnabled);

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns WeaponManager subobject **/
	FORCEINLINE class UCPWeaponManagerComponent* GetWeaponManager() const { return WeaponManager; }
	FORCEINLINE class UCPMonsterSpawnManagerComponent* GetMonsterSpawnManager() const { return MonsterSpawnManager; }

	FORCEINLINE class UCPInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
};
