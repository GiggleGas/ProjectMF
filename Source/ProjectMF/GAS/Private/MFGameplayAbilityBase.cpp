// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayAbilityBase.h"
#include "MFCharacterBase.h"
#include "MFGameplayTags.h"

UMFGameplayAbilityBase::UMFGameplayAbilityBase()
{
	// 死亡 / 眩晕时禁止任何技能激活（所有 GA 继承此基类）。
	// 派生 GA 可在自己的构造里继续 AddTag 追加专属阻断标签。
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Stunned);
}

AMFCharacterBase* UMFGameplayAbilityBase::GetMFCharacter() const
{
	return Cast<AMFCharacterBase>(GetAvatarActorFromActorInfo());
}
