// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFCharacterTypes.h"
#include "MFCharacterData.generated.h"

class UPaperFlipbook;
class UPaperZDAnimSequence;

// ============================================================
// Legacy animation config (world-axis directional flipbooks)
// ============================================================

UCLASS(BlueprintType)
class PROJECTMF_API UMFAnimationConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FMFDirectionalFlipbooks Walk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FMFDirectionalFlipbooks Pick;
};

// ============================================================
// Camera-relative PaperZD sprite DataAsset
// ============================================================

/**
 * DataAsset for PaperZD animation sequences organised by camera-relative side.
 *
 * Layout:
 *   Idle / Run  ×  Front / Back / Left / Right
 *
 * Assign this asset to AMFCharacter::SpriteData in the Blueprint.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFCharacterSpriteData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	FMFCameraRelativeSprites Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	FMFCameraRelativeSprites Run;

	/** Return the sequence for the given action + camera-relative direction. */
	UPaperZDAnimSequence* GetSequence(EMFCharacterAction Action, EMFCameraRelativeDir Dir) const;

private:
	UPaperZDAnimSequence* GetFromSet(const FMFCameraRelativeSprites& Set, EMFCameraRelativeDir Dir) const;
};
