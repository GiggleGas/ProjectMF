#include "MapGenerator.h"
#include "IslandShape.h"
#include "Containers/List.h"
#include "Voronoi/Voronoi.h"
#include <ProjectMF/NMap/Public/Biome.h>

AMapGenerator::AMapGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMapGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void AMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMapGenerator::GenerateMap()
{
	// Validate parameters
	if (PointCount <= 0) PointCount = 500;

	if (MapWidth <= 0) MapWidth = 50.0f;

	if (MapHeight <= 0) MapHeight = 50.0f;



	// Generate initial random points
	 TArray<FVector> Points = GenerateRandomPoints(PointCount);

	
	// Lloyd relaxation for better point distribution
	for (int32 i = 0; i < LloydRelaxations; ++i)
	{
		Points = FNMGraph::RelaxPoints(Points, MapWidth, MapHeight);
	}
	

	// Get island checker function
	IslandChecker = GetIslandChecker();

	// Initialize the map
		// Create new graph
	Graph = MakeUnique<FNMGraph>(IslandChecker, Points, (int32)MapWidth, (int32)MapHeight, LakeThreshold);

	UE_LOG(LogTemp, Warning, TEXT("Map generation complete: %d centers, %d corners"),
		Graph->GetCenters().Num(), Graph->GetCorners().Num());
}

TArray<FVector> AMapGenerator::GenerateRandomPoints(int32 Count)
{
	TArray<FVector> Points;
	Points.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		float X = FMath::FRand() * MapWidth;
		float Y = FMath::FRand() * MapHeight;
		Points.Add(FVector(X, Y,0));
	}

	return Points;
}

TFunction<bool(FVector)> AMapGenerator::GetIslandChecker() const
{
	switch (IslandType)
	{
		case EIslandType::Perlin:
			return FIslandShape::MakePerlin();
		case EIslandType::Radial:
			return FIslandShape::MakeRadial();
		case EIslandType::Square:
			return FIslandShape::MakeSquare();
		default:
			return FIslandShape::MakePerlin();
	}
}

const TArray<FNMCenter>& AMapGenerator::GetCenters() const
{
	static TArray<FNMCenter> Empty;
	return Graph ? Graph->GetCenters() : Empty;
}

const TArray<FNMCorner>& AMapGenerator::GetCorners() const
{
	static TArray<FNMCorner> Empty;
	return Graph ? Graph->GetCorners() : Empty;
}

const TArray<FNMEdge>& AMapGenerator::GetEdges() const
{
	static TArray<FNMEdge> Empty;
	return Graph ? Graph->GetEdges() : Empty;
}



ENMBiome AMapGenerator::GetBiomeAtLocation(FVector Location) const
{
	if (!Graph) return ENMBiome::Ocean;
	
	const auto* Center = Graph->GetCenterAtLocation(Location);

	if (Center) return Center->biome;
	return ENMBiome::Ocean;
}