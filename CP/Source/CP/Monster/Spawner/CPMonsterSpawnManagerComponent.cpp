// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawnManagerComponent.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Monster/CPMonsterBase.h"
#include "Monster/Boss/CPMonsterBoss.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

UCPMonsterSpawnManagerComponent::UCPMonsterSpawnManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SpawnerClass = ACPMonsterSpawner::StaticClass();
}

void UCPMonsterSpawnManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadAsset();
	ApplyRoundInfo(CurrentRound);

	CurrentWaveIndex = 0;
	StartWave(CurrentWaveIndex);
}

void UCPMonsterSpawnManagerComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UWorld* World = GetWorld())
	{
		for (FCPActiveSpawnJob& Job : ActiveJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}

		World->GetTimerManager().ClearTimer(WaveWaitTimer);
		World->GetTimerManager().ClearTimer(RoundWaitTimer);
	}
}

void UCPMonsterSpawnManagerComponent::LoadAsset()
{
	auto LoadMonsterClassIfMissing = [this](ECPMonsterType Type, const TCHAR* ClassPath)
		{
			if (MonsterClassByType.Contains(Type))
			{
				return;
			}

			// LoadClass는 ConstructorHelpers::FClassFinder와 달리 "_C" 접미사가 붙은 완전한 제너레이티드 클래스 경로가 필요
			if (UClass* LoadedClass = LoadClass<ACPMonsterBase>(nullptr, ClassPath))
			{
				MonsterClassByType.Add(Type, LoadedClass);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManagerComponent] MonsterClassByType 기본 경로를 찾지 못했습니다: %s"), ClassPath);
			}
		};

	LoadMonsterClassIfMissing(ECPMonsterType::Normal, TEXT("/Game/Monster/Blueprints/BP_Normal.BP_Normal_C"));
	LoadMonsterClassIfMissing(ECPMonsterType::Tanker, TEXT("/Game/Monster/Blueprints/BP_Tanker.BP_Tanker_C"));
	LoadMonsterClassIfMissing(ECPMonsterType::Ranged, TEXT("/Game/Monster/Blueprints/BP_Ranged.BP_Ranged_C"));
	LoadMonsterClassIfMissing(ECPMonsterType::Boss, TEXT("/Game/Monster/Blueprints/BP_BossMonster.BP_BossMonster_C"));

	if (!WaveInfoTable)
	{
		WaveInfoTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Monster/Data/DT_WaveInfo.DT_WaveInfo"));
	}
	if (!RoundInfoTable)
	{
		RoundInfoTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Monster/Data/DT_RoundInfo.DT_RoundInfo"));
	}
}

void UCPMonsterSpawnManagerComponent::ApplyRoundInfo(int32 InRound)
{
	const FCPRoundInfoRow* RoundInfo = FindRoundInfoRow(InRound);

	const int32 ResolvedSpawnerCount = RoundInfo ? RoundInfo->SpawnerCount : SpawnerCount;
	const float ResolvedSpawnerRadius = RoundInfo ? RoundInfo->SpawnerRadius : SpawnerRadius;

	if (!RoundInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManagerComponent] RoundInfoTable has no row for Round %d - falling back to component default SpawnerCount(%d)/SpawnerRadius(%.1f)."),
			InRound, ResolvedSpawnerCount, ResolvedSpawnerRadius);
	}

	if (UWorld* World = GetWorld())
	{
		for (FCPActiveSpawnJob& Job : ActiveJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}
	}
	ActiveJobs.Reset();

	CreateSpawnerRing(ResolvedSpawnerCount, ResolvedSpawnerRadius);
}

