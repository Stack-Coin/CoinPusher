// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawnManagerComponent.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Monster/CPMonsterBase.h"
#include "Monster/Boss/CPMonsterBoss.h"
#include "Monster/Bomb/CPMonsterBomb.h"
#include "Monster/Pool/CPMonsterPoolSubsystem.h"
#include "Player/CPPlayerCharacter.h"
#include "Player/CPTopDownPlayerController.h"
#include "UI/CPInGameWidget.h"
#include "CoinPusher/CPCoinPusher.h"
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

void UCPMonsterSpawnManagerComponent::LoadDefaultMonsterClasses()
{
	FSoftClassPath Normal(TEXT("/Game/Monster/Blueprints/BP_Normal.BP_Normal_C"));
	UClass* NormalClass =  Normal.TryLoadClass<ACPMonsterBase>();
	if (NormalClass)
	{
		MonsterClassByType.Add(ECPMonsterType::Normal, NormalClass);
	}

	FSoftClassPath Tanker(TEXT("/Game/Monster/Blueprints/BP_Tanker.BP_Tanker_C"));
	UClass* TankerClass = Tanker.TryLoadClass<ACPMonsterBase>();
	if (TankerClass)
	{
		MonsterClassByType.Add(ECPMonsterType::Tanker, TankerClass);
	}

	FSoftClassPath Ranged(TEXT("/Game/Monster/Blueprints/BP_Ranged.BP_Ranged_C"));
	UClass* RangedClass = Ranged.TryLoadClass<ACPMonsterBase>();
	if (RangedClass)
	{
		MonsterClassByType.Add(ECPMonsterType::Ranged, RangedClass);
	}

	FSoftClassPath Boss(TEXT("/Game/Monster/Blueprints/BP_BossMonster.BP_BossMonster_C"));
	UClass* BossClass = Boss.TryLoadClass<ACPMonsterBase>();
	if (BossClass)
	{
		MonsterClassByType.Add(ECPMonsterType::Boss, BossClass);
	}

	FSoftClassPath Bomb(TEXT("/Game/Monster/Blueprints/BP_Bomb.BP_Bomb_C"));
	UClass* BombClass = Bomb.TryLoadClass<ACPMonsterBase>();
	if (BombClass)
	{
		MonsterClassByType.Add(ECPMonsterType::Bomb, BombClass);
	}
}

void UCPMonsterSpawnManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadDefaultMonsterClasses();

	ApplyRoundInfo(CurrentRound);

	// CoinPusher가 DropZone에 몬스터 코인을 떨어뜨릴 때(HandleDropZoneItemDropped) 이걸로 보상 몬스터를
	// 스폰함 - GetCoinPusher()는 GetOwner()(Player)를 거치므로, 레벨에 배치된 CoinPusher가 이미
	// PostInitializeComponents()에서 Player에 채워진 뒤인 지금 시점(컴포넌트 BeginPlay)이면 안전함
	if (ACPCoinPusher* CoinPusher = GetCoinPusher())
	{
		if (FOnCPDropZoneDropped* DropZoneDelegate = CoinPusher->GetDropZoneDroppedDelegate())
		{
			DropZoneDelegate->AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleDropZoneItemDropped);
		}
	}

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
		for (FCPActiveSpawnJob& Job : RoundMobJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}

		World->GetTimerManager().ClearTimer(WaveWaitTimer);
		World->GetTimerManager().ClearTimer(RoundWaitTimer);
	}
}

void UCPMonsterSpawnManagerComponent::ApplyRoundInfo(int32 InRound)
{
	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(InRound);

	const int32 ResolvedSpawnerCount = RoundInfo ? RoundInfo->SpawnerCount : SpawnerCount;
	const float RawSpawnerRadius = RoundInfo ? RoundInfo->SpawnerRadius : SpawnerRadius;

	// ponytail: 아레나 NavMesh(8각형) 기준 안전 상한. DT_RoundInfo의 SpawnerRadius가 이걸 넘으면
	// 스포너가 NavMesh 밖에 놓여 보스/라운드몹이 안 보이는 버그 재현 가능. 맵/NavMesh 형태 바뀌면 재계산 필요.
	constexpr float MaxSafeSpawnerRadius = 1700.f;
	const float ResolvedSpawnerRadius = FMath::Min(RawSpawnerRadius, MaxSafeSpawnerRadius);

	if (RawSpawnerRadius > MaxSafeSpawnerRadius)
	{

	}

	if (!RoundInfo)
	{

	}

	if (UWorld* World = GetWorld())
	{
		for (FCPActiveSpawnJob& Job : ActiveJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}
	}
	ActiveJobs.Reset();

	// 이번 라운드 누적 생존 몬스터 수를 새로 세기 시작
	WaveAliveMonsterCount = 0;

	CreateSpawnerRing(ResolvedSpawnerCount, ResolvedSpawnerRadius);

	WarmUpMonsterPools(InRound, RoundInfo);
}

