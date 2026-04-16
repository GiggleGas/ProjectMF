#include "VoronoiDataGenerator.h"
#include "Voronoi/Voronoi.h"

void FVoronoiDataGenerator::GenerateVoronoiData(
    const TArray<FVoronoiCellInfo>& InCellInfos,
    const TArray<FVector>& InCellPoints,
    TArray<FVector>& OutCornerPoints,
    TArray<FVNCell>& OutCells,
    TArray<FVNCorner>& OutCorners,
    TArray<FVNEdge>& OutEdges)
{
    // Clear output arrays
    OutCells.Reset();
    OutCorners.Reset();
    OutEdges.Reset();
    OutCornerPoints.Reset();

    if (InCellInfos.Num() == 0) return;
    

    // Step 1: Generate Cell structures from input FVoronoiCellInfo
    // Each cell contains its corner indices and edge indices
    OutCells.Reserve(InCellInfos.Num());

    for (int32 CellIdx = 0; CellIdx < InCellInfos.Num(); ++CellIdx)
    {
        const FVoronoiCellInfo& VoronoiCell = InCellInfos[CellIdx];
        FVNCell NewCell;

        // Corners in a cell are simply the vertices of the Voronoi cell
        // NewCell.Corners = VoronoiCell.Vertices; // Will be converted to corner indices later
        NewCell.Cells = VoronoiCell.Neighbors;
        NewCell.Corners.Reserve(VoronoiCell.Vertices.Num());
        // Initialize edges array - will be filled in step 3
        NewCell.Edges.Reserve(VoronoiCell.Vertices.Num());

        OutCells.Add(NewCell);
    }

    // Step 2: Build unique corner list
    // Create a map to track unique corner positions and their IDs
    TMap<FVector, int32> CornerPositionMap;

    for (int32 CellIdx = 0; CellIdx < OutCells.Num(); ++CellIdx)
    {
        const FVoronoiCellInfo& VoronoiCell = InCellInfos[CellIdx];

        for (const FVector& CornerPos : VoronoiCell.Vertices)
        {
            // Find existing corner or create new one
            if (!CornerPositionMap.Contains(CornerPos))
            {
                int32 NewCornerId = OutCornerPoints.Num();
                CornerPositionMap.Add(CornerPos, NewCornerId);
                OutCornerPoints.Add(CornerPos);
            }
        }
    }

    // Convert corner positions in cells to corner IDs
    for (int32 CellIdx = 0; CellIdx < OutCells.Num(); ++CellIdx)
    {
        const FVoronoiCellInfo& VoronoiCell = InCellInfos[CellIdx];
        FVNCell& CurrentCell = OutCells[CellIdx];
        // CurrentCell.Corners.Reset();

        for (const FVector& CornerPos : VoronoiCell.Vertices)
        {
            int32* CornerIdPtr = CornerPositionMap.Find(CornerPos);
            if (CornerIdPtr)
            {
                CurrentCell.Corners.Add(*CornerIdPtr);
            }
        }
    }

    // Initialize corner structures
    OutCorners.Reserve(OutCornerPoints.Num());
    for (int32 i = 0; i < OutCornerPoints.Num(); ++i)
    {
        FVNCorner NewCorner;
        NewCorner.Cells.Reserve(4); // Average Voronoi corner touches 3-4 cells
        NewCorner.Edges.Reserve(4);
        OutCorners.Add(NewCorner);
    }

    // Step 3: Build edges and update corner/cell relationships
    for (int32 CellIdx = 0; CellIdx < OutCells.Num(); ++CellIdx)
    {
        FVNCell& CurrentCell = OutCells[CellIdx];
        const FVoronoiCellInfo& VoronoiCell = InCellInfos[CellIdx];

        // Create edges between consecutive corners (polygon edges)
        int32 CornerCount = CurrentCell.Corners.Num();
        for (int32 i = 0; i < CornerCount; ++i)
        {
            int32 StartCornerId = CurrentCell.Corners[i];
            int32 EndCornerId = CurrentCell.Corners[(i + 1) % CornerCount];

            // Find or create edge
            int32 EdgeId = FindOrCreateEdge(StartCornerId, EndCornerId, CellIdx, OutEdges);

            // Add edge to cell if not already present
            if (!CurrentCell.Edges.Contains(EdgeId))
            {
                CurrentCell.Edges.Add(EdgeId);
            }

            // Update corner relationships
            if (!OutCorners[StartCornerId].Cells.Contains(CellIdx))
            {
                OutCorners[StartCornerId].Cells.Add(CellIdx);
            }

            if (!OutCorners[StartCornerId].Edges.Contains(EdgeId))
            {
                OutCorners[StartCornerId].Edges.Add(EdgeId);
            }

            if (!OutCorners[EndCornerId].Edges.Contains(EdgeId))
            {
                OutCorners[EndCornerId].Edges.Add(EdgeId);
            }

            if (!OutCorners[StartCornerId].Corners.Contains(EndCornerId))
            {
                OutCorners[StartCornerId].Corners.Add(EndCornerId);
            }

            if (!OutCorners[EndCornerId].Corners.Contains(StartCornerId))
            {
                OutCorners[EndCornerId].Corners.Add(StartCornerId);
            }
        }
    }

    // Ensure all corners have their cell relationships set
    for (int32 CornerIdx = 0; CornerIdx < OutCorners.Num(); ++CornerIdx)
    {
        FVNCorner& CurrentCorner = OutCorners[CornerIdx];

        if (CurrentCorner.Cells.Num() == 0)
        {
            // This corner should have been assigned cells, but if not, find them
            for (int32 CellIdx = 0; CellIdx < OutCells.Num(); ++CellIdx)
            {
                if (OutCells[CellIdx].Corners.Contains(CornerIdx))
                {
                    if (!CurrentCorner.Cells.Contains(CellIdx))
                    {
                        CurrentCorner.Cells.Add(CellIdx);
                    }
                }
            }
        }
    }
}

int32 FVoronoiDataGenerator::FindOrCreateEdge(
    int32 StartCornerId,
    int32 EndCornerId,
    int32 CellId,
    TArray<FVNEdge>& Edges)
{
    // Normalize edge direction (smaller ID first)
    if (EndCornerId < StartCornerId) Swap(StartCornerId, EndCornerId);
    

    // Search for existing edge
    for (int32 i = 0; i < Edges.Num(); ++i)
    {
        FVNEdge& ExistingEdge = Edges[i];
        if (ExistingEdge.StartCornerId == StartCornerId && ExistingEdge.EndCornerId == EndCornerId)
        {
            // Assign to second cell if not already assigned
            if (ExistingEdge.CellId2 == -1) ExistingEdge.CellId2 = CellId;
            
            return i;
        }
    }

    // Create new edge
    FVNEdge NewEdge;
    NewEdge.StartCornerId = StartCornerId;
    NewEdge.EndCornerId = EndCornerId;
    NewEdge.CellId1 = CellId;
    NewEdge.CellId2 = -1;

    int32 NewEdgeId = Edges.Num();
    Edges.Add(NewEdge);

    return NewEdgeId;
}