void UCPMonsterSpawnManagerComponent::CreateSpawnerRing(int32 InSpawnerCount, float InSpawnerRadius)
{
	// 기존 스포너가 있으면 새로 만들기 전에 전부 파괴
	for (const TPair<int32, TObjectPtr<ACPMonsterSpawner>>& Pair : SpawnersByIndex)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
	SpawnersByIndex.Reset();

	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || InSpawnerCount <= 0)
	{
		return;
	}

	const TSubclassOf<ACPMonsterSpawner> ClassToSpawn = SpawnerClass ? SpawnerClass.Get() : ACPMonsterSpawner::StaticClass();
	const FVector CenterLocation = Owner->GetActorLocation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < InSpawnerCount; ++Index)
	{
		// 360도를 InSpawnerCount만큼 균등 분할 - 몇 개로 설정하든 항상 고르게 배치됨
		const float AngleRad = FMath::DegreesToRadians(360.f * Index / InSpawnerCount);
		const FVector Offset = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f) * InSpawnerRadius;
		FVector SpawnLocation = CenterLocation + Offset;

		// Z는 일단 플레이어(오너) 기준으로 잡히는데, 지형이 평평하지 않으면 이 위치의 실제 지면 높이랑
		// 다를 수 있어서(경사/고저차) 스포너를 놓기 전에 바로 아래로 트레이스해서 실제 바닥 높이로 보정함
		{
			constexpr float TraceUp = 500.f;
			constexpr float TraceDown = 2000.f;
			const FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, TraceUp);
			const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, TraceDown);

			FHitResult GroundHit;
			FCollisionQueryParams TraceParams(NAME_None, false, Owner);
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
			{
				SpawnLocation.Z = GroundHit.Location.Z;
			}
			// 트레이스가 아무것도 못 맞히면(바닥이 없거나 채널이 안 맞으면) 기존처럼 플레이어 Z를 그대로 씀
		}

		// 오너(플레이어) 쪽을 바라보도록 회전 - 스폰 즉시 몬스터가 플레이어 방향을 향하게 됨
		const FRotator SpawnRotation = (CenterLocation - SpawnLocation).Rotation();

		if (ACPMonsterSpawner* Spawner = GetWorld()->SpawnActor<ACPMonsterSpawner>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams))
		{
			Spawner->Tags.Add(*FString::Printf(TEXT("Spawner%d"), Index));

			// 오너에 부착 - 오너(플레이어)가 움직이면 이 반경 배치가 그대로 따라감
			Spawner->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);

			SpawnersByIndex.Add(Index, Spawner);
		}
	}
}

void UCPMonsterSpawnManagerComponent::StartWave(int32 InWaveIndex)
{
	const int32 WaveNumber = InWaveIndex + 1; // 데이터 테이블의 Wave 컬럼은 1부터 시작

	TArray<FCPSpawnWaveEntryRow*> Entries;
	GetWaveEntries(CurrentRound, WaveNumber, Entries);

	if (Entries.IsEmpty())
	{
		// 이번 라운드에 더 이상 웨이브가 없으므로 보스 웨이브로 전환
		StartBossWave();
		return;
	}

	CurrentWaveIndex = InWaveIndex;
	CurrentPhase = ECPWavePhase::Spawning;

	ActiveJobs.Reset();
	WaveSpawnTotalSeconds = 0.f;
	WaveStartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	CurrentWaveEndWaitTime = Entries[0]->WaveEndWaitTime; // 같은 웨이브의 행들은 동일한 값을 넣는다고 가정

	for (const FCPSpawnWaveEntryRow* Entry : Entries)
	{
		FCPActiveSpawnJob Job;
		Job.MonsterClass = MonsterClassByType.FindRef(Entry->MonsterType);
		Job.CountPerSpawnPoint = Entry->CountPerSpawnPoint;
		Job.MonstersPerSpawn = Entry->MonstersPerSpawn;
		Job.SpawnRowSpacingY = Entry->SpawnRowSpacingY;
		Job.TargetSpawners = ResolveValidSpawners(Entry->SpawnerIndices);

		if (!IsValid(Job.MonsterClass) || Job.TargetSpawners.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManagerComponent] Skipping empty/invalid SpawnWaveEntryRow (Round %d, Wave %d)."), CurrentRound, WaveNumber);
			continue;
		}

		WaveSpawnTotalSeconds = FMath::Max(WaveSpawnTotalSeconds, Entry->SpawnInterval * Entry->CountPerSpawnPoint);

		const int32 JobIndex = ActiveJobs.Add(Job);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("HandleSpawnJobTick"), JobIndex);
		GetWorld()->GetTimerManager().SetTimer(ActiveJobs[JobIndex].TimerHandle, TimerDelegate, Entry->SpawnInterval, true, 0.f);
	}

	// 이번 웨이브에 유효한 스폰 규칙이 하나도 없다면 곧바로 다음 단계로 진행
	if (ActiveJobs.IsEmpty())
	{
		EndWave();
	}
}

