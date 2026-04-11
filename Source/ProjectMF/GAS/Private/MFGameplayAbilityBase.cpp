// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayAbilityBase.h"
#include "MFCharacterBase.h"

AMFCharacterBase* UMFGameplayAbilityBase::GetMFCharacter() const
{
	return Cast<AMFCharacterBase>(GetAvatarActorFromActorInfo());
}