void UCPMonsterSpawnManagerComponent::WarmUpMonsterPools(int32 InRound, const FCPMonsterRoundInfoRow* InRoundInfo) const
{
	UCPMonsterPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UCPMonsterPoolSubsystem>() : nullptr;
	if (!Pool || !WaveInfoTable)
	{
		return;
	}

	TArray<FCPMonsterWaveInfoRow*> AllRows;
	WaveInfoTable->GetAllRows<FCPMonsterWaveInfoRow>(TEXT("UCPMonsterSpawnManagerComponent::WarmUpMonsterPools"), AllRows);

	TMap<ECPMonsterType, int32> ExpectedMaxByType;
	for (const FCPMonsterWaveInfoRow* Row : AllRows)
	{
		if (!Row || Row->Round != InRound)
		{
			continue;
		}

		// CountPerSpawnPoint(라운드 수) x MonstersPerSpawn(스폰 1회당 마릿수) x 스포너 수 = 이 행이
		// 만들어낼 수 있는 실제 최대 동시 마릿수 - MonstersPerSpawn을 빠뜨리면 풀이 턱없이 작게 예열돼서
		// (예: MonstersPerSpawn=60인데 5마리만 예열) 나머지는 전부 매번 새로 SpawnActor가 떨어짐
		const int32 RowMaxAlive = Row->CountPerSpawnPoint * FMath::Max(1, Row->MonstersPerSpawn) * Row->SpawnerIndices.Num();
		int32& Existing = ExpectedMaxByType.FindOrAdd(Row->MonsterType);
		Existing = FMath::Max(Existing, RowMaxAlive);
	}

	// 보스는 위 WaveInfo 루프에 안 잡히는 경우(마지막 웨이브 행에 보스 타입이 없을 수 있음)를 대비해
	// RoundInfoTable 기준으로 최소 1마리는 보장
	if (InRoundInfo)
	{
		int32& BossCount = ExpectedMaxByType.FindOrAdd(InRoundInfo->BossMonsterType);
		BossCount = FMath::Max(BossCount, 1);
	}

	for (const TPair<ECPMonsterType, int32>& Pair : ExpectedMaxByType)
	{
		if (TSubclassOf<ACPMonsterBase> MonsterClass = MonsterClassByType.FindRef(Pair.Key))
		{
			Pool->WarmUp(Pair.Key, MonsterClass, Pair.Value);
		}
	}
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
		// 360도를 InSpawnerCount만큼 균등 분할
		const float AngleRad = FMath::DegreesToRadians(360.f * Index / InSpawnerCount);
		const FVector Offset = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f) * InSpawnerRadius;
		FVector SpawnLocation = CenterLocation + Offset;

		// 지형 고저차 보정을 위해 바로 아래로 트레이스해서 실제 바닥 높이로 스폰 위치를 맞춤
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
		}

		// 링 반경 계산이 지형/NavMesh와 딱 맞지 않을 수 있어서, 스포너를 실제로 놓기 전에 NavMesh 위의
		// 가장 가까운 유효 위치로 스냅시킴 - 이렇게 하면 반경 값을 NavMesh에 맞춰 따로 조정할 필요 없이
		// 몬스터가 항상 유효한 위치에서 스폰됨. 투영 범위 밖이면(레벨에 NavMesh 자체가 없는 위치) 원래
		// 계산된 위치를 그대로 씀 - 이 경우는 코드가 아니라 레벨의 NavMesh 커버리지를 확인해야 하는 문제
		FVector ProjectedLocation = SpawnLocation;
		if (!IsSpawnerLocationValid(SpawnLocation, &ProjectedLocation))
		{

		}
		SpawnLocation = ProjectedLocation;

		// 오너(플레이어) 쪽을 바라보도록 회전
		const FRotator SpawnRotation = (CenterLocation - SpawnLocation).Rotation();

		if (ACPMonsterSpawner* Spawner = GetWorld()->SpawnActor<ACPMonsterSpawner>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams))
		{
			Spawner->Tags.Add(*FString::Printf(TEXT("Spawner%d"), Index));
			Spawner->ArenaCenterLocation = CenterLocation;

			// 오너에 부착 - 플레이어가 움직이면 이 반경 배치가 그대로 따라감
			Spawner->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);

			SpawnersByIndex.Add(Index, Spawner);
		}
	}
}

void UCPMonsterSpawnManagerComponent::StartWave(int32 InWaveIndex)
{
	const int32 WaveNumber = InWaveIndex + 1; // 데이터 테이블의 Wave 컬럼은 1부터 시작

	TArray<FCPMonsterWaveInfoRow*> Entries;
	GetWaveEntries(CurrentRound, WaveNumber, Entries);

	if (Entries.IsEmpty())
	{
		// 정상 데이터라면 WaveInfo의 진짜 마지막 행("마지막 웨이브", RoundMob 전용)은 아래
		// bIsWaveBeforeLastWave 분기에서 한 웨이브 앞서 걸러지므로 여기 도달할 일이 없음 - 즉 이
		// Round/Wave에 해당하는 행 자체가 데이터에 없다는 뜻(오타/누락). 안전하게 보스 페이즈로
		// 넘어가되 경고를 남김

		BeginRoundWait();
		return;
	}

	CurrentWaveIndex = InWaveIndex;
	CurrentPhase = ECPWavePhase::Spawning;

	// WaveInfo의 진짜 마지막 행(GetWaveCount()번째, "마지막 웨이브")은 RoundMob 전용으로 예약되어 있어
	// StartWave()로는 스폰되지 않음 - 그래서 그 바로 앞 웨이브("마지막 웨이브 직전 웨이브")가 전멸하면
	// 곧바로 BeginRoundWait()로 넘어가고, 마지막 웨이브 몹(RoundMob)과 보스가 같은 타이밍에 등장하며
	// 중복 스폰도 없어짐
	const int32 LastWaveNumber = GetWaveCount();
	bIsWaveBeforeLastWave = (LastWaveNumber <= 0) || (WaveNumber + 1 >= LastWaveNumber);



	ActiveJobs.Reset();
	WaveSpawnTotalSeconds = 0.f;
	WaveStartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	CurrentWaveEndWaitTime = Entries[0]->WaveEndWaitTime; // 같은 웨이브의 행들은 동일한 값을 넣는다고 가정

	for (const FCPMonsterWaveInfoRow* Entry : Entries)
	{
		FCPActiveSpawnJob Job;
		Job.MonsterClass = MonsterClassByType.FindRef(Entry->MonsterType);
		Job.MonsterType = Entry->MonsterType;
		Job.CountPerSpawnPoint = Entry->CountPerSpawnPoint;
		Job.MonstersPerSpawn = Entry->MonstersPerSpawn;
		Job.SpawnRowSpacingY = Entry->SpawnRowSpacingY;
		Job.TargetSpawners = ResolveValidSpawners(Entry->SpawnerIndices);

		if (!IsValid(Job.MonsterClass) || Job.TargetSpawners.IsEmpty())
		{

			continue;
		}

		WaveSpawnTotalSeconds = FMath::Max(WaveSpawnTotalSeconds, Entry->SpawnInterval * Entry->CountPerSpawnPoint);

		const int32 JobIndex = ActiveJobs.Add(Job);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("HandleSpawnJobTick"), JobIndex);
		GetWorld()->GetTimerManager().SetTimer(ActiveJobs[JobIndex].TimerHandle, TimerDelegate, Entry->SpawnInterval, true, 0.f);
	}

	// 유효한 스폰 규칙이 하나도 없으면 곧바로 다음 단계로 진행
	if (ActiveJobs.IsEmpty())
	{
		if (bIsWaveBeforeLastWave && WaveAliveMonsterCount == 0)
		{
			BeginRoundWait();
		}
		else if (!bIsWaveBeforeLastWave)
		{
			EndWave();
		}
	}
}

