// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacterData.h"

UPaperZDAnimSequence* UMFCharacterSpriteData::GetSequence(EMFCharacterAction Action, EMFCameraRelativeDir Dir) const
{
	switch (Action)
	{
	case EMFCharacterAction::Walk:
		return GetFromSet(Run, Dir);
	case EMFCharacterAction::Idle:
	case EMFCharacterAction::Pick: // Pick falls back to Idle sprites
	default:
		return GetFromSet(Idle, Dir);
	}
}

UPaperZDAnimSequence* UMFCharacterSpriteData::GetFromSet(const FMFCameraRelativeSprites& Set, EMFCameraRelativeDir Dir) const
{
	switch (Dir)
	{
	case EMFCameraRelativeDir::Front: return Set.Front;
	case EMFCameraRelativeDir::Back:  return Set.Back;
	case EMFCameraRelativeDir::Left:  return Set.Left;
	case EMFCameraRelativeDir::Right: return Set.Right;
	default:                          return nullptr;
	}
}
