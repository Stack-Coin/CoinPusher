// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPInGameWidget.generated.h"

class UCPCharacterInfoWidget;
class UCPCoinComboWidget;
class UCPCoinPointUI;
class UCPInventoryWidget;
class UCPRouletteWidget;
class UCPTicketCountWidget;
class UImage;
class UTexture2D;

/**
 *  인게임 화면을 구성하는 여러 WBP를 한데 모아놓은 최상위 HUD 위젯 (기획 문서상의 "InGameUI").
 *  게임을 일시정지했을 때 뜨는 메뉴("InGamePauseUI", UCPInGamePauseWidget)와 CoinPusher
 *  Picture-in-Picture(UCPCoinPusherCaptureWidget)는 각각 ACPTopDownPlayerController가 별도
 *  인스턴스로 직접 생성/관리하므로 이 위젯은 포함하지 않는다 - InGameWidget은 다음 하위
 *  컴포넌트만 갖는다(전부 BindWidgetOptional - WBP에 배치하지 않은 항목은 해당 갱신 함수를
 *  호출해도 조용히 무시된다):
 *  - PlayerInfoWidget/BossInfoWidget : 각각 UCPCharacterInfoWidget
 *  - BackgroundImage : 배경 이미지
 *  - TicketCountWidget : UCPTicketCountWidget
 *  - InventoryWidget : UCPInventoryWidget (스스로 플레이어의 인벤토리 컴포넌트에 바인딩되므로
 *    이 위젯 쪽에는 별도의 갱신 진입점이 필요 없다)
 *  - CoinComboWidget : UCPCoinComboWidget
 *  - RouletteWidget : UCPRouletteWidget
 *  - CoinPointUI : UCPCoinPointUI (코인이 떨어진 위치에 안내 문구를 잠깐 띄우는 UI)
 *
 *  RouletteWidget/CoinPointUI는 이미 각자 풍부한 API(스핀 연출, 월드 위치 → 화면 좌표 계산 등)를
 *  갖고 있으므로 값 하나를 그대로 전달하는 방식 대신 Getter로 인스턴스 자체를 돌려준다 - 호출부
 *  (PlayerController, ACPDropZone::OnCoinDropped의 Bind Event 대상 등)가 필요한 함수를 직접
 *  호출하면 된다. 나머지(캐릭터 정보/티켓/콤보)는 값을 그대로 전달받아 해당 하위 위젯에
 *  전달해주는 진입점 함수를 제공한다.
 *
 *  위 8개 컴포넌트는 전부 Set*Visible(bool)로 개별 On/Off가 가능하다(해당 컴포넌트가 WBP에
 *  없으면 조용히 무시). NativeConstruct에서 BossInfoWidget만 기본적으로 꺼진 상태(Collapsed)로
 *  시작한다 - 아직 보스 관련 시스템이 없어 당장은 보여줄 값이 없기 때문. SetBossInfoVisible(true)
 *  를 호출하면 다시 켤 수 있다.
 */
UCLASS(abstract)
class CP_API UCPInGameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 플레이어 정보(체력/경험치/이름/초상화) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPCharacterInfoWidget> PlayerInfoWidget;

	/** 보스 정보(체력/경험치/이름/초상화) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPCharacterInfoWidget> BossInfoWidget;

	/** 배경 이미지 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** 티켓 개수 표시 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPTicketCountWidget> TicketCountWidget;

	/** 인벤토리 표시 (자체적으로 플레이어 인벤토리에 바인딩됨) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPInventoryWidget> InventoryWidget;

	/** 코인 콤보 표시 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPCoinComboWidget> CoinComboWidget;

	/** 룰렛 UI */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPRouletteWidget> RouletteWidget;

	/** 코인 포인트 안내 UI (코인이 떨어진 위치에 문구를 잠깐 띄웠다 지운다) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPCoinPointUI> CoinPointUI;

	/** BossInfoWidget을 기본적으로 꺼진 상태(Collapsed)로 초기화한다 - 나머지 컴포넌트는 WBP에
	 *  배치된 기본 가시성을 그대로 따른다 */
	virtual void NativeConstruct() override;

public:

	/** PlayerInfoWidget의 체력 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdatePlayerHealth(float CurrentHealth, float MaxHealth);

	/** PlayerInfoWidget의 경험치 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdatePlayerExp(float CurrentExp, float MaxExp);

	/** PlayerInfoWidget의 이름 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetPlayerName(const FText& CharacterName);

	/** PlayerInfoWidget의 레벨 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetPlayerLevel(int32 Level);

	/** PlayerInfoWidget의 초상화 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetPlayerPortrait(UTexture2D* Portrait);

	/** BossInfoWidget의 체력 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdateBossHealth(float CurrentHealth, float MaxHealth);

	/** BossInfoWidget의 경험치 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdateBossExp(float CurrentExp, float MaxExp);

	/** BossInfoWidget의 이름 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetBossName(const FText& CharacterName);

	/** BossInfoWidget의 레벨 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetBossLevel(int32 Level);

	/** BossInfoWidget의 초상화 설정 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetBossPortrait(UTexture2D* Portrait);

	/** TicketCountWidget 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdateTicketCount(int32 Count);

	/** CoinComboWidget의 콤보 수 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void SetComboCount(int32 Count);

	/** CoinComboWidget의 콤보 게이지 갱신 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	void UpdateComboGauge(float CurrentValue, float MaxValue);

	/** 룰렛 UI 인스턴스 (PlaySpin 등은 호출부가 직접 제어) */
	UFUNCTION(BlueprintCallable, Category="In Game")
	UCPRouletteWidget* GetRouletteWidget() const { return RouletteWidget; }

	/** 코인 포인트 안내 UI 인스턴스 - ShowPointText/ShowCoinPointText는 호출부가 직접 제어.
	 *  ACPDropZone::OnCoinDropped(FVector)를 GetCoinPointUI()->ShowCoinPointText에 Bind Event하면
	 *  코인이 떨어질 때마다 그 위치에 안내 문구가 자동으로 뜬다 */
	UFUNCTION(BlueprintCallable, Category="In Game")
	UCPCoinPointUI* GetCoinPointUI() const { return CoinPointUI; }

	/** PlayerInfoWidget을 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetPlayerInfoVisible(bool bVisible);

	/** BossInfoWidget을 켜고 끈다 - 없으면 조용히 무시. NativeConstruct에서 기본적으로 꺼진다 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetBossInfoVisible(bool bVisible);

	/** BackgroundImage를 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetBackgroundVisible(bool bVisible);

	/** TicketCountWidget을 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetTicketCountVisible(bool bVisible);

	/** InventoryWidget을 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetInventoryVisible(bool bVisible);

	/** CoinComboWidget을 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetCoinComboVisible(bool bVisible);

	/** RouletteWidget을 켜고 끈다 - 없으면 조용히 무시 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetRouletteVisible(bool bVisible);

	/** CoinPointUI를 켜고 끈다 - 없으면 조용히 무시. 꺼져 있는 동안 ShowPointText가 호출되면
	 *  자식으로 추가되는 개별 텍스트 위젯들도 부모를 따라 함께 숨겨진다 */
	UFUNCTION(BlueprintCallable, Category="In Game|Visibility")
	void SetCoinPointUIVisible(bool bVisible);
};