void UCPMonsterSpawnManagerComponent::GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPMonsterWaveInfoRow*>& OutEntries) const
{
	OutEntries.Reset();

	if (!WaveInfoTable)
	{
		return;
	}

	TArray<FCPMonsterWaveInfoRow*> AllRows;
	WaveInfoTable->GetAllRows<FCPMonsterWaveInfoRow>(TEXT("UCPMonsterSpawnManagerComponent::GetWaveEntries"), AllRows);

	for (FCPMonsterWaveInfoRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound && Row->Wave == InWave)
		{
			OutEntries.Add(Row);
		}
	}
}

void UCPMonsterSpawnManagerComponent::DebugSkipToLastWave()
{
	if (CurrentPhase == ECPWavePhase::RoundWait || CurrentPhase == ECPWavePhase::Finished || ActiveBoss.IsValid())
	{

		return;
	}



	// 남아있는 웨이브 스폰 Job 타이머부터 정리 - 안 하면 옛 웨이브 스폰이 보스 페이즈 중에도 계속 흘러들어옴
	for (FCPActiveSpawnJob& Job : ActiveJobs)
	{
		GetWorld()->GetTimerManager().ClearTimer(Job.TimerHandle);
	}
	ActiveJobs.Reset();
	GetWorld()->GetTimerManager().ClearTimer(WaveWaitTimer);
	GetWorld()->GetTimerManager().ClearTimer(RoundWaitTimer);

	// 지금 살아있는 웨이브 몹을 전부 즉시 처치 - Dead()가 OnMonsterDied를 맨 앞에서 바로 브로드캐스트하므로
	// WaveAliveMonsterCount/TotalAliveMonsterCount는 기존 델리게이트(HandleWaveMonsterDied/HandleAnyMonsterDied)가
	// 알아서 맞춰줌 - 카운터를 직접 건드릴 필요 없음
	TArray<AActor*> AliveMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACPMonsterBase::StaticClass(), AliveMonsters);
	for (AActor* Actor : AliveMonsters)
	{
		if (ACPMonsterBase* Monster = Cast<ACPMonsterBase>(Actor))
		{
			if (!Monster->HasCCState(ECPMonsterCCState::Dead))
			{
				Monster->Dead();
			}
		}
	}

	// BeginBossPhase() 직접 호출 대신 정상 흐름 그대로 BeginRoundWait()를 태움 - RoundEndWaitTime만큼
	// 짧게 기다린 뒤 RoundMob+보스가 동시 등장함(실제 라운드 클리어 때와 동일한 연출/상태 전환).
	// CurrentPhase=RoundWait 전환도 이 함수가 알아서 해주므로 직접 건드릴 필요 없음
	BeginRoundWait();
}

void UCPMonsterSpawnManagerComponent::BeginRoundWait()
{
	// 마지막 웨이브 직전 웨이브 전멸 확인 시 호출됨 - RoundEndWaitTime만큼 기다렸다가 BeginBossPhase()에서
	// RoundMob과 보스를 동시에 등장시킴


	CurrentPhase = ECPWavePhase::RoundWait;

	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	const float BossWaitTime = RoundInfo ? RoundInfo->RoundEndWaitTime : 0.f;



	// SetTimer()는 Rate가 0 이하면 조용히 아무것도 안 하므로(콜백이 영영 안 불림), 0 이하일 땐 직접 호출
	if (BossWaitTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(RoundWaitTimer, this, &UCPMonsterSpawnManagerComponent::BeginBossPhase, BossWaitTime, false);
	}
	else
	{
		BeginBossPhase();
	}
}

void UCPMonsterSpawnManagerComponent::BeginBossPhase()
{
	// RoundEndWaitTime 경과 후 딱 한 번 호출됨 - RoundMob과 보스가 항상 같은 타이밍에 등장하도록
	// 별도 딜레이 없이 이 한 콜백에서 순서대로 둘 다 처리함
	StartRoundMobSpawning();
	SpawnBoss();
}

