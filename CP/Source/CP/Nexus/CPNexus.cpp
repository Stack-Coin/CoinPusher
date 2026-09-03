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

// 넥서스가 몇 개든, 게임 전체에서 재스폰 가능한 총 횟수를 공유하는 스태틱 변수 (기본 2회)
int32 ACPNexus::GlobalRemainingRespawns = 2;

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
	// Destroy()하지 않고 메시를 남겨두기 때문에, 죽은 뒤에도 TakeDamage가 계속 들어오면 Dead()가 매번 다시 실행됩니다.
	// 그러면 아래 재스폰 로직이 맞을 때마다 반복 실행되어 GlobalRemainingRespawns가 의도한 것보다 훨씬 많이 줄고
	// 넥서스가 여러 개 더 스폰되는 버그로 이어지므로, 한 번만 실행되도록 막습니다.
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	HpBar->SetHiddenInGame(true);

	// 이 Nexus를 블랙보드 타겟으로 물고 있던 몬스터들은 Nexus 키를 비워서, BT가 다른 가까운 Nexus를 다시 찾게 합니다.
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

	// Monster 관련 트레이스(공격 판정, 타겟 탐지 등)가 곧 사라질 이 Nexus를 더 이상 대상으로 보지 않도록 무시 처리
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);

	// 넥서스가 몇 개든, 게임 전체에서 재스폰 가능한 총 횟수는 스태틱 변수(GlobalRemainingRespawns)로 관리합니다.
	// (개별 넥서스마다 재스폰 횟수를 따로 주면, 넥서스가 여러 개일 때 총합이 의도한 횟수를 넘어가 버립니다)
	if (GlobalRemainingRespawns > 0)
	{
		--GlobalRemainingRespawns;

		// 아주 간단한 버전: 월드 기준 왼쪽(Y < 0)에 있으면 자신 기준 오른쪽에, 오른쪽에 있으면 자신 기준 왼쪽에 새 Nexus를 스폰합니다.
		// (맵 레이아웃이 Y가 아니라 X가 좌우 축이라면 GetActorLocation().Y를 X로 바꿔주세요)
		const bool bIsOnWorldLeft = GetActorLocation().Y < 0.f;
		const FVector SpawnDirection = bIsOnWorldLeft ? GetActorRightVector() : -GetActorRightVector();
		const FVector SpawnLocation = GetActorLocation() + SpawnDirection * RespawnOffsetDistance;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		GetWorld()->SpawnActor<ACPNexus>(GetClass(), FTransform(GetActorRotation(), SpawnLocation), SpawnParams);
	}

	// KohMS // 요청에 따라 죽어도 Destroy()하지 않습니다 - 메시는 계속 화면에 남아있어야 합니다.
}

float ACPNexus::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 이미 죽은 넥서스는 더 이상 데미지를 받지 않습니다 (메시는 남아있지만 기능적으로는 파괴된 상태).
	if (bIsDead)
	{
		return 0.f;
	}

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
