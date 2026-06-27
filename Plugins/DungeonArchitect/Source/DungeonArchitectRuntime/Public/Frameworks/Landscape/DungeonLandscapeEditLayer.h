//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "LandscapeEditLayer.h"
#include "DungeonLandscapeEditLayer.generated.h"

class ADungeon;

UCLASS(MinimalAPI, meta = (ShortToolTip = "Special edit layer for landscape splines"))
class ULandscapeEditLayerDungeonArchitect : public ULandscapeEditLayerPersistent
{
	GENERATED_BODY()

public:
	// Begin ULandscapeEditLayerBase implementation
	virtual bool SupportsEditingTools() const override { return false; } // procedural layers cannot be edited through standard editing tools
	virtual bool SupportsTargetType(ELandscapeToolTargetType InType) const override;
	virtual bool NeedsPersistentTextures() const override { return true; }
	virtual bool SupportsMultiple() const override { return true; }
	virtual bool SupportsBeingCollapsedAway() const override { return false; } // this is a procedural and therefore cannot be collapsed 
	virtual FString GetDefaultName() const override;
#if WITH_EDITOR
	virtual UE::Landscape::EditLayers::ERenderFlags GetRenderFlags(const UE::Landscape::EditLayers::FMergeContext* InMergeContext) const override;
	virtual ELandscapeBlendMode GetBlendMode() const override { return ELandscapeBlendMode::LSBM_AlphaBlend; };
	bool SupportsAlphaForTargetType(ELandscapeToolTargetType InType) const override { return false; };
	virtual float GetAlphaForTargetType(ELandscapeToolTargetType InType) const override { return 1.0f; };
	virtual void SetAlphaForTargetType(ELandscapeToolTargetType InType, float InNewValue, bool bInModify, EPropertyChangeType::Type InChangeType) override { /* do nothing */ };
#endif // WITH_EDITOR
	// End ULandscapeEditLayerBase implementation
	
	ADungeon* GetOwningDungeon() const;
	void SetOwningDungeon(ADungeon* InDungeon);

protected:
	UPROPERTY()
	TWeakObjectPtr<ADungeon> OwningDungeon;
	
};

