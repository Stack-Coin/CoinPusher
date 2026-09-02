// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPCoinPusher.h"
#include "CPDispenser.h"
#include "CPDropZone.h"
#include "CPInput.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "TimerManager.h"

ACPCoinPusher::ACPCoinPusher()
{
	PrimaryActorTick.bCanEverTick = false;

	// 추가 박스 콜리전 + 그 자식으로 붙는 비주얼 메시. 용도는 BP에서 확장
	ExtraBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtraBox"));
	RootComponent = ExtraBox;

	ExtraBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	ExtraBox->SetCollisionProfileName(FName("Custom"));

	ExtraBoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExtraBoxMesh"));
	ExtraBoxMesh->SetupAttachment(ExtraBox);
	ExtraBoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 코인이 놓이는 바닥. RootComponent로 지정해 실제 충돌의 기준이 되도록 함.
	Floor = CreateDefaultSubobject<UBoxComponent>(TEXT("Floor"));
	Floor->SetBoxExtent(FVector(150.0f, 150.0f, 10.0f));
	Floor->SetCollisionProfileName(FName("BlockAllDynamic"));
	Floor->SetupAttachment(ExtraBox);

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

	UE_LOG(LogTemp, Warning, TEXT("%f"), CurrentHealth);
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
		return;
	}

	//천장 Dispenser 중 하나를 랜덤하게 골라 그쪽에서 Spawn되도록 위임
	const int32 RandomIndex = FMath::RandRange(0, ValidCeilingDispensers.Num() - 1);
	ValidCeilingDispensers[RandomIndex]->DispenseItemByID(ItemID, SpawnCount);
}