void UCPMonsterSpawnManagerComponent::SpawnBoss()
{


	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	if (!RoundInfo)
	{

		StopRoundMobSpawning();
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	ACPMonsterSpawner* BossSpawner = SpawnersByIndex.FindRef(RoundInfo->BossSpawnerIndex);
	if (!IsValid(BossSpawner))
	{

		StopRoundMobSpawning();
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	// 스포너 위치가 NavMesh 밖이어도 여기서 막지 않음 - 바로 아래 SpawnMonsterRow가
	// bAllowFallbackOutsideNavMesh=true로 넓은 범위 재탐색/오너 위치 기준 보정까지 자체적으로
	// 처리함. 여기서 미리 걸러버리면 그 resilient 로직이 실행될 기회조차 없이 보스 스폰 자체가
	// 스킵되는 모순이 생김(바로 아래 주석 "보스는 절대 스폰이 스킵되면 안 되므로"와 충돌)

	TSubclassOf<ACPMonsterBase> BossClass = MonsterClassByType.FindRef(RoundInfo->BossMonsterType);
	if (!IsValid(BossClass))
	{

		StopRoundMobSpawning();
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	// 보스는 절대 스폰이 스킵되면 안 되므로 NavMesh 투영 실패해도 강제로 스폰(bAllowFallbackOutsideNavMesh=true)
	const TArray<ACPMonsterBase*> SpawnedBossRow = BossSpawner->SpawnMonsterRow(BossClass, RoundInfo->BossMonsterType, 1, 0.f, CurrentRound, GetWaveCount(), GetPlayerLocation(), /*bAllowFallbackOutsideNavMesh=*/true);
	ACPMonsterBase* SpawnedBoss = SpawnedBossRow.IsValidIndex(0) ? SpawnedBossRow[0] : nullptr;
	if (SpawnedBoss)
	{
		ActiveBoss = SpawnedBoss;
		++TotalAliveMonsterCount;
		// 보스는 MaxAliveMonsterCount 상한과 무관하게 항상 스폰을 보장함 - 그래서 위쪽에 상한 체크가 아예 없음
		SpawnedBoss->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBossDied);
		SpawnedBoss->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleAnyMonsterDied);

		// RoundInfoTable의 보스 전용 값(포효 임계치/슬램 쿨타임/포효 지속시간 등)을 스폰된 인스턴스에 적용
		if (ACPMonsterBoss* Boss = Cast<ACPMonsterBoss>(SpawnedBoss))
		{
			Boss->ApplyBossWaveStat(RoundInfo->RoarHealthPercentThreshold, RoundInfo->SlamCooldown, RoundInfo->SlamRadius, RoundInfo->RoarDuration,
				RoundInfo->AddBossMaxHealth, RoundInfo->AddBossMoveSpeed, RoundInfo->AddBossAttackPower, RoundInfo->AddBossAttackRange);

			// 보스 공격이 플레이어에게 명중할 때마다 CoinPusher의 활성 코인을 몬스터 코인으로 전환
			Boss->OnBossAttackedPlayer.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBossAttackedPlayer);

			// 보스가 데미지를 받을 때마다 InGameUI의 보스 체력 게이지를 갱신
			if (UCPMonsterStatComponent* BossStat = Boss->GetAIStatComponent())
			{
				BossStat->OnMonsterHealthChanged.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBossHealthChanged);
			}

			// 보스 등장 - InGameUI의 보스 정보 블록을 켜고 이름/초기 체력을 채운다
			if (UCPInGameWidget* InGameWidget = GetInGameWidget())
			{
				InGameWidget->SetBossName(FText::FromName(Boss->GetBossName()));
				InGameWidget->SetBossInfoVisible(true);
				InGameWidget->UpdateBossHealth(Boss->GetAICurrentHealth(), Boss->GetAIMaxHealth());
			}
		}
	}
	else
	{

		StopRoundMobSpawning();
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

			continue;
		}

		if (!IsSpawnerLocationValid(Spawner->GetActorLocation()))
		{

			continue;
		}

		Result.Add(Spawner);
	}

	return Result;
}

bool UCPMonsterSpawnManagerComponent::IsSpawnerLocationValid(const FVector& InLocation, FVector* OutProjectedLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		// 검증할 방법이 없는 상태(NavMesh 시스템 자체가 없음)를 "유효함"으로 속이지 않고 그대로
		// 무효 처리함 - 예전엔 여기서 true를 반환해 검증 안 된 위치에도 스포너/몬스터가 배치됐음.
		// 주의: 이 레벨에 NavMesh가 아예 빌드되어 있지 않으면 스포너가 전부 무효 판정을 받아
		// 웨이브가 하나도 스폰되지 않게 됨 - 테스트용 빈 레벨이라면 NavMesh부터 빌드해야 함
		if (OutProjectedLocation)
		{
			*OutProjectedLocation = InLocation;
		}
		return false;
	}

	FNavLocation OutNavLocation;
	constexpr float ProjectionExtentXY = 500.f;
	constexpr float ProjectionExtentZ = 500.f;
	const bool bIsValid = NavSys->ProjectPointToNavigation(InLocation, OutNavLocation, FVector(ProjectionExtentXY, ProjectionExtentXY, ProjectionExtentZ));

	if (OutProjectedLocation)
	{
		// 실패하면(투영 범위 안에 NavMesh가 전혀 없으면) 보정할 방법이 없으므로 원래 위치를 그대로 돌려줌
		*OutProjectedLocation = bIsValid ? OutNavLocation.Location : InLocation;
	}

	return bIsValid;
}