void UCPMonsterSpawnManagerComponent::GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPSpawnWaveEntryRow*>& OutEntries) const
{
	OutEntries.Reset();

	if (!WaveInfoTable)
	{
		return;
	}

	TArray<FCPSpawnWaveEntryRow*> AllRows;
	WaveInfoTable->GetAllRows<FCPSpawnWaveEntryRow>(TEXT("UCPMonsterSpawnManagerComponent::GetWaveEntries"), AllRows);

	for (FCPSpawnWaveEntryRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound && Row->Wave == InWave)
		{
			OutEntries.Add(Row);
		}
	}
}

void UCPMonsterSpawnManagerComponent::StartBossWave()
{
	CurrentPhase = ECPWavePhase::RoundWait;

	const FCPRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	const float WaitTime = RoundInfo ? RoundInfo->RoundEndWaitTime : 0.f;

	GetWorld()->GetTimerManager().SetTimer(RoundWaitTimer, this, &UCPMonsterSpawnManagerComponent::SpawnBoss, WaitTime, false);
}

void UCPMonsterSpawnManagerComponent::SpawnBoss()
{
	const FCPRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	if (!RoundInfo)
	{
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	ACPMonsterSpawner* BossSpawner = SpawnersByIndex.FindRef(RoundInfo->BossSpawnerIndex);
	if (!IsValid(BossSpawner))
	{
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	if (!IsSpawnerLocationValid(BossSpawner->GetActorLocation()))
	{
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	TSubclassOf<ACPMonsterBase> BossClass = MonsterClassByType.FindRef(RoundInfo->BossMonsterType);
	if (!IsValid(BossClass))
	{
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	ACPMonsterBase* SpawnedBoss = BossSpawner->SpawnMonsterRow(BossClass, 1, 0.f, GetWaveCount());
	if (SpawnedBoss)
	{
		ActiveBoss = SpawnedBoss;
		SpawnedBoss->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBossDied);

		// RoundInfoTable 행에 담긴 Boss 전용 값(포효 임계치/슬램 쿨타임)을 스폰된 인스턴스에 적용
		if (ACPMonsterBoss* Boss = Cast<ACPMonsterBoss>(SpawnedBoss))
		{
			Boss->ApplyBossWaveStat(RoundInfo->RoarHealthPercentThreshold, RoundInfo->SlamCooldown);
		}
	}
	else
	{
		CurrentPhase = ECPWavePhase::Finished;
	}
}

TArray<TObjectPtr<ACPMonsterSpawner>> UCPMonsterSpawnManagerComponent::ResolveValidSpawners(const TArray<int32>& InIndices) const
{
	TArray<TObjectPtr<ACPMonsterSpawner>> Result;
	Result.Reserve(InIndices.Num());

	for (const int32 Index : InIndices)
	{
		ACPMonsterSpawner* Spawner = SpawnersByIndex.FindRef(Index);
		if (!IsValid(Spawner))
		{
			UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManagerComponent] SpawnerIndex %d - 현재 스포너 범위(0~%d)를 벗어나 무시합니다."),
				Index, SpawnersByIndex.Num() - 1);
			continue;
		}

		if (!IsSpawnerLocationValid(Spawner->GetActorLocation()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManagerComponent] SpawnerIndex %d - 맵/네브메시 밖으로 판단되어 이번 웨이브에서 제외합니다."), Index);
			continue;
		}

		Result.Add(Spawner);
	}

	return Result;
}

bool UCPMonsterSpawnManagerComponent::IsSpawnerLocationValid(const FVector& InLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		// 네브메시가 아예 없는(빌드 안 된) 테스트 레벨에서는 검증을 생략하고 항상 유효 처리
		return true;
	}

	FNavLocation OutNavLocation;
	constexpr float ProjectionExtentXY = 500.f;
	constexpr float ProjectionExtentZ = 500.f;
	return NavSys->ProjectPointToNavigation(InLocation, OutNavLocation, FVector(ProjectionExtentXY, ProjectionExtentXY, ProjectionExtentZ));
}

void UCPMonsterSpawnManagerComponent::EndWave()
{
	CurrentPhase = ECPWavePhase::WaveWait;

	GetWorld()->GetTimerManager().SetTimer(WaveWaitTimer, this, &UCPMonsterSpawnManagerComponent::HandleWaveWaitFinished, CurrentWaveEndWaitTime, false);
}

