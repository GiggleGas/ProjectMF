// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFCharacterTypes.h"
#include "MFCharacterData.generated.h"

class UPaperZDAnimSequence;

/**
 * DataAsset holding one multi-directional PaperZD sequence per character action.
 * Direction selection is handled at runtime by the SetDirectionality AnimBP node —
 * no per-direction splits are needed here.
 *
 * Assign this asset to AMFCharacter::SpriteData in the Blueprint.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFCharacterSpriteData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UPaperZDAnimSequence> Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UPaperZDAnimSequence> Run;

	/** Return the sequence for the given action. Pick falls back to Idle. */
	UPaperZDAnimSequence* GetSequence(EMFCharacterAction Action) const;
};
