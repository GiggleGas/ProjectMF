#include "VNGraph.h"

bool FVNGraph::PointInside(int CellId, float x, float y)

{
    // http://alienryderflex.com/polygon/

    //  The function will return YES if the point x,y is inside the polygon, or
    //  NO if it is not.  If the point is exactly on the edge of the polygon,
    //  then the function may return YES or NO.
    //
    //  Note that division by zero is avoided because the division is protected
    //  by the "if" clause which surrounds it.
    
    if (!Cells.IsValidIndex(CellId)) return false;
    
    const FVNCell& Cell = Cells[CellId];
    int polyCorners = Cell.Corners.Num();

    int j = polyCorners - 1;
    bool oddNodes = false;

    for (int i : Cell.Corners)
    {
        const FVector2D& tpointi = Corners[i].Position;
        const FVector2D& tpointj = Corners[j].Position;

        if ((tpointi.Y < y && tpointj.Y >= y
            || tpointj.Y < y && tpointi.Y >= y)
            && (tpointi.X <= x || tpointj.X <= x))
        {
            oddNodes ^= (tpointi.X + (y - tpointi.Y) / (tpointj.Y - tpointi.Y) * (tpointj.X - tpointi.X) < x);
        }
        j = i;
    }

    return oddNodes;
}


TSharedPtr<FVNGraph> FVNGraph::BuildFromPointsAndCorners(const TArray<FVector2D>& InPoints, const TArray<TArray<FVector2D>>& InCorners)
{
    TSharedPtr<FVNGraph> retp = MakeShared<FVNGraph>();
   FVNGraph& ret=*retp.Get();

   if (InPoints.IsEmpty() || InCorners.IsEmpty() || InPoints.Num() != InCorners.Num()) return retp;

   // Step 1: Generate Cell structures from input FVoronoiCellInfo
    // Each cell contains its corner indices and edge indices
   ret.Cells.SetNumZeroed(InPoints.Num());
   for (int i = 0; i < InPoints.Num(); ++i)
   {
       FVNCell& Cell=ret.Cells[i];
       Cell.Index = i;
       Cell.Position = InPoints[i];
       Cell.Cells.Reserve(InCorners[i].Num());
       Cell.Corners.Reserve(InCorners[i].Num());
       Cell.Edges.Reserve(InCorners[i].Num());
   }

   // Step 2: Build unique corner list
   // Create a map to track unique corner positions and their IDs
   // cell <-> corner
   TMap<FVector2D, int32> CornerPositionMap;

   for (int32 i = 0; i < InPoints.Num(); ++i)
   {
       FVNCell& Cell = ret.Cells[i];

       for (const FVector2D& CornerPos : InCorners[i])
       {
           // Find existing corner or create new one
           int ci = CornerPositionMap.FindOrAdd(CornerPos, ret.Corners.Num());

           if (ci== ret.Corners.Num())
           {
               ret.Corners.Add({.Index= ret.Corners.Num(),.Position= CornerPos });
           }

           FVNCorner& Corner = ret.Corners[ci];

           Cell.Corners.Add(ci);
           Corner.Cells.Add(i);
       }
   }

   //
   // Step 3: Build edges and update corner/cell relationships
   // edge -> cell
   // edge -> corner
   TMap<TTuple<int, int>, int> EdgeIdMap;

   for (int32 CellIdx = 0; CellIdx < InPoints.Num(); ++CellIdx)
   {
       FVNCell& CurrentCell = ret.Cells[CellIdx];

       // Create edges between consecutive corners (polygon edges)
       int32 CornerCount = CurrentCell.Corners.Num();
       for (int32 i = 0; i < CornerCount; ++i)
       {
           int32 StartCornerId = CurrentCell.Corners[i];
           int32 EndCornerId = CurrentCell.Corners[(i + 1) % CornerCount];

           if (StartCornerId > EndCornerId) Swap(StartCornerId, EndCornerId);
           TTuple<int, int> ekey = { StartCornerId,EndCornerId };

           // 
           if (int* eip = EdgeIdMap.Find(ekey)) ret.Edges[*eip].CellId2 = CellIdx;
           else
           {
               int EdgeId = ret.Edges.Num();
               EdgeIdMap.Add(ekey, EdgeId);
               ret.Edges.Add({ .Index = EdgeId ,.StartCornerId = StartCornerId,.EndCornerId = EndCornerId,.CellId1 = CellIdx });
           }
       }
   }

   //
   for (int ei = 0; ei < ret.Edges.Num(); ++ei)
   {
       const FVNEdge& Edge = ret.Edges[ei];

       // cell -> edge
       // cell <->cell
       ret.Cells[Edge.CellId1].Edges.Add(ei);

       if (Edge.CellId2>=0) {
           ret.Cells[Edge.CellId2].Edges.Add(ei);

           ret.Cells[Edge.CellId1].Cells.Add(Edge.CellId2);
           ret.Cells[Edge.CellId2].Cells.Add(Edge.CellId1);
       }

       // corner -> edge
       ret.Corners[Edge.StartCornerId].Edges.Add(ei);
       ret.Corners[Edge.EndCornerId].Edges.Add(ei);

       // corner <-> corner
       ret.Corners[Edge.StartCornerId].Corners.Add(Edge.EndCornerId);
       ret.Corners[Edge.EndCornerId].Corners.Add(Edge.StartCornerId);
   }

   return retp;
}


