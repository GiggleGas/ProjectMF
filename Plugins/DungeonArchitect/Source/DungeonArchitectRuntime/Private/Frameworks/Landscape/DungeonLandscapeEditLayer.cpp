//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Landscape/DungeonLandscapeEditLayer.h"

#include "Core/Dungeon.h"

#define LOCTEXT_NAMESPACE "DungeonLandscapeEditLayer"


bool ULandscapeEditLayerDungeonArchitect::SupportsTargetType(ELandscapeToolTargetType InType) const {
	return (InType == ELandscapeToolTargetType::Heightmap) || (InType == ELandscapeToolTargetType::Weightmap) || (InType == ELandscapeToolTargetType::Visibility);
}

FString ULandscapeEditLayerDungeonArchitect::GetDefaultName() const {
	FString DungeonName = OwningDungeon.IsValid() ? OwningDungeon->GetName() : "[INVALID]";
	return FString::Printf(TEXT("Dungeon Layout - %s"), *DungeonName);
}

ADungeon* ULandscapeEditLayerDungeonArchitect::GetOwningDungeon() const {
	return OwningDungeon.Get();
}

void ULandscapeEditLayerDungeonArchitect::SetOwningDungeon(ADungeon* InDungeon) {
	OwningDungeon = InDungeon;
}

#if WITH_EDITOR
UE::Landscape::EditLayers::ERenderFlags ULandscapeEditLayerDungeonArchitect::GetRenderFlags(const UE::Landscape::EditLayers::FMergeContext* InMergeContext) const {
	using namespace UE::Landscape::EditLayers;
	return ERenderFlags::RenderMode_Recorded | ERenderFlags::BlendMode_SeparateBlend; // Supports the command recorder and has a separate BlendLayer function
}
#endif 

#undef LOCTEXT_NAMESPACE

