#pragma once

#include "CoreMinimal.h"

/**
 * Island shape generation factory
 * Produces functions that determine if a point is on island or water
 * Corresponds to IslandShape.cs
 */
class FIslandShape
{
public:
	// Island radius factor (1.0 = no small islands, 2.0 = lots of small islands)
	static constexpr float ISLAND_FACTOR = 1.07f;

	/**
	 * Create a radial island with overlapping sine waves
	 * @return Function that takes normalized point (x,y: -1 to +1) and returns true if on island
	 */
	static TFunction<bool(FVector2D)> MakeRadial();

	/**
	 * Create an island using Perlin noise
	 * @return Function that determines if point is on island
	 */
	static TFunction<bool(FVector2D)> MakePerlin();

	/**
	 * Create a square island (entire space is land)
	 * @return Function that always returns true
	 */
	static TFunction<bool(FVector2D)> MakeSquare();
};