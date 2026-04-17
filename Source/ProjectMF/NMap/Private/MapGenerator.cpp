#include "MapGenerator.h"
#include "IslandShape.h"
#include "Containers/List.h"
#include <Biome.h>
#include <Kismet/KismetRenderingLibrary.h>
#include "Engine/Canvas.h"
#include "CompGeom/Delaunay2.h"
#include "BoxTypes.h"
#include "VNGraph.h"

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

void AMapGenerator::DrawToRT()
{
	if (Graph&& RenderTarget)
	{
		FDrawToRenderTargetContext Context;
		UCanvas* Canvas;
		FVector2D Size;

		// 获取Canvas和渲染上下文
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
		if (!Canvas) return;

		const TArray<FNMCenter>& Centers=GetCenters();
		const TArray<FNMCorner>& Corners =GetCorners();
		const TArray<FNMEdge>& Edges =GetEdges();
		// 画文本
		FLinearColor TextColor = FLinearColor::Red;

		for (const FNMCenter& cen : Centers)
		{
			FCanvasUVTri tri{};
			tri.V0_Pos = cen.pvn->Position;
			tri.V0_Color =  tri.V1_Color = tri.V2_Color = FBiomeProperties::GetBiomeColor(cen.biome);

			TArray<FCanvasUVTri> CanvasUVTri;

			for (int k=0;k<cen.pvn->Corners.Num();++k)
			{
				int c1 = cen.pvn->Corners[k];
				int c2 = cen.pvn->Corners[(k + 1) % cen.pvn->Corners.Num()];

				tri.V1_Pos = Corners[c1].pvn->Position;
				tri.V2_Pos = Corners[c2].pvn->Position;
				CanvasUVTri.Add(tri);
				
			}
			Canvas->K2_DrawTriangle(nullptr,CanvasUVTri);

			// debug
			/*
			for (auto& trix : CanvasUVTri)
			{
				Canvas->K2_DrawLine(trix.V0_Pos, trix.V1_Pos, 1, FLinearColor::Gray);
				Canvas->K2_DrawBox((trix.V0_Pos + trix.V1_Pos + trix.V2_Pos) / 3, FVector2D(2), 2,FLinearColor::Green);
			}
			*/

			Canvas->K2_DrawBox(tri.V0_Pos-1, FVector2D(2, 2), 2, TextColor);
		}
		for (auto& e : Edges)
		{
			FLinearColor c = e.river > 0 ? FLinearColor::Blue:FLinearColor::Yellow;
			Canvas->K2_DrawLine(Corners[e.pvn->StartCornerId].pvn->Position, Corners[e.pvn->EndCornerId].pvn->Position, 1, c);
		}

		// 结束渲染
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
	}
}

void AMapGenerator::GenerateMap()
{
	// Validate parameters
	if (PointCount <= 0) PointCount = 500;

	if (MapWidth <= 0) MapWidth = 50.0f;

	if (MapHeight <= 0) MapHeight = 50.0f;

	
	// Generate initial random points
	TArray<FVector2D> points = GenerateRandomPoints2D(PointCount,MapWidth,MapHeight);
	TArray<TArray<FVector2D>> corners = GenerateCornerPoints(points, MapWidth, MapHeight, LloydRelaxations);
	TSharedPtr<FVNGraph> VNGraph=  FVNGraph::BuildFromPointsAndCorners(points, corners);



	// Get island checker function
	IslandChecker = GetIslandChecker();

	// Initialize the map
		// Create new graph
	Graph = MakeUnique<FNMGraph>(IslandChecker, VNGraph, (int32)MapWidth, (int32)MapHeight, LakeThreshold);

	UE_LOG(LogTemp, Warning, TEXT("Map generation complete: %d centers, %d corners"),
		Graph->GetCenters().Num(), Graph->GetCorners().Num());
}



TArray<FVector2D> AMapGenerator::GenerateRandomPoints2D(int32 Count,float Width,float Height)
{
	TArray<FVector2D> Points;// = { FVector(10,10,0),FVector(10,20,0) ,FVector(20,10,0) ,FVector(20,20,0) };

	Points.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		float X = FMath::FRandRange(0.01,0.99) * Width;
		float Y = FMath::FRandRange(0.01, 0.99) * Height;
		Points.Add(FVector2D(X, Y));
	}

	return Points;
}
TArray<TArray<FVector2D>> AMapGenerator::GenerateCornerPoints(TArray<FVector2D>& InOutPoints,float Width,float Height,int LloydRelaxations)
{
	using namespace UE::Geometry;
	FAxisAlignedBox2d Box = { FVector2D (0),FVector2D (Width,Height)};
	FDelaunay2 deal;
	bool re = deal.Triangulate(InOutPoints);
	TArray<TArray<FVector2D>> OutCornerPoints = deal.GetVoronoiCells(InOutPoints, true, Box);

	for (int32 i = 0; i < LloydRelaxations; ++i)
	{
		for (int j = 0; j < InOutPoints.Num(); ++j)
		{
			if (OutCornerPoints[j].Num() > 0) {
				InOutPoints[j].Set(0, 0);
				for (auto& v : OutCornerPoints[j]) InOutPoints[j] += v;
				InOutPoints[j] /= OutCornerPoints[j].Num();
			}
		}
		deal.Triangulate(InOutPoints);
		OutCornerPoints = deal.GetVoronoiCells(InOutPoints, true, Box);
	}
	return OutCornerPoints;
}

TFunction<bool(FVector2D)> AMapGenerator::GetIslandChecker() const
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