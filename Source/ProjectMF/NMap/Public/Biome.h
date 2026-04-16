#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"

/**
 * Biome enumeration for terrain types
 * Matches the original C# Biome enum
 */
UENUM(BlueprintType)
enum class ENMBiome : uint8
{
	Ocean = 0,
	Marsh = 1,
	Ice = 2,
	Lake = 3,
	Beach = 4,
	Snow = 5,
	Tundra = 6,
	Bare = 7,
	Scorched = 8,
	Taiga = 9,
	Shrubland = 10,
	TemperateDesert = 11,
	TemperateRainForest = 12,
	TemperateDeciduousForest = 13,
	Grassland = 14,
	SubtropicalDesert = 15,
	TropicalRainForest = 16,
	TropicalSeasonalForest = 17
};

/**
 * Biome properties - colors and names
 */
class FBiomeProperties
{
public:
	static FLinearColor GetBiomeColor(ENMBiome Biome);
	static FString GetBiomeName(ENMBiome Biome);
};