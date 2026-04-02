// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFCharacterTypes.h"
#include "MFCharacterData.generated.h"

class UPaperFlipbook;

UCLASS(BlueprintType)
class PROJECTMF_API UMFAnimationConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Idle has no directional variation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FMFDirectionalFlipbooks Walk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FMFDirectionalFlipbooks Pick;
};
