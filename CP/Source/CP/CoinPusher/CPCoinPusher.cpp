// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPCoinPusher.h"
#include "CPDispenser.h"
#include "CPDropZone.h"
#include "CPPassiveCoinConvertArea.h"
#include "CPCoinThrowArea.h"
#include "CPCoinTowerSpawner.h"
#include "CPPusher.h"
#include "CPCoin.h"
//#include "CPInput.h"
#include "../Nexus/CPNexus.h"
#include "CPCoinPusherViewCaptureComponent.h"
#include "../Roulette/CPRoulette.h"
#include "Datatables/CPItemData.h"
#include "Log/CPLogCategories.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

ACPCoinPusher::ACPCoinPusher()
{
	PrimaryActorTick.bCanEverTick = false;

	// 코인이 놓이는 바닥. RootComponent로 지정해 실제 충돌의 기준이 되도록 함.
	Floor = CreateDefaultSubobject<UBoxComponent>(TEXT("Floor"));

	RootComponent = Floor;

	Floor->SetBoxExtent(FVector(150.0f, 150.0f, 10.0f));
	Floor->SetCollisionProfileName(FName("BlockAllDynamic"));

	// 추가 비주얼 메시 (콜리전 없음, Floor에 부착). 용도는 BP에서 자유롭게 확장
	ExtraBoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExtraBoxMesh"));
	ExtraBoxMesh->SetupAttachment(Floor);
	ExtraBoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Floor);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWall"));
	LeftWall->SetupAttachment(Floor);
	LeftWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	LeftWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	RightWall = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWall"));
	RightWall->SetupAttachment(Floor);
	RightWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	RightWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	BackWall = CreateDefaultSubobject<UBoxComponent>(TEXT("BackWall"));
	BackWall->SetupAttachment(Floor);
	BackWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	BackWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	FrontWall = CreateDefaultSubobject<UBoxComponent>(TEXT("FrontWall"));
	FrontWall->SetupAttachment(Floor);
	FrontWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	FrontWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	PusherComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("PusherComponent"));
	PusherComponent->SetupAttachment(Floor);

	DispenserComponentA = CreateDefaultSubobject<UChildActorComponent>(TEXT("DispenserComponentA"));
	DispenserComponentA->SetupAttachment(Floor);

	DispenserComponentB = CreateDefaultSubobject<UChildActorComponent>(TEXT("DispenserComponentB"));
	DispenserComponentB->SetupAttachment(Floor);

	// 천장에서 물건을 뿌리는 Dispenser 5개. 위치/발사 설정과 Item Class는 BP에서 조정
	CeilingDispenserComponents.SetNum(5);
	for (int32 Index = 0; Index < CeilingDispenserComponents.Num(); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("CeilingDispenserComponent%d"), Index));
		UChildActorComponent* CeilingDispenserComponent = CreateDefaultSubobject<UChildActorComponent>(ComponentName);
		CeilingDispenserComponent->SetupAttachment(Floor);
		CeilingDispenserComponents[Index] = CeilingDispenserComponent;
	}

	DropZoneComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("DropZoneComponent"));
	DropZoneComponent->SetupAttachment(Floor);

	PassiveCoinConvertAreaComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("PassiveCoinConvertAreaComponent"));
	PassiveCoinConvertAreaComponent->SetupAttachment(Floor);

	MonsterCoinConvertAreaComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("MonsterCoinConvertAreaComponent"));
	MonsterCoinConvertAreaComponent->SetupAttachment(Floor);

	// ActiveWaveThrow()가 순차적으로 활성화시키는 CoinThrowArea 5개
	CoinThrowAreaComponents.SetNum(5);
	for (int32 Index = 0; Index < CoinThrowAreaComponents.Num(); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("CoinThrowAreaComponent%d"), Index));
		UChildActorComponent* CoinThrowAreaComponent = CreateDefaultSubobject<UChildActorComponent>(ComponentName);
		CoinThrowAreaComponent->SetupAttachment(Floor);
		CoinThrowAreaComponents[Index] = CoinThrowAreaComponent;
	}

	// SpawnTower()로 원형 코인 타워를 스폰/상승시키는 CoinTowerSpawner (컴포넌트를 통한 Has-a)
	CoinTowerSpawnerComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CoinTowerSpawnerComponent"));
	CoinTowerSpawnerComponent->SetupAttachment(Floor);

	// ViewCaptureComponent를 SpringArm 소켓에 붙여서 동작. 기본값은 위에서 내려다보는 구도이고,
	// ArmLength/각도는 ViewCaptureBoom을 통해 BP에서 조정.
	// ACPPartyCamera의 CameraBoom과 달리 이 Boom은 CoinPusher 자신(Floor)의 회전을 그대로 따라가야
	// 한다 - bInherit*를 꺼두면 레벨에 배치된 CoinPusher의 실제 회전과 무관하게 캡처 카메라가 항상
	// 고정된 월드 방향만 보게 되어, 배치 각도에 따라 엉뚱한 곳(바닥 밑, 허공 등)을 비출 수 있다
	ViewCaptureBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ViewCaptureBoom"));
	ViewCaptureBoom->SetupAttachment(Floor);
	ViewCaptureBoom->TargetArmLength = 400.0f;
	ViewCaptureBoom->SetRelativeRotation(FRotator(-70.0f, 0.0f, 0.0f));
	ViewCaptureBoom->bUsePawnControlRotation = false;
	ViewCaptureBoom->bDoCollisionTest = false;

	ViewCaptureComponent = CreateDefaultSubobject<UCPCoinPusherViewCaptureComponent>(TEXT("ViewCaptureComponent"));
	ViewCaptureComponent->SetupAttachment(ViewCaptureBoom, USpringArmComponent::SocketName);
}

