// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacterData.h"

UPaperZDAnimSequence* UMFCharacterSpriteData::GetSequence(EMFCharacterAction Action) const
{
	switch (Action)
	{
	case EMFCharacterAction::Walk:
		return Run;
	case EMFCharacterAction::Idle:
	case EMFCharacterAction::Pick: // Pick falls back to Idle sprites
	default:
		return Idle;
	}
}
