// Copyright ProjectMF. All Rights Reserved.

#include "MFFactionStatics.h"
#include "MFGameplayTags.h"
#include "AbilitySystemComponent.h"

FGameplayTagContainer UMFFactionStatics::GetTeamTags(const UAbilitySystemComponent* ASC)
{
	FGameplayTagContainer TeamTags;
	if (!ASC) return TeamTags;

	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);

	for (const FGameplayTag& Tag : Owned)
	{
		if (Tag.MatchesTag(MFGameplayTags::Team))
		{
			TeamTags.AddTag(Tag);
		}
	}
	return TeamTags;
}

void UMFFactionStatics::SetFaction(UAbilitySystemComponent* ASC, const FGameplayTagContainer& NewTeamTags)
{
	if (!ASC) return;

	const FGameplayTagContainer Existing = GetTeamTags(ASC);
	if (!Existing.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(Existing);
	}
	if (!NewTeamTags.IsEmpty())
	{
		ASC->AddLooseGameplayTags(NewTeamTags);
	}
}

bool UMFFactionStatics::AreSameTeam(const UAbilitySystemComponent* ASCA, const UAbilitySystemComponent* ASCB)
{
	if (!ASCA || !ASCB) return false;

	const FGameplayTagContainer TeamA = GetTeamTags(ASCA);
	if (TeamA.IsEmpty()) return false;  // 中立与所有人都不同队

	const FGameplayTagContainer TeamB = GetTeamTags(ASCB);
	return TeamB.HasAny(TeamA);
}