bool UCPMonsterSpawnManagerComponent::IsWaveSpawningComplete() const
{
	for (const FCPActiveSpawnJob& Job : ActiveJobs)
	{
		if (Job.SpawnedCount < Job.CountPerSpawnPoint)
		{
			return false;
		}
	}

	return true;
}

void UCPMonsterSpawnManagerComponent::EndWave()
{
	CurrentPhase = ECPWavePhase::WaveWait;

	// SetTimer()는 Rate가 0 이하면 조용히 아무것도 안 하므로, WaveEndWaitTime이 0이면 바로 다음 웨이브로
	if (CurrentWaveEndWaitTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(WaveWaitTimer, this, &UCPMonsterSpawnManagerComponent::HandleWaveWaitFinished, CurrentWaveEndWaitTime, false);
	}
	else
	{
		HandleWaveWaitFinished();
	}
}

void UCPMonsterSpawnManagerComponent::HandleSpawnJobTick(int32 JobIndex)
{
	if (!ActiveJobs.IsValidIndex(JobIndex))
	{
		return;
	}

	FCPActiveSpawnJob& Job = ActiveJobs[JobIndex];

	// 최대 마릿수 상한에 도달했으면 이번 틱은 스폰을 건너뜀 - SpawnedCount는 그대로 둬서 다음 틱에 다시 시도함
	const int32 MaxAliveMonsterCount = GetMaxAliveMonsterCount();
	if (MaxAliveMonsterCount > 0 && TotalAliveMonsterCount >= MaxAliveMonsterCount)
	{

		return;
	}

	// 이번 틱에 처리할 스포너 개수를 예산(MaxMonstersPerJobTick)으로 제한 - 스포너 수 x MonstersPerSpawn이
	// 예산을 넘으면(대규모 웨이브) 한 프레임에 몰아서 스폰하지 않고 NextSpawnerCursor로 이어서 다음
	// 틱(SpawnInterval 후)에 계속 처리함
	const int32 MonstersPerSpawner = FMath::Max(1, Job.MonstersPerSpawn);
	const int32 MaxSpawnersThisTick = FMath::Max(1, MaxMonstersPerJobTick / MonstersPerSpawner);
	const int32 SpawnerEndIndex = FMath::Min(Job.TargetSpawners.Num(), Job.NextSpawnerCursor + MaxSpawnersThisTick);

	for (int32 SpawnerIndex = Job.NextSpawnerCursor; SpawnerIndex < SpawnerEndIndex; ++SpawnerIndex)
	{
		ACPMonsterSpawner* Spawner = Job.TargetSpawners[SpawnerIndex];
		if (IsValid(Spawner))
		{
			// TargetSpawners는 StartWave() 시점에 한 번만 검증됨 - 스포너가 플레이어에 붙어 따라다니므로
			// 그 사이 NavMesh 밖으로 밀려났을 수 있음. 이 함수 진입 전에 이미 상한(MaxAliveMonsterCount)
			// 체크를 통과한 상태라 스폰 기회를 잃지 않도록, 보스/보상 몬스터와 같은 resilient
			// 체인(bAllowFallbackOutsideNavMesh=true)에 맡김 - 여기서 미리 걸러 스킵하지 않음
			const TArray<ACPMonsterBase*> SpawnedMonsters = Spawner->SpawnMonsterRow(Job.MonsterClass, Job.MonsterType, Job.MonstersPerSpawn, Job.SpawnRowSpacingY, CurrentRound, CurrentWaveIndex + 1, GetPlayerLocation(), /*bAllowFallbackOutsideNavMesh=*/true);
			for (ACPMonsterBase* SpawnedMonster : SpawnedMonsters)
			{
				if (IsValid(SpawnedMonster))
				{
					++WaveAliveMonsterCount;
					++TotalAliveMonsterCount;
					SpawnedMonster->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleWaveMonsterDied);
					SpawnedMonster->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleAnyMonsterDied);

					// 자폭 몬스터가 플레이어에 닿아 터질 때마다 CoinPusher에 몬스터 코인을 스폰
					if (ACPMonsterBomb* Bomb = Cast<ACPMonsterBomb>(SpawnedMonster))
					{
						Bomb->OnBombExplodedOnPlayer.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBombExplodedOnPlayer);
					}
				}
			}
		}
	}

	Job.NextSpawnerCursor = SpawnerEndIndex;
	if (Job.NextSpawnerCursor < Job.TargetSpawners.Num())
	{
		return; // 이번 라운드 아직 안 끝남(예산 초과분은 다음 틱에 커서 이어서 계속) - 다음 틱을 기다림
	}

	Job.NextSpawnerCursor = 0;
	++Job.SpawnedCount;

	if (Job.SpawnedCount < Job.CountPerSpawnPoint)
	{
		return; // 이 Job은 아직 안 끝남 - 다음 틱을 기다림
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Job.TimerHandle);
	}

	if (!IsWaveSpawningComplete())
	{
		return; // 다른 Job이 아직 안 끝남
	}

	// 스폰이 막 끝남 - 이미 전멸(WaveAliveMonsterCount==0)했다면 곧바로 다음 단계로.
	// 아직 살아있는 몹이 있다면 HandleWaveMonsterDied()가 나중에 전멸을 감지해서 처리함
	if (bIsWaveBeforeLastWave)
	{

		if (WaveAliveMonsterCount == 0)
		{
			BeginRoundWait();
		}
	}
	else
	{
		EndWave();
	}
}

void UCPMonsterSpawnManagerComponent::HandleWaveWaitFinished()
{
	StartWave(CurrentWaveIndex + 1);
}