void ACPCoinPusher::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//ChildActorComponent가 스폰한 Dispenser는 레벨에 직접 배치된 액터가 아니라서
	//에디터에서 개별적으로 LinkedInput을 지정할 수 없다. 대신 CoinPusher가 들고 있는
	//InputA/InputB를 BeginPlay 이전에 각 Dispenser로 전달해 준다
	if (ACPDispenser* DispenserA = GetDispenserA())
	{
		DispenserA->SetLinkedInput(InputA);
	}

	if (ACPDispenser* DispenserB = GetDispenserB())
	{
		DispenserB->SetLinkedInput(InputB);
	}

	//DropZone도 마찬가지로 ChildActorComponent로 스폰되는 인스턴스라 레벨에서 직접 편집할 수
	//없으므로, 레벨(이 CoinPusher 인스턴스)에서 지정한 ItemRespawnDispenser를 대신 전달해 준다
	if (ACPDropZone* DropZone = GetDropZone())
	{
		DropZone->SetItemRespawnDispenser(ItemRespawnDispenser);
	}

	//CoinTowerSpawner도 ChildActorComponent로 스폰되는 인스턴스라, 스폰/상승 동안 멈춰야 할 Pusher를
	//레벨(BP)에서 직접 편집할 수 없다. 같은 CoinPusher가 소유한 Pusher를 대신 전달해 준다
	if (ACPCoinTowerSpawner* CoinTowerSpawner = GetCoinTowerSpawner())
	{
		CoinTowerSpawner->SetTargetPusher(GetPusher());
	}
}

void ACPCoinPusher::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 게임 시작 시 천장 Dispenser들이 각각 InitialCoinDropCount개씩 코인을 드롭
	for (const TObjectPtr<UChildActorComponent>& CeilingComponent : CeilingDispenserComponents)
	{
		if (!CeilingComponent)
		{
			continue;
		}

		if (ACPDispenser* CeilingDispenser = Cast<ACPDispenser>(CeilingComponent->GetChildActor()))
		{
			CeilingDispenser->DispenseItems(InitialCoinDropCount);
		}
	}

	// 게임 시작 FrontWallRemovalDelay초 후 FrontWall을 비활성화해 코인이 앞으로 빠질 수 있도록 함
	GetWorldTimerManager().SetTimer(FrontWallRemovalTimerHandle, this, &ACPCoinPusher::RemoveFrontWall, FrontWallRemovalDelay, false);

	// LinkedRoulette가 아이템을 뽑을 때마다(OnPickedUp) HandleRoulettePickedUp()이 자동으로
	// 호출되도록 등록 - 룰렛은 CoinPusher를 전혀 모르며, 이 CoinPusher가 스스로 룰렛의 결과를 구독하는 방식
	if (LinkedRoulette)
	{
		LinkedRoulette->OnPickedUp.AddDynamic(this, &ACPCoinPusher::HandleRoulettePickedUp);
	}
}

float ACPCoinPusher::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	ApplyDamage(ActualDamage, DamageCauser);

	return ActualDamage;
}

void ACPCoinPusher::ApplyDamage(float Damage, AActor* DamageCauser)
{
	if (bIsDestroyed || Damage <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);

	UE_LOG(LogCoinPusher, Warning, TEXT("%f"), CurrentHealth);
	OnDamaged.Broadcast(Damage, DamageCauser);

	if (CurrentHealth <= 0.0f)
	{
		HandleDestroyed();
	}
}

