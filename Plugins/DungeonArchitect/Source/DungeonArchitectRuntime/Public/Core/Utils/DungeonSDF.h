//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

namespace DA::SDF {
    struct FDungeonSDFPoint {
        float SDF;
        FIntPoint NearestSeed;
    };

    void GenerateSDF(int32 SDFWidth, int32 SDFHeight, const TArray<bool>& InsideMask, TArray<FDungeonSDFPoint>& OutData);
}