void UCPMonsterSpawnManagerComponent::StartRoundMobSpawning()
{
	// 새로 시작하기 전에 혹시 남아있을 이전 Job들부터 정리
	StopRoundMobSpawning();

	// 보스 페이즈의 잡몹은 별도 데이터가 아니라 이 라운드 WaveInfo의 마지막 행을 그대로 재사용함
	TArray<FCPMonsterWaveInfoRow*> Entries;
	GetWaveEntries(CurrentRound, GetWaveCount(), Entries);



	for (const FCPMonsterWaveInfoRow* Entry : Entries)
	{
		if (!Entry || Entry->MonsterType == ECPMonsterType::Boss)
		{
			continue; // 보스 타입 행은 SpawnBoss()가 따로 처리하므로 건너뜀
		}

		FCPActiveSpawnJob Job;
		Job.MonsterClass = MonsterClassByType.FindRef(Entry->MonsterType);
		Job.MonsterType = Entry->MonsterType;
		Job.CountPerSpawnPoint = Entry->CountPerSpawnPoint;
		Job.MonstersPerSpawn = Entry->MonstersPerSpawn;
		Job.SpawnRowSpacingY = Entry->SpawnRowSpacingY;
		Job.TargetSpawners = ResolveValidSpawners(Entry->SpawnerIndices);

		if (!IsValid(Job.MonsterClass) || Job.TargetSpawners.IsEmpty())
		{

			continue;
		}

		const int32 JobIndex = RoundMobJobs.Add(Job);



		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("HandleRoundMobSpawnTick"), JobIndex);
		GetWorld()->GetTimerManager().SetTimer(RoundMobJobs[JobIndex].TimerHandle, TimerDelegate, Entry->SpawnInterval, true, 0.f);
	}
}

void UCPMonsterSpawnManagerComponent::StopRoundMobSpawning()
{
	if (!RoundMobJobs.IsEmpty())
	{

	}

	if (UWorld* World = GetWorld())
	{
		for (FCPActiveSpawnJob& Job : RoundMobJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}
	}

	RoundMobJobs.Reset();
}

void UCPMonsterSpawnManagerComponent::HandleRoundMobSpawnTick(int32 JobIndex)
{
	if (!RoundMobJobs.IsValidIndex(JobIndex))
	{
		return;
	}

	FCPActiveSpawnJob& Job = RoundMobJobs[JobIndex];

	// 최대 마릿수 상한에 도달했으면 이번 틱은 스폰을 건너뜀 - SpawnedCount는 그대로 둬서 다음 틱에 다시 시도함
	const int32 MaxAliveMonsterCount = GetMaxAliveMonsterCount();
	if (MaxAliveMonsterCount > 0 && TotalAliveMonsterCount >= MaxAliveMonsterCount)
	{

		return;
	}

	// HandleSpawnJobTick과 동일한 이유 - 한 프레임에 스포너 수 x MonstersPerSpawn만큼 몰아서 스폰하지
	// 않고 예산(MaxMonstersPerJobTick)만큼만 처리한 뒤 커서를 남겨서 다음 틱에 이어감
	const int32 MonstersPerSpawner = FMath::Max(1, Job.MonstersPerSpawn);
	const int32 MaxSpawnersThisTick = FMath::Max(1, MaxMonstersPerJobTick / MonstersPerSpawner);
	const int32 SpawnerEndIndex = FMath::Min(Job.TargetSpawners.Num(), Job.NextSpawnerCursor + MaxSpawnersThisTick);

	for (int32 SpawnerIndex = Job.NextSpawnerCursor; SpawnerIndex < SpawnerEndIndex; ++SpawnerIndex)
	{
		ACPMonsterSpawner* Spawner = Job.TargetSpawners[SpawnerIndex];
		if (IsValid(Spawner))
		{
			// TargetSpawners는 StartRoundMobSpawning() 시점에 한 번만 검증됨 - 스포너가 플레이어에 붙어
			// 따라다니므로 그 사이 NavMesh 밖으로 밀려났을 수 있음. 이 함수 진입 전에 이미 상한
			// (MaxAliveMonsterCount) 체크를 통과한 상태라 스폰 기회를 잃지 않도록, 보스/보상 몬스터와
			// 같은 resilient 체인(bAllowFallbackOutsideNavMesh=true)에 맡김

			// 보스 페이즈 잡몹은 전멸 판정에 관여하지 않으므로 WaveAliveMonsterCount는 건드리지 않고,
			// TotalAliveMonsterCount(마릿수 상한 체크용)만 늘림
			const TArray<ACPMonsterBase*> SpawnedMonsters = Spawner->SpawnMonsterRow(Job.MonsterClass, Job.MonsterType, Job.MonstersPerSpawn, Job.SpawnRowSpacingY, CurrentRound, GetWaveCount(), GetPlayerLocation(), /*bAllowFallbackOutsideNavMesh=*/true);
			for (ACPMonsterBase* SpawnedMonster : SpawnedMonsters)
			{
				if (IsValid(SpawnedMonster))
				{
					++TotalAliveMonsterCount;
					SpawnedMonster->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleAnyMonsterDied);

					// 자폭 몬스터가 플레이어에 닿아 터질 때마다 CoinPusher에 몬스터 코인을 스폰
					if (ACPMonsterBomb* Bomb = Cast<ACPMonsterBomb>(SpawnedMonster))
					{
						Bomb->OnBombExplodedOnPlayer.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBombExplodedOnPlayer);
					}
				}
			}
		}
	}

	Job.NextSpawnerCursor = SpawnerEndIndex;
	if (Job.NextSpawnerCursor < Job.TargetSpawners.Num())
	{
		return; // 이번 라운드 아직 안 끝남(예산 초과분은 다음 틱에 커서 이어서 계속)
	}

	Job.NextSpawnerCursor = 0;
	++Job.SpawnedCount;



	if (Job.SpawnedCount >= Job.CountPerSpawnPoint)
	{
		// 목표 마릿수만큼 다 스폰했으면 이 Job 종료 (무한 스폰이 아님)
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}
	}
}

