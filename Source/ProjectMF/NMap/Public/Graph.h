#pragma once

#include "CoreMinimal.h"
#include "MapTypes.h"
#include "VoronoiDataGenerator.h"

class FVoronoiDiagram;


class FNMGraph
{
public:

    FNMGraph(const TArray<FVector>& points,  int width, int height, float lakeThreshold);

    FNMGraph(TFunction<bool(FVector)> checkIsland, TArray<FVector> points,  int width, int height, float lakeThreshold);


    void Init(TFunction<bool(FVector)> checkIsland, TArray<FVector> points,  int width, int height, float lakeThreshold);
    
    static TArray<FVector> RelaxPoints(TArray<FVector> startingPoints, float width, float height);

    const FNMCenter* GetCenterAtLocation(FVector Location) const;



    const TArray<FNMCenter>& GetCenters() const { return centers ;}
    const TArray<FNMCorner>& GetCorners() const { return corners; }
    const TArray<FNMEdge>& GetEdges() const { return edges; }


    int lookupEdgeFromCenter(int pi, int ri);
 

    int lookupEdgeFromCorner(int qi, const int si);


private:

    void BuildGraph(const TArray<FVector> &points);
   

    static bool PointInside(const TArray<FVector>& CornerPoints, const TArray<int>& CornerIndex, float x, float y);


    static void AddCorner(FNMCenter& topLeft, int x, int y);
    /*
    Comparison<FNMCorner> ClockwiseComparison(FNMCenter center)
    {
        Comparison<FNMCorner> result =
            (a, b) = >
        {
            return (int)(((a.point.X - center.point.X) * (b.point.Y - center.point.Y) - (b.point.X - center.point.X) * (a.point.Y - center.point.Y)) * 1000);
        };
        return result;
    }
    */
    FNMCorner MakeCorner(FVector2D  nullablePoint);


    void AddToCornerTArray(const TArray<FNMCorner*>& v, FNMCorner* x);


    void AddToCenterTArray(TArray<FNMCenter*>& v, FNMCenter* x);


    void AssignCornerElevations();
    void AssignOceanCoastAndLand(float lakeThreshold);
    void RedistributeElevations();
    void AssignPolygonElevations();
 

    void CalculateDownslopes();
    void CalculateWatersheds();
    void CreateRivers();


    void AssignCornerMoisture();
    void AssignPolygonMoisture();
    void RedistributeMoisture();


    static ENMBiome GetBiome(const FNMCenter& p);
    
    TArray<int> LandCorners();



    private:


        TArray<TPair<int, FNMCorner>> _cornerMap;

        TFunction<bool(FVector)> inside;
        bool _needsMoreRandomness;

        int Width;
        int Height;

        // info
        TArray<FNMCenter> centers;
        TArray<FNMCorner> corners;
        TArray<FNMEdge> edges;

        // VN graph type
        TArray<FVector> CornerPoints;
        TArray<FVNCell> VNCells;
        TArray<FVNCorner>VNCorners;
        TArray<FVNEdge>VNEdges;
};

