#include "Biome.h"

FLinearColor FBiomeProperties::GetBiomeColor(ENMBiome Biome)
{
	// Convert hex colors like from the original C# implementation
	static const TMap<ENMBiome, FLinearColor> BiomeColors = {
		{ ENMBiome::Ocean, FLinearColor(0.267f, 0.267f, 0.478f) },        // 44447a
		{ ENMBiome::Marsh, FLinearColor(0.184f, 0.4f, 0.4f) },            // 2f6666
		{ ENMBiome::Ice, FLinearColor(0.6f, 1.0f, 1.0f) },                // 99ffff
		{ ENMBiome::Lake, FLinearColor(0.2f, 0.4f, 0.6f) },               // 336699
		{ ENMBiome::Beach, FLinearColor(0.627f, 0.565f, 0.467f) },        // a09077
		{ ENMBiome::Snow, FLinearColor(1.0f, 1.0f, 1.0f) },               // ffffff
		{ ENMBiome::Tundra, FLinearColor(0.733f, 0.733f, 0.667f) },       // bbbbaa
		{ ENMBiome::Bare, FLinearColor(0.533f, 0.533f, 0.533f) },         // 888888
		{ ENMBiome::Scorched, FLinearColor(0.333f, 0.333f, 0.333f) },     // 555555
		{ ENMBiome::Taiga, FLinearColor(0.6f, 0.667f, 0.467f) },          // 99aa77
		{ ENMBiome::Shrubland, FLinearColor(0.533f, 0.6f, 0.467f) },      // 889977
		{ ENMBiome::TemperateDesert, FLinearColor(0.789f, 0.824f, 0.604f) }, // c9d29b
		{ ENMBiome::TemperateRainForest, FLinearColor(0.267f, 0.533f, 0.333f) }, // 448855
		{ ENMBiome::TemperateDeciduousForest, FLinearColor(0.4f, 0.58f, 0.353f) }, // 679459
		{ ENMBiome::Grassland, FLinearColor(0.533f, 0.667f, 0.333f) },    // 88aa55
		{ ENMBiome::SubtropicalDesert, FLinearColor(0.824f, 0.725f, 0.545f) }, // d2b98b
		{ ENMBiome::TropicalRainForest, FLinearColor(0.2f, 0.467f, 0.333f) }, // 337755
		{ ENMBiome::TropicalSeasonalForest, FLinearColor(0.333f, 0.6f, 0.267f) }, // 559944
	};

	const FLinearColor* Color = BiomeColors.Find(Biome);
	return Color ? *Color : FLinearColor::Black;
}

FString FBiomeProperties::GetBiomeName(ENMBiome Biome)
{
	static const TMap<ENMBiome, FString> BiomeNames = {
		{ ENMBiome::Ocean, TEXT("Ocean") },
		{ ENMBiome::Marsh, TEXT("Marsh") },
		{ ENMBiome::Ice, TEXT("Ice") },
		{ ENMBiome::Lake, TEXT("Lake") },
		{ ENMBiome::Beach, TEXT("Beach") },
		{ ENMBiome::Snow, TEXT("Snow") },
		{ ENMBiome::Tundra, TEXT("Tundra") },
		{ ENMBiome::Bare, TEXT("Bare") },
		{ ENMBiome::Scorched, TEXT("Scorched") },
		{ ENMBiome::Taiga, TEXT("Taiga") },
		{ ENMBiome::Shrubland, TEXT("Shrubland") },
		{ ENMBiome::TemperateDesert, TEXT("Temperate Desert") },
		{ ENMBiome::TemperateRainForest, TEXT("Temperate Rain Forest") },
		{ ENMBiome::TemperateDeciduousForest, TEXT("Temperate Deciduous Forest") },
		{ ENMBiome::Grassland, TEXT("Grassland") },
		{ ENMBiome::SubtropicalDesert, TEXT("Subtropical Desert") },
		{ ENMBiome::TropicalRainForest, TEXT("Tropical Rain Forest") },
		{ ENMBiome::TropicalSeasonalForest, TEXT("Tropical Seasonal Forest") },
	};

	const FString* Name = BiomeNames.Find(Biome);
	return Name ? *Name : TEXT("Unknown");
}