void UCPMonsterSpawnManagerComponent::HandleWaveMonsterDied()
{
	WaveAliveMonsterCount = FMath::Max(0, WaveAliveMonsterCount - 1);



	// 마지막 웨이브 직전 웨이브의 스폰이 끝난 상태에서 전멸(0)이 되면 여기서 BeginRoundWait()를 호출함
	if (bIsWaveBeforeLastWave && WaveAliveMonsterCount == 0 && CurrentPhase == ECPWavePhase::Spawning && IsWaveSpawningComplete())
	{

		BeginRoundWait();
	}
}

void UCPMonsterSpawnManagerComponent::HandleAnyMonsterDied()
{
	// 웨이브 몹/RoundMob/보스 구분 없이 죽을 때마다 호출됨 - MaxAliveMonsterCount 상한 체크에 쓰는
	// TotalAliveMonsterCount만 줄임 (전멸 판정용 WaveAliveMonsterCount는 HandleWaveMonsterDied가 별도로 관리)
	TotalAliveMonsterCount = FMath::Max(0, TotalAliveMonsterCount - 1);
}

void UCPMonsterSpawnManagerComponent::HandleRewardMonsterDied()
{
	// 보상 몬스터만 죽을 때 호출됨(SpawnRandomRewardMonster에서만 구독) - MaxRewardMonsterCount
	// 상한 체크에 쓰는 ActiveRewardMonsterCount만 줄임
	ActiveRewardMonsterCount = FMath::Max(0, ActiveRewardMonsterCount - 1);
}

void UCPMonsterSpawnManagerComponent::HandleBossDied()
{


	// 보스 페이즈 동안 계속 돌던 RoundMob 스폰 타이머를 멈춤
	StopRoundMobSpawning();

	// 보스와 동시에 스폰된 마지막 웨이브 몹은 보스가 죽어도 그대로 남아서 플레이어가 직접 잡아야 함 -
	// 그냥 놔둠(별도 정리 없음)

	ActiveBoss = nullptr;

	// 보스가 죽었으니 InGameUI의 보스 정보 블록을 다시 끈다
	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetBossInfoVisible(false);
	}

	// 다음 라운드 정보가 있으면 NextRoundStartDelay 후 다음 라운드를 시작하고, 없으면 이번이 마지막
	// 라운드라는 뜻이므로 바로 종료 처리하고 승리 화면을 띄운다
	if (FindRoundInfoRow(CurrentRound + 1))
	{
		// CurrentRound는 아직 증가시키기 전이므로 방금 끝난 라운드의 NextRoundStartDelay를 씀
		const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
		const float NextRoundDelay = RoundInfo ? RoundInfo->NextRoundStartDelay : 0.f;



		// RoundWaitTimer를 재사용(BeginRoundWait()의 보스 대기와 같은 변수) - SetTimer()는 Rate가
		// 0 이하면 조용히 아무것도 안 하므로 그 경우 직접 호출
		if (NextRoundDelay > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(RoundWaitTimer, this, &UCPMonsterSpawnManagerComponent::StartNextRound, NextRoundDelay, false);
		}
		else
		{
			StartNextRound();
		}
	}
	else
	{
		CurrentPhase = ECPWavePhase::Finished;

		// 마지막 라운드의 보스까지 처치 - 승리 처리
		if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(GetOwner()))
		{
			if (ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(PlayerCharacter->GetController()))
			{
				PC->ShowEndingResult(true);
			}
		}
	}
}

void UCPMonsterSpawnManagerComponent::StartNextRound()
{
	++CurrentRound;
	ApplyRoundInfo(CurrentRound);
	CurrentWaveIndex = 0;
	StartWave(0);
}

FCPMonsterRoundInfoRow* UCPMonsterSpawnManagerComponent::FindRoundInfoRow(int32 InRound) const
{
	if (!RoundInfoTable)
	{
		return nullptr;
	}

	TArray<FCPMonsterRoundInfoRow*> AllRows;
	RoundInfoTable->GetAllRows<FCPMonsterRoundInfoRow>(TEXT("UCPMonsterSpawnManagerComponent::FindRoundInfoRow"), AllRows);

	for (FCPMonsterRoundInfoRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound)
		{
			return Row;
		}
	}

	return nullptr;
}

int32 UCPMonsterSpawnManagerComponent::GetMaxAliveMonsterCount() const
{
	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	return RoundInfo ? RoundInfo->MaxAliveMonsterCount : 0;
}

int32 UCPMonsterSpawnManagerComponent::GetMaxRewardMonsterCount() const
{
	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
	return RoundInfo ? RoundInfo->MaxRewardMonsterCount : DefaultMaxRewardMonsterCount;
}

FVector UCPMonsterSpawnManagerComponent::GetPlayerLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

ACPCoinPusher* UCPMonsterSpawnManagerComponent::GetCoinPusher() const
{
	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(GetOwner()))
	{
		return PlayerCharacter->GetCoinPusher();
	}

	return nullptr;
}

UCPInGameWidget* UCPMonsterSpawnManagerComponent::GetInGameWidget() const
{
	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(GetOwner()))
	{
		if (ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(PlayerCharacter->GetController()))
		{
			return PC->GetInGameWidget();
		}
	}

	return nullptr;
}

