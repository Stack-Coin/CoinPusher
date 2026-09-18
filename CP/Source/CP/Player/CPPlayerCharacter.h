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
#include "Weapon/CPAimDirectionInterface.h"
#include "Player/CPWeaponEquipper.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Debug/CPDebugTypes.h"
#include "CPPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UCPWeaponManagerComponent;
class ACPWeaponBase;
class ACPRoulette;
class UCPDebugCollisionShapeComponent;
class UCPMonsterSpawnManagerComponent;
class UDataTable;
struct FCPPlayerLevelStatRow;
struct FItemData;
class UTimelineComponent;
class UCurveFloat;
class UMaterialInstanceDynamic;
class UCameraShakeBase;
class UCPInventoryComponent;
class ACPCoinPusher;
class USoundBase;
class UNiagaraSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogCPPlayerCharacter, Log, All);

/** Broadcast right after an item is added to the player's inventory. UI (e.g. the item toast)
 *  should react to this event instead of the item actor touching any UI directly. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPItemAcquired, FCPItemData, AcquiredItem);

/** Broadcast the moment this player's Health reaches 0 and it enters the downed state */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPPlayerDowned);

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
class CP_API ACPPlayerCharacter : public ACharacter, public ICPStatInterface, public ICPInteractor, public ICPItemInventory, public ICPAimDirectionProvider, public ICPWeaponEquipper, public ICPKnockbackable
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

	/** Draws GetCapsuleComponent()'s wireframe while the F1 debug widget's PlayerHitbox checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPDebugCollisionShapeComponent* DebugHitboxShape;

	/** 뱀서류 몬스터 웨이브/라운드/보스 스폰을 전담하는 컴포넌트. 매 인스턴스에 자동으로 붙어있고,
	 *  MonsterClassByType/WaveInfoTable/RoundInfoTable은 전부 UPROPERTY(EditAnywhere)라 이 컴포넌트를
	 *  들고 있는 BP(또는 여기서 파생된 BP)의 Class Defaults 패널에서 직접 값을 채워야 함 - 런타임에
	 *  하드코딩 경로로 자동 채워지지 않음(과거엔 그랬는데, 그 경로는 쿠커가 못 봐서 패키지 빌드에서
	 *  전부 빠졌었음). See Monster/Spawner/CPMonsterSpawnManagerComponent */
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	TObjectPtr<USoundBase> LevelUpSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	FVector LevelUpSoundLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level", meta = (ClampMin = 0))
	float LevelUpSoundVolume = 1.0f;

	/** 레벨업 시(최초 1회) 재생할 이펙트. 비워두면 재생하지 않음 (see SetStat) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	TObjectPtr<UNiagaraSystem> LevelUpEffect;

	/** LevelUpEffect가 플레이어 RootComponent에 부착되어 따라다닐 로컬(상대) 위치 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	FVector LevelUpEffectLocationOffset = FVector::ZeroVector;

	/** LevelUpEffect의 로컬(상대) 회전 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	FRotator LevelUpEffectRotationOffset = FRotator::ZeroRotator;

	/** LevelUpEffect 스폰 스케일 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Level")
	FVector LevelUpEffectScale = FVector::OneVector;

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

	/** 플레이어가 데미지를 받을 때(TakeDamage) 재생할 사운드. 비워두면 재생하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Damage")
	TObjectPtr<USoundBase> DamageTakenSound;

	/** DamageTakenSound 재생 위치(플레이어 위치 기준)에 더할 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Damage")
	FVector DamageTakenSoundLocationOffset = FVector::ZeroVector;

	/** DamageTakenSound 재생 볼륨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Damage", meta = (ClampMin = 0))
	float DamageTakenSoundVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Wallet")
	int32 ScoreCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Wallet")
	int32 TicketCount = 0;

	/** Score 보유량이 이 개수만큼 늘어날 때마다 티켓 1개 획득 (see AddScore) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wallet", meta = (ClampMin = 1))
	int32 ScorePerTicket = 20;

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

	/** BuffCode (PlayerBuffDataTable row name) used for the buff icon shown while the equipped weapon's
	 *  attack-power passive buff is active (see ACPWeaponBase::ApplyPassiveStatBuff/UpdateAttackBuffIcon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Attack|Buff Icons")
	FName AttackBuffIconCode = FName("AttackBuff");

	/** BuffCode (PlayerBuffDataTable row name) used for the buff icon shown while the equipped weapon's
	 *  orbiting-crescent passive skill is active (see UCPOrbitPassiveSkillModule/UpdateOrbitBuffIcon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Attack|Buff Icons")
	FName OrbitBuffIconCode = FName("OrbitBuff");

	/** Buff icon widgets currently shown for the two buffs above, tracked so Tick can keep updating the
	 *  same instance instead of creating a new one every frame. Cleared back to null once the buff ends
	 *  (see UpdateAttackBuffIcon/UpdateOrbitBuffIcon) - the widget removes itself from its parent then */
	TWeakObjectPtr<class UCPBuffIconWidget> AttackBuffIconWidget;
	TWeakObjectPtr<class UCPBuffIconWidget> OrbitBuffIconWidget;

	/** True while a weapon combo string is in progress. Movement itself is NOT blocked while this is true -
	 *  only the movement-driven rotation is: the character's facing is locked to the attack direction
	 *  instead of following movement input (see HandleAttackStateChanged/OrientTowardsAttackDirection) */
	bool bIsAttackLocked = false;

	/** True while auto-attack is turned on (see SetAutoAttackEnabled) - the F1 debug widget's auto-attack
	 *  checkbox toggles this. While true, DoAttack() is retried every AutoAttackPollInterval seconds without
	 *  any attack input - the aim direction still comes from GetAttackDirection() as usual (mouse cursor/
	 *  gamepad right stick, falling back to the movement direction), so the player only ever adjusts where
	 *  the auto-attack swings land, never whether it swings */
	bool bAutoAttackEnabled = true;

	/** How often (in seconds) DoAttack() is retried while auto-attack is on. ACPWeaponBase::Attack() already
	 *  no-ops on its own combo/interval cooldown (see CanAttack), so this only needs to be short enough that
	 *  the next swing starts promptly once the weapon becomes ready again - it does not itself pace the
	 *  attack rate. Tune this in BP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Attack", meta = (ClampMin = 0.01, Units = "s"))
	float AutoAttackPollInterval = 0.1f;

	/** Loops TickAutoAttack every AutoAttackPollInterval while bAutoAttackEnabled is true */
	FTimerHandle AutoAttackTimerHandle;

	/** Distance covered by a single dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "cm"))
	float DashDistance = 600.0f;

	/** Duration of the dash movement, and of the invincibility window */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashDuration = 0.2f;

	/** Minimum time that must pass between dashes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0, Units = "s"))
	float DashCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash")
	TObjectPtr<USoundBase> DashSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash")
	FVector DashSoundLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Dash", meta = (ClampMin = 0))
	float DashSoundVolume = 1.0f;

	/** 인벤토리에서 Crown(Big) 코인 아이템을 사용했을 때 재생할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use")
	TObjectPtr<USoundBase> CrownUseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use", meta = (ClampMin = 0))
	float CrownUseSoundVolume = 1.0f;

	/** 인벤토리에서 Tower(CoinTower) 아이템을 사용했을 때 재생할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use")
	TObjectPtr<USoundBase> TowerUseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use", meta = (ClampMin = 0))
	float TowerUseSoundVolume = 1.0f;

	/** 인벤토리에서 Passive 또는 HP 코인 아이템을 사용했을 때 재생할 사운드 - 둘이 같은 사운드를 공유 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use")
	TObjectPtr<USoundBase> PassiveOrHPUseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Item Use", meta = (ClampMin = 0))
	float PassiveOrHPUseSoundVolume = 1.0f;

	/** Converts ApplyKnockback's Distance into a launch speed: Speed = Distance / KnockbackDuration
	 *  (same convention as DashDistance/DashDuration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Knockback", meta = (ClampMin = 0.01, Units = "s"))
	float KnockbackDuration = 0.2f;

	/** Additional vertical launch speed applied on top of the horizontal knockback, for a "popped up" feel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats|Knockback", meta = (Units = "cm/s"))
	float KnockbackLaunchStrength = 500.0f;

	/** True while the dash movement is in progress */
	bool bIsDashing = false;

	/** Number of active invincibility sources (dash, a debug override via SetDebugInvincible). Damage/
	 *  knockback are ignored whenever this is > 0. Use BeginInvincibility()/EndInvincibilityRequest() to
	 *  add/remove a source instead of tracking a single bool directly, so overlapping windows don't
	 *  cancel each other out */
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

	/** True while Health is at 0 and the character is lying down, uncontrollable */
	bool bIsDowned = false;

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

	//�귿
	TObjectPtr<ACPRoulette> Roulette;

	/** AddTicket()으로 한 번에 여러 장이 지급됐을 때, 그만큼 Roulette를 자동으로 순차 회전시키기 위해
	 *  남은 횟수. Roulette::bIsRolling 가드 때문에 한 번에 하나만 돌 수 있으므로, 스핀 하나가 끝날
	 *  때마다(HandleRouletteAutoRollFinished) 1개씩 소모하며 TryStartNextAutoRoll()로 다음 스핀을 이어간다 */
	UPROPERTY(Transient)
	int32 PendingAutoRollCount = 0;

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

	/** HealthItemID가 떨어져 HP가 회복될 때 재생할 사운드. 비워두면 재생하지 않음 (see HandleDropZoneItemDropped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	TObjectPtr<USoundBase> HealthRecoverSound;

	/** HealthRecoverSound 재생 위치에 더할 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FVector HealthRecoverSoundLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards", meta = (ClampMin = 0))
	float HealthRecoverSoundVolume = 1.0f;

	/** HealthItemID가 떨어져 HP가 회복될 때(최초 1회) 재생할 이펙트. 비워두면 재생하지 않음 (see HandleDropZoneItemDropped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	TObjectPtr<UNiagaraSystem> HealthRecoverEffect;

	/** HealthRecoverEffect가 플레이어 RootComponent에 부착되어 따라다닐 로컬(상대) 위치 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FVector HealthRecoverEffectLocationOffset = FVector::ZeroVector;

	/** HealthRecoverEffect의 로컬(상대) 회전 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FRotator HealthRecoverEffectRotationOffset = FRotator::ZeroRotator;

	/** HealthRecoverEffect 스폰 스케일 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop Zone Rewards")
	FVector HealthRecoverEffectScale = FVector::OneVector;

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

	/** Drives the equipped weapon's movement-trail effect (see ACPWeaponBase::SetMovementEffectActive):
	 *  on only while actually moving and not mid-attack (see bIsAttackLocked). Also drives the two weapon
	 *  passive-skill buff icons (see UpdateAttackBuffIcon/UpdateOrbitBuffIcon) */
	virtual void Tick(float DeltaTime) override;

	/** Shows/updates/hides the buff icon (AttackBuffIconCode) for CurrentWeapon's attack-power passive buff
	 *  (see ACPWeaponBase::GetPassiveStatBuffTimeRemaining/GetPassiveStatBuffDuration). Called every tick */
	void UpdateAttackBuffIcon(ACPWeaponBase* CurrentWeapon);

	/** Shows/updates/hides the buff icon (OrbitBuffIconCode) for CurrentWeapon's orbiting-crescent passive
	 *  skill, if its PassiveSkillModule is a UCPOrbitPassiveSkillModule (see GetActiveDurationRemaining/
	 *  GetActiveMaxDuration). No-ops (icon stays hidden) for any other weapon/module. Called every tick */
	void UpdateOrbitBuffIcon(ACPWeaponBase* CurrentWeapon);

	/** Shared by UpdateAttackBuffIcon/UpdateOrbitBuffIcon: while Remaining > 0, creates IconRef via the
	 *  current InGameWidget's BuffCreate(BuffCode) if it doesn't exist yet and calls UpdateBuff(Remaining,
	 *  MaxDuration) on it. Once Remaining <= 0, calls UpdateBuff(0, MaxDuration) one last time (so the
	 *  widget removes itself, per its own contract) and clears IconRef. No-ops entirely if there's no
	 *  InGameWidget (e.g. no PlayerController yet) */
	void UpdateBuffIcon(TWeakObjectPtr<class UCPBuffIconWidget>& IconRef, FName BuffCode, float Remaining, float MaxDuration);

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for attack input */
	void Attack(const FInputActionValue& Value);

	/** Bound to AutoAttackTimerHandle while bAutoAttackEnabled is true. Just calls DoAttack() - the weapon's
	 *  own CanAttack() cooldown makes this a no-op on every poll except the one right after it's ready again */
	void TickAutoAttack();

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

	/** Roulette 액터를 찾아 캐싱하고, 최초로 찾은 시점에 OnPickedUp을 구독한다
	 *  (HandleRouletteAutoRollFinished) - RollRoulette()의 수동 회전과 TryStartNextAutoRoll()의
	 *  자동 연속 회전 양쪽에서 공용으로 사용한다. 레벨에 Roulette가 없으면 nullptr 반환 */
	ACPRoulette* GetOrFindRoulette();

	/** PendingAutoRollCount가 남아있고 bIsDowned가 아니며 Roulette가 이미 돌고 있지 않다면, 티켓
	 *  1개를 소모해 다음 스핀을 시도한다. Roulette가 아직 스핀 중이면 아무것도 하지 않고 그 스핀이
	 *  끝날 때(HandleRouletteAutoRollFinished) 다시 호출되기를 기다린다. Roll() 자체가 실패하면
	 *  (추첨 후보가 하나도 없는 등 데이터 문제) 소모한 티켓을 돌려주고 남은 대기 수를 비워, 같은
	 *  이유로 계속 실패하는 무한 재시도를 막는다 */
	void TryStartNextAutoRoll();

	/** Roulette::OnPickedUp에 구독되어 스핀 하나가 끝날 때마다 호출됨(수동/자동 회전 공통) - 남은
	 *  PendingAutoRollCount가 있으면 TryStartNextAutoRoll()로 다음 스핀을 이어간다 */
	UFUNCTION()
	void HandleRouletteAutoRollFinished(FName ItemID, int32 Count);

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

	/** Puts the character into the downed (uncontrollable, lying down) state. Called when Health reaches 0 */
	void EnterDownedState();

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

	/** HandleInventoryItemUsed가 ItemDataTable에서 찾은 Row의 CoinType에 따라 CrownUseSound/
	 *  TowerUseSound/PassiveOrHPUseSound 중 맞는 것을 재생 (Row가 없거나 해당 CoinType용 사운드가
	 *  비어있으면 아무것도 하지 않음) */
	void PlayItemUseSound(const FItemData* Row) const;

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles attack inputs from either controls or UI interfaces. Forwards to the current weapon if one is equipped, otherwise falls back to the legacy unarmed box-trace attack */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void DoAttack();

	/** Turns auto-attack on/off. While on, DoAttack() is retried every AutoAttackPollInterval seconds with no
	 *  attack input required - bound to the F1 debug widget's auto-attack checkbox (see
	 *  UCPDebugWidget::HandleAutoAttackCheckChanged) */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetAutoAttackEnabled(bool bEnabled);

	/** Returns true while auto-attack is on */
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAutoAttackEnabled() const { return bAutoAttackEnabled; }

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

	/** Adds one invincibility source (dash starting, a debug override) */
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

	/** TicketCount에 Amount만큼 더하고, 그만큼 Roulette를 자동으로 순차 회전시킨다. Roulette는
	 *  bIsRolling 가드로 한 번에 하나만 돌 수 있으므로 Amount번을 한꺼번에 Roll()하지 않고,
	 *  PendingAutoRollCount에 Amount를 누적해 TryStartNextAutoRoll()이 스핀이 끝날 때마다
	 *  (HandleRouletteAutoRollFinished) 하나씩 이어서 돌리게 한다 */
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

	/** Returns true while this character is downed */
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsDowned() const { return bIsDowned; }

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

	/** Returns true while the character is invincible, for any reason (dash, or a debug override) */
	UFUNCTION(BlueprintPure, Category="Dash")
	bool IsInvincible() const { return InvincibilityRequestCount > 0; }

	/** Debug-only: force this character invincible (or not) regardless of dash state, until
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
