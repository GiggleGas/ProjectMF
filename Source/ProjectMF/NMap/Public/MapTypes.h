#pragma once

#include "CoreMinimal.h"
#include "Containers/List.h"
#include "Biome.h"

struct FNMEdge;
struct FNMCenter;
struct FNMCorner;

struct FNMCorner
{
    int index;
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
    int river;
};


struct FNMCenter
{
    int index;
    bool water;
    bool ocean;
    bool coast;
    bool border;
    ENMBiome biome;
    float elevation;
    float moisture;


    

};
