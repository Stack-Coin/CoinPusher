// Fill out your copyright notice in the Description page of Project Settings.


#include "Nexus/CPGoddess.h"
#include "Nexus/CPNexus.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Nexus/CPUserWidget_NexusHpBar.h"

// Sets default values
ACPGoddess::ACPGoddess()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);

	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBar->SetupAttachment(CollisionSphere);
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	HpBar->SetWidgetClass(UCPUserWidget_NexusHpBar::StaticClass());
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(100.0f, 10.0f));
	HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void ACPGoddess::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundNexuses;
	UGameplayStatics::GetAllActorsOfClass(this, ACPNexus::StaticClass(), FoundNexuses);

	MaxHp = 0.f;
	//int32 NexusCount = 0;

	for (AActor* Actor : FoundNexuses)
	{
		if (ACPNexus* Nexus = Cast<ACPNexus>(Actor))
		{
			MaxHp += Nexus->GetMaxHp();
			Nexuses.Add(Nexus);
			Nexus->OnNexusHpChanged.AddUniqueDynamic(this, &ACPGoddess::OnNexusHpChanged);
			//++NexusCount;
		}
	}

	CurrentHp = MaxHp;

	HpBar->InitWidget();
	HpBarWidget = Cast<UCPUserWidget_NexusHpBar>(HpBar->GetUserWidgetObject());
	if (HpBarWidget)
	{
		HpBarWidget->SetMaxHp(MaxHp);
		UpdateHpBar();
	}
}

void ACPGoddess::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentHp > 0.f && CurrentHp < MaxHp && HealAmount > 0.f)
	{
		CurrentHp = FMath::Min(CurrentHp + HealAmount * DeltaTime, MaxHp);
		UpdateHpBar();
	}

	if (CurrentHp <= 0)
	{
		Dead();
	}
}

void ACPGoddess::UpdateHpBar()
{
	if (HpBarWidget)
	{
		HpBarWidget->UpdateHpBar(CurrentHp);
	}
}

void ACPGoddess::Dead()
{
	bIsDead = true;
	HpBar->SetHiddenInGame(true);

	OnGoddessDead.Broadcast();
}

void ACPGoddess::OnNexusHpChanged(ACPNexus* Nexus, float NewCurrentHp)
{
	if (bIsDead)
	{
		return;
	}

	float TotalHp = 0.f;
	for (const TObjectPtr<ACPNexus>& Nexus : Nexuses)
	{
		if (Nexus)
		{
			TotalHp += Nexus->GetCurrentHp();
		}
	}

	CurrentHp = FMath::Clamp(TotalHp, 0.f, MaxHp);
	UpdateHpBar();

	if (CurrentHp <= 0.f)
	{
		Dead();
	}
}
