#pragma once

#include "CoreMinimal.h"

// Forward declarations
struct FVoronoiCellInfo;
struct FVNCell
{
    TArray<int32> Cells; // 
    TArray<int32> Corners; // corner id match Vertices
    TArray<int32> Edges;
};

struct FVNCorner
{
    TArray<int32> Cells; // belong to 
    TArray<int32> Corners; // corner id match Vertices
    TArray<int32> Edges; // belong to 
};

struct FVNEdge
{
    int StartCornerId;
    int EndCornerId;
    int CellId1 = -1; // border edge may only belong to one cell  
    int CellId2 = -1; //
};

class FVoronoiDataGenerator
{
public:
    /**
     * Generate structured Voronoi data from raw cell information
     * 
     * @param InCells Input Voronoi cell data
     * @param InPoints Cell location points
     * @param OutCells Generated cell structures with corners and edges
     * @param OutCorners Generated unique corner structures
     * @param OutEdges Generated unique edge structures
     */
    static void GenerateVoronoiData(
        const TArray<FVoronoiCellInfo>& InCellInfos,
        const TArray<FVector>& InCellPoints,
        TArray<FVector>& OutCornerPoints,
        TArray<FVNCell>& OutCells,
        TArray<FVNCorner>& OutCorners,
        TArray<FVNEdge>& OutEdges
    );

private:
    /**
     * Build corner data from cells
     */
    static void BuildCorners(
        const TArray<FVNCell>& InCells,
        const TArray<FVoronoiCellInfo>& InCellInfos,
        TArray<FVNCorner>& OutCorners
    ) {
    };

    /**
     * Build edge data from cells and corners
     */
    static void BuildEdges(
        const TArray<FVNCell>& InCells,
        const TArray<FVoronoiCellInfo>& InVoronoiCells,
        TArray<FVNEdge>& OutEdges
    ) {
    };

    /**
     * Find or create edge between two corners
     */
    static int32 FindOrCreateEdge(
        int32 StartCornerId,
        int32 EndCornerId,
        int32 CellId,
        TArray<FVNEdge>& Edges
    );
};
