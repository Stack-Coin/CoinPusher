// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Animation/CPMonsterAttackRangeNotifyState.h"
#include "Monster/CPMonsterBase.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

const FName UCPMonsterAttackRangeNotifyState::DecalComponentTag(TEXT("CPAttackRangeIndicator"));

void UCPMonsterAttackRangeNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	ACPMonsterBase* Monster = MeshComp ? Cast<ACPMonsterBase>(MeshComp->GetOwner()) : nullptr;
	if (!Monster || !IndicatorMaterial)
	{
		return;
	}

	// 몽타주 초반(윈드업)엔 TurnToTarget이 아직 플레이어 쪽으로 다 안 돌았을 수 있어서, 캡슐
	// Forward 대신 "몬스터 -> 플레이어" 방향을 직접 구해서 씀 - 실제 판정(AttackHitCheck)은 나중에
	// 실행돼서 그때는 이미 플레이어 쪽을 보고 있을 거라 캡슐 Forward 그대로 써도 문제 없음
	FVector EffectiveForward = Monster->GetActorForwardVector();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Monster->GetWorld(), 0);
	FVector ToPlayer = FVector::ZeroVector;
	if (PlayerPawn)
	{
		ToPlayer = (PlayerPawn->GetActorLocation() - Monster->GetActorLocation()).GetSafeNormal2D();
		if (!ToPlayer.IsNearlyZero())
		{
			EffectiveForward = ToPlayer;
		}
	}

	// Pitch/Yaw 성분을 손으로 조합하면(오일러) 축이 꼬이기 쉬워서, 로컬 X축=아래/로컬 Z축=정면이
	// 되도록 축 벡터로 직접 조립함(X는 정확히 고정, Z는 직교화를 위해 살짝만 보정됨 - Forward랑
	// 거의 동일). 로컬 Y축(측면)은 자동으로 계산됨
	FRotator DecalRotation = FRotationMatrix::MakeFromXZ(-FVector::UpVector, EffectiveForward).Rotator();

	// 액터 위치(피벗)는 캡슐 중심 높이라 지면보다 HalfHeight만큼 위에 있음 - 여기서 그대로
	// 아래로 DecalDepth만큼만 투영하면 몸집 큰 몬스터(보스 등)는 지면까지 안 닿아서 데칼이 아예
	// 안 보임. 스폰 지점 자체를 지면 높이로 내려서 투영 깊이는 짧게 유지함
	const FVector GroundOffset = FVector(0.f, 0.f, -Monster->GetAICollisionHalfHeight());

	FVector WorldLocation = Monster->GetActorLocation() + GroundOffset;
	FVector DecalSize = FVector(DecalDepth, 50.f, 50.f);

	if (Shape == ECPAttackRangeIndicatorShape::Circle)
	{
		const float Radius = Monster->GetAIAOERadius();
		DecalSize = FVector(DecalDepth, Radius, Radius);
	}
	else
	{
		const ACPMonsterBase::FAttackSweepShape SweepShape = Monster->GetAttackSweepShape(EffectiveForward);
		const FVector Center = (SweepShape.Start + SweepShape.End) * 0.5f;
		WorldLocation = Center + GroundOffset;

		const float HalfLength = (SweepShape.End - SweepShape.Start).Size() * 0.5f;
		// MakeFromXZ(-Up, Forward)로 조립하면 로컬 Z축=Forward(정면/길이), Y축=측면(폭)에 대응함
		// (축 매핑 자체는 맞음, 실측 로그로 검산 완료) - 근데 몸집 큰 데 사거리 짧은 몬스터(예: 보스)는
		// Radius(캡슐 반경 비례)가 HalfLength(사거리)보다 커져서 화면상 "폭이 더 넓은 사각형"으로
		// 보일 수 있음. 실제 판정(GetAttackSweepShape/AttackHitCheck)은 안 건드리고, 표시용 크기만
		// 큰 값을 정면(Z) 축에 배치해서 항상 "정면이 더 길어 보이게" 함 - 대신 Radius>HalfLength인
		// 경우 화면상 폭이 실제 판정 폭보다 좁게 보이는 트레이드오프가 있음(합의된 사항)
		const float DisplayWidth = FMath::Min(SweepShape.Radius, HalfLength);
		const float DisplayLength = FMath::Max(SweepShape.Radius, HalfLength) + 50.f;
		DecalSize = FVector(DecalDepth, DisplayWidth, DisplayLength);
	}

	// KeepRelativeOffset을 쓰면 넘긴 좌표를 부모 로컬 기준으로 해석해서, 월드 스페이스로 계산한
	// Center/GroundOffset이 보스가 보는 방향에 따라 반대쪽으로 튀는 버그가 있었음 - 여기선 월드
	// 좌표를 그대로 넘기고 KeepWorldPosition으로 스폰(붙은 뒤엔 평소처럼 부모를 따라다님)
	if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(
		IndicatorMaterial, DecalSize,
		Monster->GetRootComponent(), NAME_None,
		WorldLocation, DecalRotation, EAttachLocation::KeepWorldPosition))
	{
		Decal->ComponentTags.Add(DecalComponentTag);
	}
}

void UCPMonsterAttackRangeNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// NotifyState는 몽타주 애셋에 딸린 공유 인스턴스라 멤버 변수로 데칼을 들고 있으면 여러 몬스터가
	// 동시에 재생할 때 서로 덮어씀 - 그래서 스폰 때 붙여둔 태그로 찾아서 지움
	TArray<UActorComponent*> Decals = Owner->GetComponentsByTag(UDecalComponent::StaticClass(), DecalComponentTag);
	for (UActorComponent* Comp : Decals)
	{
		Comp->DestroyComponent();
	}
}