void ACPCoinPusher::HandleDestroyed()
{
	if (!bIsDestroyed)
	{
		bIsDestroyed = true;

		OnCoinPusherDestroyed.Broadcast();

		// call the BP handler to play effects, disable the machine, etc.
		BP_OnDestroyed();
	}
}

ACPPusher* ACPCoinPusher::GetPusher() const
{
	return PusherComponent ? Cast<ACPPusher>(PusherComponent->GetChildActor()) : nullptr;
}

ACPDispenser* ACPCoinPusher::GetDispenserA() const
{
	return DispenserComponentA ? Cast<ACPDispenser>(DispenserComponentA->GetChildActor()) : nullptr;
}

ACPDispenser* ACPCoinPusher::GetDispenserB() const
{
	return DispenserComponentB ? Cast<ACPDispenser>(DispenserComponentB->GetChildActor()) : nullptr;
}

ACPDispenser* ACPCoinPusher::GetCeilingDispenser(int32 Index) const
{
	if (!CeilingDispenserComponents.IsValidIndex(Index) || !CeilingDispenserComponents[Index])
	{
		return nullptr;
	}

	return Cast<ACPDispenser>(CeilingDispenserComponents[Index]->GetChildActor());
}

ACPDropZone* ACPCoinPusher::GetDropZone() const
{
	return DropZoneComponent ? Cast<ACPDropZone>(DropZoneComponent->GetChildActor()) : nullptr;
}

FOnCPDropZoneDropped* ACPCoinPusher::GetDropZoneDroppedDelegate() const
{
	ACPDropZone* DropZone = GetDropZone();
	return DropZone ? &DropZone->OnDropped : nullptr;
}

ACPPassiveCoinConvertArea* ACPCoinPusher::GetPassiveCoinConvertArea() const
{
	return PassiveCoinConvertAreaComponent ? Cast<ACPPassiveCoinConvertArea>(PassiveCoinConvertAreaComponent->GetChildActor()) : nullptr;
}

ACPPassiveCoinConvertArea* ACPCoinPusher::GetMonsterCoinConvertArea() const
{
	return MonsterCoinConvertAreaComponent ? Cast<ACPPassiveCoinConvertArea>(MonsterCoinConvertAreaComponent->GetChildActor()) : nullptr;
}

ACPCoinTowerSpawner* ACPCoinPusher::GetCoinTowerSpawner() const
{
	return CoinTowerSpawnerComponent ? Cast<ACPCoinTowerSpawner>(CoinTowerSpawnerComponent->GetChildActor()) : nullptr;
}

ACPCoinThrowArea* ACPCoinPusher::GetCoinThrowArea(int32 Index) const
{
	if (!CoinThrowAreaComponents.IsValidIndex(Index) || !CoinThrowAreaComponents[Index])
	{
		return nullptr;
	}

	return Cast<ACPCoinThrowArea>(CoinThrowAreaComponents[Index]->GetChildActor());
}

void ACPCoinPusher::RemoveFrontWall()
{
	if (FrontWall)
	{
		FrontWall->SetVisibility(false);
		FrontWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ACPCoinPusher::ItemSpawn(FName ItemID, int32 SpawnCount)
{
	// 코인 여부 판별/CoinType 적용은 Dispenser::DispenseItemByID()가 ItemDataTable을 조회해 알아서
	// 처리하므로, 여기서는 Dispenser 하나를 골라 그대로 위임하기만 하면 된다

	for (int i = 0; i < SpawnCount; ++i)
	{
		if (ACPDispenser* Dispenser = PickRandomValidCeilingDispenser())
		{
			Dispenser->DispenseItemByID(ItemID, 1);
		}
	}
}

void ACPCoinPusher::HandleRoulettePickedUp(FName ItemID, int32 SpawnCount)
{
	if (!ItemDataTable)
	{
		return;
	}

	const FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ACPCoinPusher::HandleRoulettePickedUp"));
	if (Row && Row->bRouletteToCoinPusher)
	{
		ItemSpawn(ItemID, SpawnCount);
	}
}

void ACPCoinPusher::SpawnBigCoin(FName ItemID, int32 Count)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::Big))
	{
		return;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		ACPDispenser* Dispenser = PickRandomValidCeilingDispenser();
		if (!Dispenser)
		{
			continue;
		}

		if (ACPCoin* SpawnedCoin = Dispenser->DispenseCoinByID(ItemID))
		{
			// Big 코인이 CoinPusher의 Collision에 부딪혔을 때 ActiveWaveThrow()를 호출할 대상을 직접 알려줌
			SpawnedCoin->SetOwningCoinPusher(this);
			SpawnedCoin->SetCoinType(ECPCoinType::Big);
		}
	}
}

