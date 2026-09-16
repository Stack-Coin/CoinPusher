// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPWorldItem.h"
#include "Player/CPInteractor.h"
#include "Player/CPItemInventory.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/CPDebugCollisionShapeComponent.h"
#include "TimerManager.h"

ACPWorldItem::ACPWorldItem()
{
	PrimaryActorTick.bCanEverTick = true;

	// 물리 충돌 담당 루트 - 다른 ACPWorldItem의 CollisionBody와만 겹치면 서로 밀어내되, 중력 없이
	// XY 평면 위에서만 미끄러지고 충돌로 인해 임의 회전하지 않도록 잠근다 (ItemMesh의 스크립트
	// 애니메이션과 분리). ECC_GameTraceChannel9("WorldItem", DefaultEngine.ini의 Object Channels에
	// 이미 등록돼 있음)를 오브젝트 타입으로 써서 플레이어/월드/몬스터 등 다른 모든 것과는 부딪히지
	// 않고 같은 ACPWorldItem끼리만 물리적으로 충돌한다
	CollisionBody = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionBody"));
	SetRootComponent(CollisionBody);
	CollisionBody->InitSphereRadius(50.0f);
	CollisionBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBody->SetCollisionObjectType(ECC_GameTraceChannel9);
	CollisionBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBody->SetCollisionResponseToChannel(ECC_GameTraceChannel9, ECR_Block);
	CollisionBody->SetSimulatePhysics(true);
	CollisionBody->SetEnableGravity(false);
	// bLock* 플래그는 DOFMode가 SixDOF일 때만 실제로 읽힌다 - CustomPlane은 CustomDOFPlaneNormal
	// 벡터로 평면을 지정하는 별개 모드라 bLock* 플래그를 무시한다(이름이 헷갈리지만 "축별 개별
	// 잠금 가능"이라는 뜻의 SixDOF가 맞는 값). 기본값 Default일 땐 프로젝트 설정을 따라가 버려 역시 무시됨
	CollisionBody->BodyInstance.DOFMode = EDOFMode::SixDOF;
	CollisionBody->BodyInstance.bLockZTranslation = true;
	CollisionBody->BodyInstance.bLockXRotation = true;
	CollisionBody->BodyInstance.bLockYRotation = true;
	CollisionBody->BodyInstance.bLockZRotation = true;

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(CollisionBody);
	InteractionRange->InitSphereRadius(150.0f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	DebugPickupShape = CreateDefaultSubobject<UCPDebugCollisionShapeComponent>(TEXT("DebugPickupShape"));
	DebugPickupShape->Category = ECPDebugCollisionCategory::ItemPickup;
	DebugPickupShape->ShapeColor = FColor::Cyan;
	DebugPickupShape->SetTargetComponent(InteractionRange);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(CollisionBody);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &ACPWorldItem::OnInteractionRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &ACPWorldItem::OnInteractionRangeEndOverlap);
}

void ACPWorldItem::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMesh)
	{
		StartRelativeLocation = ItemMesh->GetRelativeLocation();
		StartRelativeRotation = ItemMesh->GetRelativeRotation();
	}

	if (LifetimeSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(LifetimeExpireTimerHandle, this, &ACPWorldItem::HandleLifetimeExpired, LifetimeSeconds, false);
	}
}

void ACPWorldItem::HandleLifetimeExpired()
{
	Destroy();
}

void ACPWorldItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ItemMesh)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const FRotator NewRotation = StartRelativeRotation + FRotator(0.0f, ElapsedTime * RotationSpeed, 0.0f);
	ItemMesh->SetRelativeRotation(NewRotation);

	// -BobDistance/2 ~ +BobDistance/2 사이를 사인파로 왕복 (ACPPusher와 동일한 절대 위치 재계산 방식)
	const float ZOffset = FMath::Sin(ElapsedTime * BobSpeed * 2.0f * PI) * (BobDistance * 0.5f);
	ItemMesh->SetRelativeLocation(StartRelativeLocation + FVector(0.0f, 0.0f, ZOffset));
}

void ACPWorldItem::OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->RegisterInteractable(this);
	}
}

void ACPWorldItem::OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->UnregisterInteractable(this);
	}
}

void ACPWorldItem::Interact(AActor* Interactor)
{
	if (bCollected)
	{
		return;
	}

	ICPItemInventory* Inventory = Cast<ICPItemInventory>(Interactor);
	if (!Inventory)
	{
		return;
	}

	bCollected = true;

	Inventory->AddOwnedItem(ItemData);

	Destroy();
}

FText ACPWorldItem::GetInteractableDisplayName() const
{
	return ItemData.ItemName;
}
