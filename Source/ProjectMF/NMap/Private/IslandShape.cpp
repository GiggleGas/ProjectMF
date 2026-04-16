#include "IslandShape.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/Map.h"

TFunction<bool(FVector)> FIslandShape::MakeRadial()
{
	// Random parameters for this island
	int32 Bumps = FMath::RandRange(1, 5);
	float StartAngle = FMath::FRand() * 2 * PI;
	float DipAngle = FMath::FRand() * 2 * PI;

	float Random = FMath::FRand();
	float Start = 0.2f;
	float End = 0.7f;
	float DipWidth = (End - Start) * Random + Start;

	return [Bumps, StartAngle, DipAngle, DipWidth](FVector Q) -> bool
	{
		float Angle = FMath::Atan2(Q.Y, Q.X);
		float Length = 0.5f * (FMath::Max(FMath::Abs(Q.X), FMath::Abs(Q.Y)) + Q.Length());

		float R1 = 0.5f + 0.40f * FMath::Sin(StartAngle + Bumps * Angle + FMath::Cos((Bumps + 3) * Angle));
		float R2 = 0.7f - 0.20f * FMath::Sin(StartAngle + Bumps * Angle - FMath::Sin((Bumps + 2) * Angle));

		// Create a dip in the island
		if (FMath::Abs(Angle - DipAngle) < DipWidth
			|| FMath::Abs(Angle - DipAngle + 2 * PI) < DipWidth
			|| FMath::Abs(Angle - DipAngle - 2 * PI) < DipWidth)
		{
			R1 = R2 = 0.2f;
		}

		return (Length < R1 || (Length > R1 * ISLAND_FACTOR && Length < R2));
	};
}

TFunction<bool(FVector)> FIslandShape::MakePerlin()
{
	float Offset = FMath::RandRange(0.0f, 100000.0f);

	return [Offset](FVector Q) -> bool
	{
		float X = Q.X + Offset;
		float Y = Q.Y + Offset;
		float Perlin = FMath::PerlinNoise2D(FVector2D(X / 10.0f, Y / 10.0f));
		float CheckValue = 0.3f + 0.3f * Q.SquaredLength();
		return Perlin > 0.3f;
	};
}

TFunction<bool(FVector)> FIslandShape::MakeSquare()
{
	return [](FVector Q) -> bool
	{
		return true; // Entire map is land
	};
}