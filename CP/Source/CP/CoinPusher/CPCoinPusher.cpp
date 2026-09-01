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

	// 코인이 놓이는 바닥. RootComponent로 지정해 실제 충돌의 기준이 되도록 함.
	RootComponent = Floor = CreateDefaultSubobject<UBoxComponent>(TEXT("Floor"));
	Floor->SetBoxExtent(FVector(150.0f, 150.0f, 10.0f));
	Floor->SetCollisionProfileName(FName("BlockAllDynamic"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	LeftWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWall"));
	LeftWall->SetupAttachment(RootComponent);
	LeftWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	LeftWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	RightWall = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWall"));
	RightWall->SetupAttachment(RootComponent);
	RightWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	RightWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	BackWall = CreateDefaultSubobject<UBoxComponent>(TEXT("BackWall"));
	BackWall->SetupAttachment(RootComponent);
	BackWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	BackWall->SetCollisionProfileName(FName("BlockAllDynamic"));

	FrontWall = CreateDefaultSubobject<UBoxComponent>(TEXT("FrontWall"));
	FrontWall->SetupAttachment(RootComponent);
	FrontWall->SetBoxExtent(FVector(150.0f, 10.0f, 100.0f));
	FrontWall->SetCollisionProfileName(FName("BlockAllDynamic"));


	PusherComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("PusherComponent"));
	PusherComponent->SetupAttachment(RootComponent);

	DispenserComponentA = CreateDefaultSubobject<UChildActorComponent>(TEXT("DispenserComponentA"));
	DispenserComponentA->SetupAttachment(RootComponent);

	DispenserComponentB = CreateDefaultSubobject<UChildActorComponent>(TEXT("DispenserComponentB"));
	DispenserComponentB->SetupAttachment(RootComponent);

	// 천장에서 물건을 뿌리는 Dispenser 5개. 위치/발사 설정과 Item Class는 BP에서 조정
	CeilingDispenserComponents.SetNum(5);
	for (int32 Index = 0; Index < CeilingDispenserComponents.Num(); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("CeilingDispenserComponent%d"), Index));
		UChildActorComponent* CeilingDispenserComponent = CreateDefaultSubobject<UChildActorComponent>(ComponentName);
		CeilingDispenserComponent->SetupAttachment(RootComponent);
		CeilingDispenserComponents[Index] = CeilingDispenserComponent;
	}

	DropZoneComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("DropZoneComponent"));
	DropZoneComponent->SetupAttachment(RootComponent);

	//Overlap 방식은 추후 변경
	// react to enemies making contact with the machine
	OnActorBeginOverlap.AddDynamic(this, &ACPCoinPusher::OnActorOverlapBegin);
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

void ACPCoinPusher::ApplyDamage(float Damage, AActor* DamageCauser)
{
	if (bIsDestroyed || Damage <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);

	OnDamaged.Broadcast(Damage, DamageCauser);

	if (CurrentHealth <= 0.0f)
	{
		HandleDestroyed();
	}
}

//Overlap방식은 변경
void ACPCoinPusher::OnActorOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	// does the overlapping actor count as an enemy?
	if (OtherActor && OtherActor->ActorHasTag(EnemyActorTag))
	{
		ApplyDamage(EnemyContactDamage, OtherActor);
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
