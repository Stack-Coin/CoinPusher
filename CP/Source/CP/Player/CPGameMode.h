// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/CPStatTypes.h"
#include "CPGameMode.generated.h"

class ACPPlayerCharacter;
class UCPHorizonGuageBarWidget;
class UCPTicketCountWidget;
class UCPCoinCountWidget;
class UCPInGameWidget;
class USoundBase;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamLevelUp, int32, NewLevel);

UCLASS(abstract)
class ACPGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	//***** �� ���� ����
	UPROPERTY(BlueprintReadOnly, Category="Team")
	float TeamExperience = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Team")
	int32 TeamLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team|Ranges")
	FCPStatRange TeamLevelRange = FCPStatRange(1.0f, 99.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team|Ranges")
	FCPStatRange TeamExperienceRange = FCPStatRange(0.0f, 999999.0f);

	// ���� �� �ϴ� ���� �ʿ��� ����ġ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Team|Leveling", meta = (ClampMin = 0))
	float BaseRequiredTeamExperience = 100.0f;

	// ���� ���� �ʿ� ����ġ �߰��ϴ� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Team|Leveling", meta = (ClampMin = 0))
	float RequiredTeamExperiencePerLevel = 0.0f;

	/** Widget Blueprint (inheriting UCPTicketCountWidget) for the player's ticket count HUD. Created once in
	 *  BeginPlay and bound directly to the player's OnTicketChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPTicketCountWidget> TicketWidgetClass;

	/** Widget Blueprint (inheriting UCPCoinCountWidget) for the player's score count HUD. Created once in
	 *  BeginPlay and bound directly to the player's OnScoreChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPCoinCountWidget> CoinWidgetClass;

	/** Widget Blueprint (inheriting UCPHorizonGuageBarWidget) for the player's health bar */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPHorizonGuageBarWidget> PlayerHealthBarWidgetClass;

	//***** BGM *****
	/** 평상시 재생되는 인게임 BGM */
	UPROPERTY(EditAnywhere, Category="BGM")
	TObjectPtr<USoundBase> MainBgmSound;

	UPROPERTY(EditAnywhere, Category="BGM", meta = (ClampMin = 0))
	float MainBgmVolume = 1.0f;

	/** 보스 출현 중(SpawnBoss ~ HandleBossDied) 재생되는 BGM */
	UPROPERTY(EditAnywhere, Category="BGM")
	TObjectPtr<USoundBase> BossBgmSound;

	UPROPERTY(EditAnywhere, Category="BGM", meta = (ClampMin = 0))
	float BossBgmVolume = 1.0f;

	/** 게임 승리(ShowEndingResult(true)) 시 재생되는 BGM */
	UPROPERTY(EditAnywhere, Category="BGM")
	TObjectPtr<USoundBase> ClearBgmSound;

	UPROPERTY(EditAnywhere, Category="BGM", meta = (ClampMin = 0))
	float ClearBgmVolume = 1.0f;

	/** 게임 패배(ShowEndingResult(false)) 시 재생되는 BGM */
	UPROPERTY(EditAnywhere, Category="BGM")
	TObjectPtr<USoundBase> LoseBgmSound;

	UPROPERTY(EditAnywhere, Category="BGM", meta = (ClampMin = 0))
	float LoseBgmVolume = 1.0f;

	/** MainBgmSound/BossBgmSound/ClearBgmSound/LoseBgmSound를 재생하는 단일 컴포넌트 - 평소엔
	 *  UpdateBgmPlayback()이, 게임 종료 시엔 HandleGameEnded()가 직접 SetSound() 후 Play()함.
	 *  BeginPlay에서 SpawnSound2D로 생성 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BgmComponent;

	/** 일시정지 메뉴가 열려있는 동안 true - true면 어떤 곡도 재생하지 않는다 */
	bool bIsBgmPaused = false;

	/** 보스가 살아있는 동안(SpawnBoss ~ HandleBossDied) true - true면 MainBgmSound 대신 BossBgmSound를 재생 */
	bool bIsBossActive = false;

	/** 게임이 끝난(승/패 Ending이 뜬) 후 true - true면 UpdateBgmPlayback()이 더 이상 관여하지 않고
	 *  (Pause/Boss 상태 변화 무시), HandleGameEnded()가 재생한 Clear/Lose BGM을 그대로 유지한다 */
	bool bHasGameEnded = false;

public:

	/** Constructor */
	ACPGameMode();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Assigns a PlayerStart tagged Player0 to the player, falling back to any PlayerStart in the level */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:

	/** Creates PlayerCharacter's ticket/coin HUD widgets (see TicketWidgetClass/CoinWidgetClass) and binds
	 *  them directly to PlayerCharacter's OnTicketChanged/OnScoreChanged. Called once per local player from BeginPlay */
	void SetupPlayerWalletWidgets(ACPPlayerCharacter* PlayerCharacter);

	/** Creates a health bar widget using HealthBarWidgetClass, adds it to PlayerCharacter's owning
	 *  player's screen, and binds it directly to PlayerCharacter's OnHealthChanged. Called once per
	 *  local player from BeginPlay */
	void SetupPlayerHealthBarWidget(ACPPlayerCharacter* PlayerCharacter, TSubclassOf<UCPHorizonGuageBarWidget> HealthBarWidgetClass);

	/** Binds PlayerCharacter's OnHealthChanged/OnExpChanged/OnLevelChanged/OnTicketChanged directly to
	 *  its possessing ACPTopDownPlayerController's InGameUI (GetInGameWidget()), and pushes each
	 *  value's current state once right after binding. Deferred to next tick (via a SetTimerForNextTick
	 *  call in BeginPlay) because InGameUI is created in the controller's own BeginPlay, and actor
	 *  BeginPlay order between the GameMode and the PlayerController isn't guaranteed - takes a weak
	 *  pointer since PlayerCharacter isn't a member here. No-ops if the controller isn't an
	 *  ACPTopDownPlayerController or has no InGameUI (InGameWidgetClass unset) */
	void SetupPlayerInGameWidgetBindings(TWeakObjectPtr<ACPPlayerCharacter> WeakPlayerCharacter);

	/** Bound to PlayerCharacter->OnPlayerDowned in BeginPlay - treats any player going down as an
	 *  immediate loss and shows the Lose ending on the world's first local PlayerController (see
	 *  ACPTopDownPlayerController::ShowEndingResult) */
	UFUNCTION()
	void HandlePlayerDowned();

	/** BeginPlay에서 BgmComponent를 생성하고 MainBgmSound 재생을 시작 */
	void StartBgm();

	/** bIsBgmPaused/bIsBossActive/bHasGameEnded를 종합해 지금 재생돼야 할 곡(없음/Main/Boss)을 판단하고,
	 *  BgmComponent 상태가 다르면 맞춰준다 - 아래 4개 핸들러가 플래그를 바꾼 직후 항상 이 함수를 호출하므로
	 *  Pause 중 보스가 죽는 등 상태가 겹쳐도 항상 올바른 곡(혹은 무음)으로 수렴한다 */
	void UpdateBgmPlayback();

	/** ACPTopDownPlayerController::OnGamePauseStateChanged에 바인딩됨(BeginPlay) */
	UFUNCTION()
	void HandleGamePauseStateChanged(bool bIsPaused);

	/** UCPMonsterSpawnManagerComponent::OnBossAppeared에 바인딩됨(BeginPlay) */
	UFUNCTION()
	void HandleBossAppeared();

	/** UCPMonsterSpawnManagerComponent::OnBossDefeated에 바인딩됨(BeginPlay) */
	UFUNCTION()
	void HandleBossDefeated();

	/** ACPTopDownPlayerController::OnGameEnded에 바인딩됨(BeginPlay) */
	UFUNCTION()
	void HandleGameEnded(bool bIsClear);

public:

	/** Broadcast right after TeamLevel increases by 1 (once per level, even on a multi level up) */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamLevelUp OnTeamLevelUp;

	/** Adds team experience, handling one or multiple team level ups if enough is accumulated at once */
	UFUNCTION(BlueprintCallable, Category="Team")
	void AddTeamExperience(float Amount);

	/** Returns the current team experience */
	UFUNCTION(BlueprintPure, Category="Team")
	float GetTeamExperience() const { return TeamExperience; }

	/** Returns the current team level */
	UFUNCTION(BlueprintPure, Category="Team")
	int32 GetTeamLevel() const { return TeamLevel; }

	/** Returns the team experience required to go from the current team level to the next */
	UFUNCTION(BlueprintPure, Category="Team")
	float GetRequiredTeamExperience() const;
};