void ACPCoinPusher::SpawnMonsterCoin(FName ItemID, int32 Num)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::Monster))
	{
		return;
	}

	for (int32 Index = 0; Index < Num; ++Index)
	{
		ACPDispenser* Dispenser = PickRandomValidCeilingDispenser();
		if (!Dispenser)
		{
			continue;
		}

		if (ACPCoin* SpawnedCoin = Dispenser->DispenseCoinByID(ItemID))
		{
			SpawnedCoin->SetCoinType(ECPCoinType::Monster);
		}
	}
}

void ACPCoinPusher::ConvertActive(FName ItemID, int32 SpawnCount)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::Passive))
	{
		return;
	}

	if (ACPPassiveCoinConvertArea* ConvertArea = GetPassiveCoinConvertArea())
	{
		ConvertArea->ConvertActive(ItemID, SpawnCount);
	}
}

void ACPCoinPusher::HPConvertActive(FName ItemID, int32 SpawnCount)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::HP))
	{
		return;
	}

	if (ACPPassiveCoinConvertArea* ConvertArea = GetPassiveCoinConvertArea())
	{
		ConvertArea->HPConvertActive(ItemID, SpawnCount);
	}
}

void ACPCoinPusher::MonsterConvertActive(FName ItemID, int32 SpawnCount)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::Monster))
	{
		return;
	}

	if (ACPPassiveCoinConvertArea* ConvertArea = GetMonsterCoinConvertArea())
	{
		ConvertArea->MonsterConvertActive(ItemID, SpawnCount);
	}
}

void ACPCoinPusher::SpawnTower(FName ItemID, int32 SpawnCount)
{
	if (!ValidateItemCoinType(ItemID, ECPCoinType::CoinTower))
	{
		return;
	}

	if (ACPCoinTowerSpawner* CoinTowerSpawner = GetCoinTowerSpawner())
	{
		CoinTowerSpawner->SpawnTower(ItemID, SpawnCount);
	}
}

bool ACPCoinPusher::ValidateItemCoinType(FName ItemID, ECPCoinType ExpectedType) const
{
	if (!ItemDataTable)
	{
		return false;
	}

	const FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ACPCoinPusher::ValidateItemCoinType"));
	return Row && Row->CoinType == ExpectedType;
}

ACPDispenser* ACPCoinPusher::PickRandomValidCeilingDispenser() const
{
	TArray<ACPDispenser*> ValidCeilingDispensers;
	ValidCeilingDispensers.Reserve(CeilingDispenserComponents.Num());

	for (const TObjectPtr<UChildActorComponent>& CeilingComponent : CeilingDispenserComponents)
	{
		if (!CeilingComponent)
		{
			continue;
		}

		if (ACPDispenser* CeilingDispenser = Cast<ACPDispenser>(CeilingComponent->GetChildActor()))
		{
			ValidCeilingDispensers.Add(CeilingDispenser);
		}
	}

	if (ValidCeilingDispensers.Num() == 0)
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, ValidCeilingDispensers.Num() - 1);
	return ValidCeilingDispensers[RandomIndex];
}

void ACPCoinPusher::ActiveWaveThrow()
{
	if (CoinThrowAreaComponents.Num() == 0)
	{
		return;
	}

	WaveThrowIndex = 0;

	// 0초 뒤(즉시) 첫 CoinThrowArea를 활성화하고, 이후 WaveThrowInterval마다 다음 것을 순차적으로 활성화
	GetWorldTimerManager().SetTimer(WaveThrowTimerHandle, this, &ACPCoinPusher::HandleWaveThrowTick, WaveThrowInterval, true, 0.0f);
}

void ACPCoinPusher::HandleWaveThrowTick()
{
	if (!CoinThrowAreaComponents.IsValidIndex(WaveThrowIndex))
	{
		GetWorldTimerManager().ClearTimer(WaveThrowTimerHandle);
		return;
	}

	if (ACPCoinThrowArea* ThrowArea = GetCoinThrowArea(WaveThrowIndex))
	{
		ThrowArea->ActiveThrow();
	}

	++WaveThrowIndex;

	if (!CoinThrowAreaComponents.IsValidIndex(WaveThrowIndex))
	{
		GetWorldTimerManager().ClearTimer(WaveThrowTimerHandle);
	}
}
