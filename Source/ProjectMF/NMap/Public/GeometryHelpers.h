#pragma once

#include "CoreMinimal.h"
#include "MapTypes.h"

/**
 * Geometry helper functions for point/polygon operations
 */
class FGeometryHelpers
{
public:
	/**
	 * Interpolate between two points
	 * Result = f * Pt1 + (1 - f) * Pt2
	 */
	static FVector2D Interpolate(const FVector2D& Pt1, const FVector2D& Pt2, float F)
	{
		return FVector2D(
			F * Pt1.X + (1.0f - F) * Pt2.X,
			F * Pt1.Y + (1.0f - F) * Pt2.Y
		);
	}

	/**
	 * Calculate distance squared between two points (faster than distance)
	 */
	static float DistanceSquared(const FVector2D& P0, const FVector2D& P1)
	{
		float DX = P0.X - P1.X;
		float DY = P0.Y - P1.Y;
		return DX * DX + DY * DY;
	}

	/**
	 * Clockwise comparison for sorting corners around a center
	 * Used to order polygon vertices
	 */
	static int32 ClockwiseComparison(const FVector2D& Center, const FVector2D& A, const FVector2D& B)
	{
		float Result = (A.X - Center.X) * (B.Y - Center.Y) - (B.X - Center.X) * (A.Y - Center.Y);
		if (Result > 0) return 1;
		if (Result < 0) return -1;
		return 0;
	}

	/**
	 * Perlin noise wrapper for 2D coordinates
	 */
	static float PerlinNoise2D(float X, float Y)
	{
		return FMath::PerlinNoise2D(FVector2D(X, Y));
	}
};