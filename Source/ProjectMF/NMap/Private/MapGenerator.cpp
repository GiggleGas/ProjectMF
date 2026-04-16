#include "MapGenerator.h"
#include "IslandShape.h"
#include "Containers/List.h"
#include "Voronoi/Voronoi.h"
#include <ProjectMF/NMap/Public/Biome.h>
#include <Kismet/KismetRenderingLibrary.h>
#include "Engine/Canvas.h"
#include "CompGeom/Delaunay2.h"
#include "BoxTypes.h"

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

	//
	/*
	{
		// 1. 定义站点（输入点）
		TArray<FVector> Sites;
		Sites.Add(FVector(0.f, 0.f, 0.f));
		Sites.Add(FVector(100.f, 0.f, 0.f));
		Sites.Add(FVector(0.f, 100.f, 0.f));
		Sites.Add(FVector(50.f, 50.f, 0.f));

		// 2. 定义计算范围（Bounding Box）
		FBox Bounds(FVector(0.f), FVector(100.f,100.f,100));

		// 3. 实例化并计算 Voronoi 图
		FVoronoiDiagram Voronoi(Sites, Bounds,10);

		// 4. 获取生成的胞元信息
		// 每个站点对应一个 Cell
		TArray<FVoronoiCellInfo> Cellss;
		Voronoi.ComputeAllCells(Cellss);

		for (auto& c : Cellss)
		{
			UE_LOG(LogTemp, Warning, TEXT("%d"),c.Vertices.Num());
		}
	}
	*/
	//

	// Generate initial random points
	if (RenderTarget)
	{

		TArray<FVector2D> points2 = GenerateRandomPoints2D(PointCount);
		using namespace UE::Geometry;
		FDelaunay2 deal;
		bool re = deal.Triangulate(points2);
		TArray<TArray<FVector2D>>vers = deal.GetVoronoiCells(points2, true, FAxisAlignedBox2d{FVector2D(5),FVector2D(490)});
		for (int32 i = 0; i < LloydRelaxations; ++i)
		{
			for (int j = 0; j < points2.Num(); ++j)
			{
				if (vers[j].Num() > 0) {
					points2[j].Set(0, 0);
					for (auto& v : vers[j]) points2[j] += v;
					points2[j] /= vers[j].Num();
				}
			}
			deal.Triangulate(points2);
			vers = deal.GetVoronoiCells(points2, true, FAxisAlignedBox2d{ FVector2D(5),FVector2D(490) });
		}

		FDrawToRenderTargetContext Context;
		UCanvas* Canvas;
		FVector2D Size;

		// 获取Canvas和渲染上下文
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
		if (!Canvas) return;

		// 画文本
		FString MyString = TEXT("Hello Canvas!");
		FLinearColor TextColor = FLinearColor::Red;
		for (auto& p : points2)
		{
			Canvas->K2_DrawBox(FVector2D(p.X - 1, p.Y - 1), FVector2D(2, 2), 2, TextColor);
		}
		for (auto& ps : vers)
		{
			for (int i=0;i<ps.Num();++i)
			{
				Canvas->K2_DrawLine(ps[i],ps[(i+1)%ps.Num()], 1, FLinearColor::Blue);
			}
		}

		// 结束渲染
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
		return;
	}




	TArray<FVector> Points = GenerateRandomPoints3D(PointCount);
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

TArray<FVector> AMapGenerator::GenerateRandomPoints3D(int32 Count)
{
	TArray<FVector> Points;// = { FVector(10,10,0),FVector(10,20,0) ,FVector(20,10,0) ,FVector(20,20,0) };
	
	Points.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		float X = FMath::FRand() * MapWidth;
		float Y = FMath::FRand() * MapHeight;
		float Z = FMath::FRand() * 10;
		Points.Add(FVector(X, Y,Z));
	}
	

	return Points;
}
TArray<FVector2D> AMapGenerator::GenerateRandomPoints2D(int32 Count)
{
	TArray<FVector2D> Points;// = { FVector(10,10,0),FVector(10,20,0) ,FVector(20,10,0) ,FVector(20,20,0) };

	Points.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		float X = FMath::FRandRange(0.1,0.9) * MapWidth;
		float Y = FMath::FRandRange(0.1, 0.9) * MapHeight;
		Points.Add(FVector2D(X, Y));
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