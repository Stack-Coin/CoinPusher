// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CoinPusher/CPCoinTypes.h"
#include "CPItemData.generated.h"

class AActor;
class UTexture2D;
class UStaticMesh;
class UMaterialInterface;
class ACPWeaponBase;

/** 아이템 마스터 데이터 한 행. UCPItemDataTableGameInstance::ItemDataTable의 Row Struct로 쓰인다 */
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	/** 식별용 아이템 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName ID;

	/** 아이템 대분류 (예: Coin, Item, Equipment 등 - 프로젝트에서 자유롭게 정의) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName Category;

	/** Category 안에서의 세부 종류 (예: Category가 Coin이면 Normal/Big/Monster 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName Type;

	/** 화면에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FText Name;

	/** 이 아이템이 코인일 때 적용할 코인 타입 (코인이 아니면 무시됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	ECPCoinType CoinType = ECPCoinType::Normal;

	/** ACPCoinPusher(천장 Dispenser 등)에서 이 아이템을 스폰할 때 사용할 액터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TSubclassOf<AActor> CoinPusherSpawnBPClass;

	/** 월드에 스폰할 때 사용할 액터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TSubclassOf<AActor> WorldSpawnBPClass;

	/** 이 아이템이 룰렛에서 당첨될 수 있는 후보인지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	bool bRoulette = false;

	/** 룰렛 당첨 확률 가중치. bRoulette가 true인 모든 행의 RouletteProbability 합(TotalProbability)
	 *  대비 이 행의 비율로 당첨 확률이 결정되며, 합이 반드시 1일 필요는 없다. bRoulette가 false면 무시됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	float RouletteProbability = 0.0f;

	/** 룰렛에서 이 아이템이 당첨됐을 때 스폰(또는 전달)할 개수. bRoulette가 false면 무시됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	int32 RouletteSpawnCount = 1;

	/** 이 아이템이 룰렛에서 당첨된 후 CoinPusher에 스폰되는지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	bool bRouletteToCoinPusher = false;

	/** Category가 Coin일 때, 이 행의 코인이 드랍되면 플레이어에게 지급할 경험치 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	float ExperienceAmount = 0.0f;

	/** Category가 Coin일 때, 이 행의 코인이 드랍되면 플레이어에게 지급할 Score 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	int32 ScoreAmount = 0;

	/** CoinPoint 로 표시될 Text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FText CoinPointText;

	/** 인벤토리 슬롯에 표시할 아이콘 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TObjectPtr<UTexture2D> InventoryIcon = nullptr;

	/** ACPItem이 월드에 3D 메시로 표시될 때 사용할 스태틱 메시. ACPItem::ApplyItemData()가 ItemId로
	 *  이 행을 찾아 자신의 Mesh 컴포넌트에 적용한다. 비어있으면(nullptr) Mesh에 기존에 지정된
	 *  스태틱 메시를 그대로 둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TObjectPtr<UStaticMesh> ItemMesh = nullptr;

	/** ItemMesh(또는 Mesh 컴포넌트에 이미 지정된 메시)의 슬롯 0에 적용할 머티리얼. 비어있으면
	 *  (nullptr) 메시에 원래 지정된 머티리얼을 그대로 둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TObjectPtr<UMaterialInterface> ItemMaterial = nullptr;

	/** ItemMesh(또는 Mesh 컴포넌트에 이미 지정된 메시)의 슬롯 1에 적용할 머티리얼. 비어있으면
	 *  (nullptr) 메시에 원래 지정된 머티리얼을 그대로 둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TObjectPtr<UMaterialInterface> ItemMaterial2 = nullptr;

	/** ACPItem이 3D 메시 대신 평면(Billboard)으로 표시될 때 쓰는 이미지. ACPItem이 BillboardMaterial
	 *  로부터 만든 Dynamic Material Instance의 ItemTextureParameterName(기본 "ItemTexture") 텍스처
	 *  파라미터에 이 값을 넣어준다. 비어있으면(nullptr) Billboard(ImageMesh)를 숨긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TObjectPtr<UTexture2D> ItemImage = nullptr;

	/** 이 아이템이 무기일 때 장착할 무기 액터 클래스. 무기 아이템이 아니면 비워둔다 (nullptr) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TSubclassOf<ACPWeaponBase> WeaponClass;
};
