// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPItem.h"
#include "CPDropZone.h"
#include "CPCoinPusher.h"
#include "CPCoinPusherViewCaptureComponent.h"
#include "Datatables/CPItemData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

ACPItem::ACPItem()
{
	// ImageMesh(빌보드)가 보이는 동안만 카메라를 바라보도록 Tick을 켜야 하므로, 기본은 꺼둔 채로
	// ApplyItemData()가 필요할 때만 SetActorTickEnabled(true/false)로 토글한다 (ACPDropZone의
	// 콤보 게이지 Tick과 동일한 패턴)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// create the collision sphere, root component
	RootComponent = CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetCollisionProfileName(FName("BlockAllDynamic"));

	// enable physics so the item reacts to the Pusher and gravity, same as a coin
	CollisionSphere->SetSimulatePhysics(true);

	// disable navigation relevance so items don't affect NavMesh generation
	CollisionSphere->bNavigationRelevant = false;

	// create the visual mesh, purely cosmetic
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->bNavigationRelevant = false;

	// billboard-style flat image alternative to Mesh - centered on Mesh's "ImagePoint" socket (defined
	// on the StaticMesh asset assigned to Mesh) so it can be positioned per-mesh in the mesh editor
	// rather than hardcoded here. Hidden until ApplyItemData() finds an ItemImage
	ImageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImageMesh"));
	ImageMesh->SetupAttachment(Mesh, TEXT("ImagePoint"));
	ImageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImageMesh->bNavigationRelevant = false;
	ImageMesh->SetVisibility(false);
}

void ACPItem::BeginPlay()
{
	Super::BeginPlay();

	ApplyItemData();
}

void ACPItem::ApplyItemData()
{
	if (!ItemDataTable)
	{
		return;
	}

	const FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemId, TEXT("ACPItem::ApplyItemData"));
	if (!Row)
	{
		return;
	}

	if (Row->ItemMesh && Mesh)
	{
		Mesh->SetStaticMesh(Row->ItemMesh);
	}

	if (Row->ItemMaterial && Mesh)
	{
		Mesh->SetMaterial(0, Row->ItemMaterial);
	}

	if (Row->ItemMaterial2 && Mesh)
	{
		Mesh->SetMaterial(1, Row->ItemMaterial2);
	}

	if (!ImageMesh)
	{
		return;
	}

	if (!Row->ItemImage)
	{
		ImageMesh->SetVisibility(false);
		SetActorTickEnabled(false);
		return;
	}

	if (!ImageMaterialInstance && BillboardMaterial)
	{
		ImageMaterialInstance = UMaterialInstanceDynamic::Create(BillboardMaterial, this);
		ImageMesh->SetMaterial(0, ImageMaterialInstance);
	}

	if (ImageMaterialInstance)
	{
		ImageMaterialInstance->SetTextureParameterValue(ItemTextureParameterName, Row->ItemImage);
		ImageMesh->SetVisibility(true);
		SetActorTickEnabled(true);
	}
}

void ACPItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ImageMesh || !ImageMesh->IsVisible())
	{
		return;
	}

	UCPCoinPusherViewCaptureComponent* CaptureComponent = GetCaptureComponent();
	if (!CaptureComponent)
	{
		return;
	}

	const FVector ToCamera = CaptureComponent->GetComponentLocation() - ImageMesh->GetComponentLocation();
	if (ToCamera.IsNearlyZero())
	{
		return;
	}

	ImageMesh->SetWorldRotation(ToCamera.Rotation() + BillboardRotationOffset);
}

ACPCoinPusher* ACPItem::GetCoinPusher() const
{
	if (ACPCoinPusher* Cached = CachedCoinPusher.Get())
	{
		return Cached;
	}

	CachedCoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass()));
	return CachedCoinPusher.Get();
}

UCPCoinPusherViewCaptureComponent* ACPItem::GetCaptureComponent() const
{
	if (UCPCoinPusherViewCaptureComponent* Cached = CachedCaptureComponent.Get())
	{
		return Cached;
	}

	ACPCoinPusher* CoinPusher = GetCoinPusher();
	if (!CoinPusher)
	{
		return nullptr;
	}

	CachedCaptureComponent = CoinPusher->GetViewCaptureComponent();
	return CachedCaptureComponent.Get();
}

void ACPItem::SetItemId(FName NewItemId)
{
	if (ItemId == NewItemId)
	{
		return;
	}

	ItemId = NewItemId;
	ApplyItemData();
}

bool ACPItem::Collect()
{
	// only process this once
	if (bCollected)
	{
		return false;
	}

	bCollected = true;

	// call the BP handler to play effects, etc.
	BP_OnCollected();

	Destroy();

	return true;
}

void ACPItem::OnDroppedInZone(ACPDropZone* DropZone)
{
	// DropZone already read GetItemCode() and called RecordCollectedItem() directly at the overlap - just
	// clean ourselves up, no need to report back to it
	Collect();
}
