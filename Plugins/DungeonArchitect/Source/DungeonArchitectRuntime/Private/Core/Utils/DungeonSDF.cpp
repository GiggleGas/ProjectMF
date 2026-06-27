//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonSDF.h"

#include "Core/Utils/DungeonLog.h"

#include "Async/ParallelFor.h"

void DA::SDF::GenerateSDF(int32 SDFWidth, int32 SDFHeight, const TArray<bool>& InsideMask,
                          TArray<FDungeonSDFPoint>& OutData) {
	const int32 SDFSize = SDFWidth * SDFHeight;
	if (InsideMask.Num() != SDFSize) {
		UE_LOG(LogDungeonArchitect, Log, TEXT("Invalid Inside mask array size"));
		return;
	}

	TArray<FIntPoint> NearestSeed;
	NearestSeed.SetNumUninitialized(SDFSize);

	const FIntPoint InvalidSeed(-1, -1);
	for (int32 i = 0; i < SDFSize; ++i) {
		NearestSeed[i] = InvalidSeed;
	}

	ParallelFor(SDFSize, [&](int32 Index) {
		int32 X = Index % SDFWidth;
		int32 Y = Index / SDFWidth;
		if (InsideMask[Index]) {
			bool bIsBoundary = false;
			for (int32 dy = -1; dy <= 1 && !bIsBoundary; ++dy) {
				for (int32 dx = -1; dx <= 1 && !bIsBoundary; ++dx) {
					if (dx == 0 && dy == 0) continue;
					const int32 nx = X + dx;
					const int32 ny = Y + dy;
					if (nx >= 0 && nx < SDFWidth && ny >= 0 && ny < SDFHeight) {
						const int32 nIndex = ny * SDFWidth + nx;
						if (!InsideMask[nIndex]) {
							bIsBoundary = true;
						}
					}
					else {
						bIsBoundary = true;
					}
				}
			}
			if (bIsBoundary) {
				NearestSeed[Index] = FIntPoint(X, Y);
			}
		}
	});
	
	for (int32 Y = 0; Y < SDFHeight; ++Y) {
		for (int32 X = 0; X < SDFWidth; ++X) {
			const int32 Index = Y * SDFWidth + X;
			
		}
	}

	int32 MaxDim = FMath::Max(SDFWidth, SDFHeight);
	for (int32 Step = MaxDim / 2; Step >= 1; Step /= 2) {
		TArray<FIntPoint> NextNearestSeed = NearestSeed;

		ParallelFor(SDFSize, [&](int32 Index) {
			int32 X = Index % SDFWidth;
			int32 Y = Index / SDFWidth;
			FIntPoint BestSeed = NearestSeed[Index];
			float BestDistSq = (BestSeed.X >= 0)
								   ? FMath::Square(static_cast<float>(X - BestSeed.X)) + FMath::Square(
									   static_cast<float>(Y - BestSeed.Y))
								   : FLT_MAX;

			for (int32 dy = -1; dy <= 1; ++dy) {
				for (int32 dx = -1; dx <= 1; ++dx) {
					if (dx == 0 && dy == 0) continue;
					const int32 nx = X + dx * Step;
					const int32 ny = Y + dy * Step;
					if (nx >= 0 && nx < SDFWidth && ny >= 0 && ny < SDFHeight) {
						const int32 nIndex = ny * SDFWidth + nx;
						const FIntPoint& NeighborSeed = NearestSeed[nIndex];
						if (NeighborSeed.X >= 0) {
							const float DistSq = FMath::Square(static_cast<float>(X - NeighborSeed.X)) +
								FMath::Square(static_cast<float>(Y - NeighborSeed.Y));
							if (DistSq < BestDistSq) {
								BestDistSq = DistSq;
								BestSeed = NeighborSeed;
							}
						}
					}
				}
			}

			NextNearestSeed[Index] = BestSeed;
		});

		NearestSeed = MoveTemp(NextNearestSeed);
	}

	OutData.SetNumUninitialized(SDFSize);

	for (int32 Y = 0; Y < SDFHeight; ++Y) {
		for (int32 X = 0; X < SDFWidth; ++X) {
			const int32 Index = Y * SDFWidth + X;
			const FIntPoint& Seed = NearestSeed[Index];

			float Distance = 0.0f;
			if (Seed.X >= 0) {
				Distance = FMath::Sqrt(FMath::Square(static_cast<float>(X - Seed.X)) + FMath::Square(static_cast<float>(Y - Seed.Y)));
			}

			OutData[Index].SDF = InsideMask[Index] ? -Distance : Distance;
			OutData[Index].NearestSeed = Seed;
		}
	}
}

