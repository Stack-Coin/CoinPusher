// Fill out your copyright notice in the Description page of Project Settings.


#include "Nexus/CPNexus.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Nexus/CPUserWidget_NexusHpBar.h"
#include "Player/CPInteractor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/CPMonsterBase.h"
#include "Monster/Task/CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
ACPNexus::ACPNexus()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(150.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPNexus::OnCollisionSphereBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ACPNexus::OnCollisionSphereEndOverlap);

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

void ACPNexus::BeginPlay()
{
	Super::BeginPlay();

	CurrentHp = FMath::Clamp(CurrentHp, 0.f, MaxHp);
	HpBar->InitWidget();
	HpBarWidget = Cast<UCPUserWidget_NexusHpBar>(HpBar->GetUserWidgetObject());
	if (HpBarWidget)
	{
		HpBarWidget->SetMaxHp(MaxHp);
		UpdateHpBar();
	}
}

void ACPNexus::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentHp > 0.f && CurrentHp < MaxHp && HealAmount > 0.f)
	{
		CurrentHp = FMath::Min(CurrentHp + HealAmount * DeltaTime, MaxHp);
		UpdateHpBar();
		OnNexusHpChanged.Broadcast(this, CurrentHp);
	}
}

void ACPNexus::UpdateHpBar()
{
	if (HpBarWidget)
	{
		HpBarWidget->UpdateHpBar(CurrentHp);
	}
}

void ACPNexus::Interact(AActor* Interactor)
{
	OnInteracted.Broadcast(Interactor);

	BP_OnInteracted(Interactor);
}

void ACPNexus::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->RegisterInteractable(this);
	}
}

void ACPNexus::OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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

void ACPNexus::Dead()
{
	HpBar->SetHiddenInGame(true);

	// 아주 간단한 버전 // 단, 경로 테스트 목적이므로 여기서 여신상의 체력 로직은 수정하지 않ㅇㅡㅁ
	//여신상 배치 변경해야 함.TODO.
	
	// BT가 다른 가까운 Nexus를 다시 찾게
	TArray<AActor*> Monsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACPMonsterBase::StaticClass(), Monsters);

	for (AActor* Actor : Monsters)
	{
		ACPMonsterBase* Monster = Cast<ACPMonsterBase>(Actor);
		if (!Monster)
		{
			continue;
		}

		AAIController* AIController = Cast<AAIController>(Monster->GetController());
		UBlackboardComponent* MonsterBlackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
		if (!MonsterBlackboard)
		{
			continue;
		}

		if (MonsterBlackboard->GetValueAsObject(BBKEY_NEXUS) == this)
		{
			MonsterBlackboard->SetValueAsObject(BBKEY_NEXUS, nullptr);
		}
	}

	// Monster 관련 트레이스
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);

	// 월드 기준 왼쪽(Y < 0)에 있으면 자신 기준 오른쪽에, 오른쪽에 있으면 자신 기준 왼쪽에 새 Nexus를 스 // (맵 레이아웃이 Y가 아니라 X가 좌우 축이라면 GetActorLocation().Y를 X로)
	const bool bIsOnWorldLeft = GetActorLocation().Y < 0.f;
	const FVector SpawnDirection = bIsOnWorldLeft ? GetActorRightVector() : -GetActorRightVector();
	const FVector SpawnLocation = GetActorLocation() + SpawnDirection * RespawnOffsetDistance;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	GetWorld()->SpawnActor<ACPNexus>(GetClass(), FTransform(GetActorRotation(), SpawnLocation), SpawnParams);

	Destroy();
}

float ACPNexus::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float Damage = FMath::Clamp(DamageAmount, 0.f, CurrentHp);
	CurrentHp -= Damage;
	UpdateHpBar();

	OnNexusHpChanged.Broadcast(this, CurrentHp);

	if (CurrentHp <= 0)
	{
		Dead();
	}

	return Damage;
}