void UCPMonsterSpawnManagerComponent::HandleBossHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdateBossHealth(CurrentHealth, MaxHealth);
	}
}

void UCPMonsterSpawnManagerComponent::HandleBossAttackedPlayer()
{
	if (ACPCoinPusher* CoinPusher = GetCoinPusher())
	{
		CoinPusher->MonsterConvertActive(MonsterCoinItemID, MonsterConvertCountOnBossAttack);
	}
}

void UCPMonsterSpawnManagerComponent::HandleBombExplodedOnPlayer()
{
	if (ACPCoinPusher* CoinPusher = GetCoinPusher())
	{
		CoinPusher->SpawnMonsterCoin(MonsterCoinItemID, MonsterCoinSpawnCountOnBombExplode);
	}
}

void UCPMonsterSpawnManagerComponent::HandleDropZoneItemDropped(FName ItemID)
{
	// OnDropped는 떨어진 아이템 1개마다 개별로 Broadcast되고 Count 파라미터가 따로 없으므로,
	// 이벤트 1번 = 몬스터 코인 1개로 취급함
	if (ItemID != MonsterCoinItemID)
	{
		return;
	}

	SpawnRandomRewardMonster();
}

void UCPMonsterSpawnManagerComponent::SpawnRandomRewardMonster()
{
	const int32 MaxRewardMonsterCount = GetMaxRewardMonsterCount();
	if (MaxRewardMonsterCount > 0 && ActiveRewardMonsterCount >= MaxRewardMonsterCount)
	{

		return;
	}

	// 보스를 제외한 타입 중 무작위로 하나를 고름 - DropZone에 몬스터 코인이 떨어진 데 대한 보상
	TArray<ECPMonsterType> Candidates;
	for (const TPair<ECPMonsterType, TSubclassOf<ACPMonsterBase>>& Pair : MonsterClassByType)
	{
		if (Pair.Key != ECPMonsterType::Boss && IsValid(Pair.Value))
		{
			Candidates.Add(Pair.Key);
		}
	}

	if (Candidates.IsEmpty())
	{

		return;
	}

	const ECPMonsterType ChosenType = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	const TSubclassOf<ACPMonsterBase> ChosenClass = MonsterClassByType.FindRef(ChosenType);

	// 보스와 동일하게 여기서 네브메시 유효성을 미리 걸러 스폰 자체를 막지 않음 - 스포너 존재
	// 여부만 보고 고른 뒤, 아래 SpawnMonsterRow의 bAllowFallbackOutsideNavMesh=true가 넓은 범위
	// 재탐색/오너 위치 기준 재탐색까지 다 해서 항상 스폰되게 보장함
	TArray<ACPMonsterSpawner*> AvailableSpawners;
	for (const TPair<int32, TObjectPtr<ACPMonsterSpawner>>& Pair : SpawnersByIndex)
	{
		if (IsValid(Pair.Value))
		{
			AvailableSpawners.Add(Pair.Value);
		}
	}

	if (AvailableSpawners.IsEmpty())
	{

		return;
	}

	ACPMonsterSpawner* ChosenSpawner = AvailableSpawners[FMath::RandRange(0, AvailableSpawners.Num() - 1)];
	// 보스처럼 절대 스킵되면 안 되므로 bAllowFallbackOutsideNavMesh=true
	const TArray<ACPMonsterBase*> SpawnedMonsters = ChosenSpawner->SpawnMonsterRow(ChosenClass, ChosenType, 1, 0.f, CurrentRound, CurrentWaveIndex + 1, GetPlayerLocation(), /*bAllowFallbackOutsideNavMesh=*/true);

	for (ACPMonsterBase* SpawnedMonster : SpawnedMonsters)
	{
		if (!IsValid(SpawnedMonster))
		{
			continue;
		}

		// MaxAliveMonsterCount(전체 상한)와는 무관하게 스폰됨 - 대신 MaxRewardMonsterCount(보상 몬스터
		// 전용 상한)로 위에서 따로 체크함. 전멸 판정(WaveAliveMonsterCount)에도 관여하지 않고
		// TotalAliveMonsterCount/ActiveRewardMonsterCount만 늘림
		++TotalAliveMonsterCount;
		++ActiveRewardMonsterCount;
		SpawnedMonster->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleAnyMonsterDied);
		SpawnedMonster->OnMonsterDied.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleRewardMonsterDied);

		// 보상 몬스터가 하필 자폭형이면, 그 녀석이 플레이어에 닿아 터질 때도 똑같이 몬스터 코인을 스폰함
		// (다음 보상 몬스터로 이어질 수 있음 - 의도된 동작)
		if (ACPMonsterBomb* Bomb = Cast<ACPMonsterBomb>(SpawnedMonster))
		{
			Bomb->OnBombExplodedOnPlayer.AddUniqueDynamic(this, &UCPMonsterSpawnManagerComponent::HandleBombExplodedOnPlayer);
		}


	}
}

int32 UCPMonsterSpawnManagerComponent::GetWaveCount() const
{
	if (!WaveInfoTable)
	{
		return 0;
	}

	TArray<FCPMonsterWaveInfoRow*> AllRows;
	WaveInfoTable->GetAllRows<FCPMonsterWaveInfoRow>(TEXT("UCPMonsterSpawnManagerComponent::GetWaveCount"), AllRows);

	int32 MaxWave = 0;
	for (const FCPMonsterWaveInfoRow* Row : AllRows)
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
	const FCPMonsterRoundInfoRow* RoundInfo = FindRoundInfoRow(CurrentRound);
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