void UCPMonsterSpawnManagerComponent::HandleSpawnJobTick(int32 JobIndex)
{
	if (!ActiveJobs.IsValidIndex(JobIndex))
	{
		return;
	}

	FCPActiveSpawnJob& Job = ActiveJobs[JobIndex];

	for (ACPMonsterSpawner* Spawner : Job.TargetSpawners)
	{
		if (IsValid(Spawner))
		{
			Spawner->SpawnMonsterRow(Job.MonsterClass, Job.MonstersPerSpawn, Job.SpawnRowSpacingY, CurrentWaveIndex + 1);
		}
	}

	++Job.SpawnedCount;

	if (Job.SpawnedCount < Job.CountPerSpawnPoint)
	{
		return; // 이 Job은 아직 안 끝남 - 다음 틱을 기다림
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Job.TimerHandle);
	}

	// 이 Job은 끝났으니, 다른 Job들도 전부 끝났는지 확인 (예전엔 CheckWaveComplete()로 분리했던 부분)
	for (const FCPActiveSpawnJob& OtherJob : ActiveJobs)
	{
		if (OtherJob.SpawnedCount < OtherJob.CountPerSpawnPoint)
		{
			return; // 아직 안 끝난 Job이 있음
		}
	}

	EndWave();
}

void UCPMonsterSpawnManagerComponent::HandleWaveWaitFinished()
{
	StartWave(CurrentWaveIndex + 1);
}

void UCPMonsterSpawnManagerComponent::HandleBossDied()
{
	// 기획서: 보스 사망시 나머지 일반 몬스터도 즉시 제거
	TArray<AActor*> RemainingMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACPMonsterBase::StaticClass(), RemainingMonsters);

	for (AActor* Actor : RemainingMonsters)
	{
		if (IsValid(Actor) && Actor != ActiveBoss.Get())
		{
			Actor->Destroy();
		}
	}

	ActiveBoss = nullptr;
	CurrentPhase = ECPWavePhase::Finished;

	// 프로토타입: 라운드가 1개뿐이라 여기서 종료합니다.
	// 라운드가 여러 개가 되면: ++CurrentRound; ApplyRoundInfo(CurrentRound); CurrentWaveIndex = 0;
	// StartWave(0); 순서로 여기서 호출하면 됩니다 (ApplyRoundInfo가 새 라운드의 스포너 링을
	// 재생성하고 나서 웨이브를 시작해야 하므로 이 순서가 중요합니다).
}

FCPRoundInfoRow* UCPMonsterSpawnManagerComponent::FindRoundInfoRow(int32 InRound) const
{
	if (!RoundInfoTable)
	{
		return nullptr;
	}

	TArray<FCPRoundInfoRow*> AllRows;
	RoundInfoTable->GetAllRows<FCPRoundInfoRow>(TEXT("UCPMonsterSpawnManagerComponent::FindRoundInfoRow"), AllRows);

	for (FCPRoundInfoRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound)
		{
			return Row;
		}
	}

	return nullptr;
}

int32 UCPMonsterSpawnManagerComponent::GetWaveCount() const
{
	if (!WaveInfoTable)
	{
		return 0;
	}

	TArray<FCPSpawnWaveEntryRow*> AllRows;
	WaveInfoTable->GetAllRows<FCPSpawnWaveEntryRow>(TEXT("UCPMonsterSpawnManagerComponent::GetWaveCount"), AllRows);

	int32 MaxWave = 0;
	for (const FCPSpawnWaveEntryRow* Row : AllRows)
	{
		if (Row && Row->Round == CurrentRound)
		{
			MaxWave = FMath::Max(MaxWave, Row->Wave);
		}
	}

	return MaxWave;
}

int32 UCPMonsterSpawnManagerComponent::GetSpawnElapsedSeconds() const
{
	if (!GetWorld())
	{
		return 0;
	}

	const float Elapsed = FMath::Clamp(GetWorld()->GetTimeSeconds() - WaveStartWorldTime, 0.f, WaveSpawnTotalSeconds);
	return FMath::RoundToInt(Elapsed);
}

float UCPMonsterSpawnManagerComponent::GetRoundEndWaitSeconds() const
{
	const FCPRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	return RoundInfo ? RoundInfo->RoundEndWaitTime : 0.f;
}

float UCPMonsterSpawnManagerComponent::GetWaveWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(WaveWaitTimer));
}

float UCPMonsterSpawnManagerComponent::GetRoundWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(RoundWaitTimer));
}
