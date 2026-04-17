#pragma once

#include "CoreMinimal.h"
#include "Biome.h"

struct FVNGraph;
struct FVNCell;
struct FVNCorner;
struct FVNEdge;


struct FNMEdge;
struct FNMCenter;
struct FNMCorner;

struct FNMCenter
{
    int index;
    FVNCell* pvn;
    bool water;
    bool ocean;
    bool coast;
    bool border;
    ENMBiome biome;
    float elevation;
    float moisture;

};

struct FNMCorner
{
    int index;
    FVNCorner* pvn;
    bool ocean;
    bool water;
    bool coast;
    bool border;
    float elevation;
    float moisture;

    int river;
    FNMCorner* watershed; // corner id 
    FNMCorner* downslope; // corner id
    int watershed_size;
};

struct FNMEdge
{
    int index;
    FVNEdge* pvn;

    int river;
};






class FNMGraph
{
public:

    FNMGraph(TSharedPtr<FVNGraph> Graph,  int width, int height, float lakeThreshold);

    FNMGraph(TFunction<bool(FVector2D)> checkIsland, TSharedPtr<FVNGraph> Graph,  int width, int height, float lakeThreshold);


    void Init(TFunction<bool(FVector2D)> checkIsland, TSharedPtr<FVNGraph> Graph,  int width, int height, float lakeThreshold);
    
    const FNMCenter* GetCenterAtLocation(FVector Location) const;



    const TArray<FNMCenter>& GetCenters() const { return centers ;}
    const TArray<FNMCorner>& GetCorners() const { return corners; }
    const TArray<FNMEdge>& GetEdges() const { return edges; }


    int lookupEdgeFromCenter(int pi, int ri);
 

    int lookupEdgeFromCorner(int qi, const int si);


private:

    void BuildGraph();
   
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

        TFunction<bool(FVector2D)> inside;
        bool _needsMoreRandomness;

        int Width;
        int Height;

        // info
        TArray<FNMCenter> centers;
        TArray<FNMCorner> corners;
        TArray<FNMEdge> edges;

        // VN graph type
        TSharedPtr<FVNGraph> VNGraph;
};

