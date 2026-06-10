// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayEffectBase.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

void UMFGameplayEffectBase::OnGameplayEffectChanged()
{
	Super::OnGameplayEffectChanged();
	SyncGrantedStateTags();
}

void UMFGameplayEffectBase::SyncGrantedStateTags()
{
	if (GrantedStateTags.IsEmpty())
	{
		return;
	}

	// 把 GrantedStateTags 写入 Target Tags 组件 → GE 生效期间授予目标、结束自动移除。
	FInheritedTagContainer TagChanges;
	for (const FGameplayTag& Tag : GrantedStateTags)
	{
		TagChanges.AddTag(Tag);
	}

	FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(TagChanges);
}
