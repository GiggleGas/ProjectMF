#pragma once

#include "CoreMinimal.h"

// Forward declarations
struct FVNCorner;
struct FVNCell
{
    int Index;
    FVector2D Position;

    TArray<int32> Cells; // 
    TArray<int32> Corners; // corner id match Vertices
    TArray<int32> Edges;

    void* ExData;
};

struct FVNCorner
{
    int Index;
    FVector2D Position;

    TArray<int32> Cells; // belong to 
    TArray<int32> Corners; // corner id match Vertices
    TArray<int32> Edges; // belong to 

    void* ExData;

};

struct FVNEdge
{
    int Index;

    int StartCornerId;
    int EndCornerId;
    int CellId1 = -1; // border edge may only belong to one cell  
    int CellId2 = -1; //

    void* ExData;

};

struct FVNGraph
{
    TArray<FVNCell> Cells;
    TArray<FVNCorner> Corners;
    TArray<FVNEdge> Edges;

    static TSharedPtr<FVNGraph> BuildFromPointsAndCorners(const TArray<FVector2D>&InPoints,const TArray < TArray<FVector2D>>& InCorners);
    bool PointInside(int CellId, float x, float y);

};

