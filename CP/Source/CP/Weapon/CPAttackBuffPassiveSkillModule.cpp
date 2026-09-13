#include "Weapon/CPAttackBuffPassiveSkillModule.h"
#include "Weapon/CPWeaponBase.h"

int32 UCPAttackBuffPassiveSkillModule::GetMaxLevel() const
{
	return FMath::Max(LevelData.Num(), 1);
}

const FCPAttackBuffLevelData& UCPAttackBuffPassiveSkillModule::GetLevelData(int32 Level) const
{
	static const FCPAttackBuffLevelData DefaultLevelData;

	if (LevelData.Num() == 0)
	{
		return DefaultLevelData;
	}

	return LevelData[FMath::Clamp(Level - 1, 0, LevelData.Num() - 1)];
}

void UCPAttackBuffPassiveSkillModule::Activate(const FCPPassiveSkillActivationContext& Context)
{
	if (!Context.Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPAttackBuffPassiveSkillModule::Activate - Context.Weapon is null"));
		return;
	}

	const FCPAttackBuffLevelData& LevelInfo = GetLevelData(Context.Weapon->GetWeaponLevel());

	Context.Weapon->ApplyPassiveStatBuff(LevelInfo.AttackPowerBonus, LevelInfo.AttackSpeedMultiplierBonus, LevelInfo.RangeMultiplierBonus, Duration,
		BuffEffect, BuffEffectLocationOffset, BuffEffectRotationOffset, BuffEffectScale * LevelInfo.EffectScale);
}
