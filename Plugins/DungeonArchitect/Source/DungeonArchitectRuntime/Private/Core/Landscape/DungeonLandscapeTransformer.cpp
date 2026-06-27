//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Landscape/DungeonLandscapeTransformer.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonSDF.h"
#include "Frameworks/Landscape/DungeonLandscapeEditLayer.h"

#include "Async/ParallelFor.h"
#include "LandscapeComponent.h"
#include "LandscapeDataAccess.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"

#if WITH_EDITOR
#include "LandscapeEditLayer.h"
#endif

namespace {
	int32 GetBlurPasses(EWeightPaintBlurStrength Strength) {
		switch (Strength) {
			case EWeightPaintBlurStrength::Light:  return 2;
			case EWeightPaintBlurStrength::Medium: return 4;
			case EWeightPaintBlurStrength::Heavy:  return 6;
			default: return 0;
		}
	}

	int32 GetBlurKernelRadius(EWeightPaintBlurSize Size) {
		switch (Size) {
			case EWeightPaintBlurSize::Small:  return 1;  // 3x3
			case EWeightPaintBlurSize::Medium: return 2;  // 5x5
			case EWeightPaintBlurSize::Large:  return 3;  // 7x7
			default: return 2;
		}
	}
}

DEFINE_LOG_CATEGORY(LogLandscapeModifier)

UDungeonLandscapeTransformer::UDungeonLandscapeTransformer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
}

void UDungeonLandscapeTransformer::OnDungeonLayoutBuilt_Implementation(ADungeon* Dungeon) {
#if WITH_EDITOR
	BuildLandscape(Dungeon);
#endif // WITH_EDITOR
}

void UDungeonLandscapeTransformer::OnPreDungeonDestroy_Implementation(ADungeon* Dungeon) {
#if WITH_EDITOR
	Cleanup();
#endif // WITH_EDITOR
}

void UDungeonLandscapeTransformer::OnComponentDestroyed(bool bDestroyingHierarchy) {
	Super::OnComponentDestroyed(bDestroyingHierarchy);
#if WITH_EDITOR
	Cleanup();
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void UDungeonLandscapeTransformer::Cleanup() const {
	if (IsValid(Landscape)) {
		// Cleanup stale dungeon edit layers
		TArray<ULandscapeEditLayerBase*> EditLayers = Landscape->GetEditLayers();
		for (ULandscapeEditLayerBase* EditLayer : EditLayers) { 
			if (ULandscapeEditLayerDungeonArchitect* DungeonEditLayer = Cast<ULandscapeEditLayerDungeonArchitect>(EditLayer)) {
				bool bClear = false;
				if (ADungeon* OwningDungeon = DungeonEditLayer->GetOwningDungeon()) {
					if (OwningDungeon == GetOwner()) {
						bClear = true;
					}
				}
				else {
					bClear = true;
				}
			
				if (bClear) {
					int32 LayerIndex = Landscape->GetLayerIndex(EditLayer->GetGuid());
					Landscape->DeleteLayer(LayerIndex);
				}
			}
		}
		
	}
}
#endif // WITH_EDITOR

#if WITH_EDITOR

struct FDungeonPolyRasterData {
	TArray<FVector2D> Corners;
	float Height;
	float FalloffWidth;
};

struct FDungeonCircleRasterData {
	FVector2D Center;
	float Radius;
	float Height;
	float FalloffWidth;
};

namespace ScanlineRasterizer {
	// Rasterize a filled circle using scanline approach
	template <typename ProcessPixelFunc>
	void RasterizeFilledCircle(const FVector2D& Center, float Radius,
	                           int32 ClipMinX, int32 ClipMinY, int32 ClipMaxX, int32 ClipMaxY,
	                           ProcessPixelFunc&& ProcessPixel) {
		if (Radius <= 0.0f) {
			return;
		}

		const int32 RadiusCeil = FMath::CeilToInt(Radius);
		const int32 CenterYInt = FMath::RoundToInt(Center.Y);

		const int32 YStart = FMath::Max(CenterYInt - RadiusCeil, ClipMinY);
		const int32 YEnd = FMath::Min(CenterYInt + RadiusCeil, ClipMaxY);

		const float RadiusSq = Radius * Radius;

		for (int32 Y = YStart; Y <= YEnd; ++Y) {
			const float DY = static_cast<float>(Y) - Center.Y;
			const float DYSq = DY * DY;

			if (DYSq > RadiusSq) {
				continue;
			}

			// Calculate x extent at this scanline: x = sqrt(r^2 - y^2)
			const float XExtent = FMath::Sqrt(RadiusSq - DYSq);
			const int32 XLeft = FMath::Max(FMath::CeilToInt(Center.X - XExtent), ClipMinX);
			const int32 XRight = FMath::Min(FMath::FloorToInt(Center.X + XExtent), ClipMaxX);

			for (int32 X = XLeft; X <= XRight; ++X) {
				ProcessPixel(X, Y);
			}
		}
	}

	template <typename ProcessPixelFunc>
	void RasterizeTriangle(const FVector2D& V0, const FVector2D& V1, const FVector2D& V2,
	                       int32 ClipMinX, int32 ClipMinY, int32 ClipMaxX, int32 ClipMaxY,
	                       ProcessPixelFunc&& ProcessPixel) {
		const FVector2D* Sorted[3] = {&V0, &V1, &V2};
		if (Sorted[0]->Y > Sorted[1]->Y) {
			Swap(Sorted[0], Sorted[1]);
		}
		if (Sorted[1]->Y > Sorted[2]->Y) {
			Swap(Sorted[1], Sorted[2]);
		}
		if (Sorted[0]->Y > Sorted[1]->Y) {
			Swap(Sorted[0], Sorted[1]);
		}

		const FVector2D& Top = *Sorted[0];
		const FVector2D& Mid = *Sorted[1];
		const FVector2D& Bot = *Sorted[2];

		const int32 YStart = FMath::Max(FMath::CeilToInt(Top.Y), ClipMinY);
		const int32 YEnd = FMath::Min(FMath::FloorToInt(Bot.Y), ClipMaxY);
		if (YStart > YEnd) {
			return;
		}

		const float TotalHeight = Bot.Y - Top.Y;
		if (TotalHeight < KINDA_SMALL_NUMBER) {
			return;
		}

		const float UpperHeight = Mid.Y - Top.Y;
		const float LowerHeight = Bot.Y - Mid.Y;

		for (int32 Y = YStart; Y <= YEnd; ++Y) {
			float YFloat = static_cast<float>(Y) + 0.5f;

			float T_Long = (YFloat - Top.Y) / TotalHeight;
			float X_Long = Top.X + T_Long * (Bot.X - Top.X);

			float X_Short;
			if (YFloat < Mid.Y) {
				if (UpperHeight < KINDA_SMALL_NUMBER) {
					X_Short = Mid.X;
				}
				else {
					float T_Short = (YFloat - Top.Y) / UpperHeight;
					X_Short = Top.X + T_Short * (Mid.X - Top.X);
				}
			}
			else {
				if (LowerHeight < KINDA_SMALL_NUMBER) {
					X_Short = Mid.X;
				}
				else {
					float T_Short = (YFloat - Mid.Y) / LowerHeight;
					X_Short = Mid.X + T_Short * (Bot.X - Mid.X);
				}
			}

			float XLeftF = FMath::Min(X_Long, X_Short);
			float XRightF = FMath::Max(X_Long, X_Short);

			int32 XLeft = FMath::Max(FMath::CeilToInt(XLeftF), ClipMinX);
			int32 XRight = FMath::Min(FMath::FloorToInt(XRightF), ClipMaxX);

			for (int32 X = XLeft; X <= XRight; ++X) {
				ProcessPixel(X, Y);
			}
		}
	}

	template <typename ProcessPixelFunc>
	void RasterizePolygonFan(const TArray<FVector2D>& Points,
	                         int32 ClipMinX, int32 ClipMinY, int32 ClipMaxX, int32 ClipMaxY,
	                         ProcessPixelFunc&& ProcessPixel) {
		if (Points.Num() < 3) {
			return;
		}

		const FVector2D& P0 = Points[0];
		for (int32 i = 1; i < Points.Num() - 1; ++i) {
			RasterizeTriangle(P0, Points[i], Points[i + 1],
			                  ClipMinX, ClipMinY, ClipMaxX, ClipMaxY,
			                  ProcessPixel);
		}
	}
}

#endif // WITH_EDITOR


#if WITH_EDITOR
void UDungeonLandscapeTransformer::BuildLandscape(ADungeon* Dungeon) {
	if (!Landscape) {
		UE_LOG(LogLandscapeModifier, Warning, TEXT("No landscape reference specified in the dungeon actor"));
		return;
	}

	if (!Dungeon) {
		UE_LOG(LogLandscapeModifier, Warning, TEXT("Invalid dungeon actor reference"));
		return;
	}

	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (!LandscapeInfo) {
		UE_LOG(LogLandscapeModifier, Warning,
		       TEXT("Landscape modification works only within the editor (UE4 engine limitation)"));
		return;
	}

	RasterizeLayoutOnLayer(Dungeon, LandscapeInfo);
}

ULandscapeEditLayerDungeonArchitect* UDungeonLandscapeTransformer::GetDungeonLandscapeEditLayer(ALandscape* InLandscape) const {
	if (!InLandscape) {
		return nullptr;
	}
	
	ADungeon* OwningDungeon = Cast<ADungeon>(GetOwner());
	if (!OwningDungeon) {
		return nullptr;
	}
	
	ULandscapeEditLayerDungeonArchitect* TargetLayer = nullptr;
	for (ULandscapeEditLayerBase* EditLayer : InLandscape->GetEditLayers()) {
		if (ULandscapeEditLayerDungeonArchitect* DungeonEditLayer = Cast<ULandscapeEditLayerDungeonArchitect>(EditLayer)) {
			if (DungeonEditLayer->GetOwningDungeon() == OwningDungeon) {
				TargetLayer = DungeonEditLayer;
				break;
			}
		}
	}

	if (!TargetLayer) {
		int32 LayerIndex = InLandscape->CreateLayer("Dungeon Layout", ULandscapeEditLayerDungeonArchitect::StaticClass());
		TargetLayer = Cast<ULandscapeEditLayerDungeonArchitect>(InLandscape->GetEditLayer(LayerIndex));
		if (TargetLayer) {
			TargetLayer->SetOwningDungeon(OwningDungeon);
		}
	}

	return TargetLayer;
}

void UDungeonLandscapeTransformer::RasterizeLayoutOnLayer(ADungeon* Dungeon, ULandscapeInfo* LandscapeInfo) {
	if (!Dungeon || !LandscapeInfo || !Landscape) {
		return;
	}

	int32 LandscapeMinX, LandscapeMinY, LandscapeMaxX, LandscapeMaxY;
	if (!LandscapeInfo->GetLandscapeExtent(LandscapeMinX, LandscapeMinY, LandscapeMaxX, LandscapeMaxY)) {
		return;
	}

	ALandscapeProxy* LandscapeProxy = LandscapeInfo->GetLandscapeProxy();
	if (!LandscapeProxy) {
		return;
	}

	ULandscapeEditLayerDungeonArchitect* DungeonEditLayer = GetDungeonLandscapeEditLayer(Landscape);
	if (!DungeonEditLayer) {
		UE_LOG(LogLandscapeModifier, Warning, TEXT("Failed to get or create dungeon landscape edit layer"));
		return;
	}
	Landscape->ClearEditLayer(DungeonEditLayer->GetGuid());

	FGuid DungeonLayerGuid = DungeonEditLayer->GetGuid();
	FScopedSetLandscapeEditingLayer Scope(Landscape, DungeonLayerGuid, [this]
	{
		Landscape->RequestLayersContentUpdate(Update_All);
	});

	const FTransform LandscapeToWorld = LandscapeProxy->LandscapeActorToWorld();
	const FTransform WorldToLandscape = LandscapeToWorld.Inverse();

	TArray<FDungeonPolyRasterData> PolyRasterList;
	TArray<FDungeonCircleRasterData> CircleRasterList;

	FTransform DungeonTransform = Dungeon->GetActorTransform();
	UDungeonModel* DungeonModel = Dungeon->GetModel();
	if (!DungeonModel) {
		UE_LOG(LogLandscapeModifier, Warning, TEXT("Invalid Dungeon Model state"));
		return;
	}

	const float WorldFalloffWidth = FMath::Max(0, BlendDistanceWorldUnits);
	const FVector FalloffVector = WorldToLandscape.TransformVector(FVector(WorldFalloffWidth, 0, 0));
	const float FalloffWidth = FalloffVector.Size();

	const FDungeonLayoutData& DungeonLayout = DungeonModel->DungeonLayout;
	for (const FDungeonLayoutDataChunkInfo& ChunkShape : DungeonLayout.ChunkShapes) {
		for (const FDAShapePolygon& ConvexPoly : ChunkShape.ConvexPolys) {
			FDungeonPolyRasterData& RasterData = PolyRasterList.AddDefaulted_GetRef();
			RasterData.FalloffWidth = FalloffWidth;

			RasterData.Height = ConvexPoly.Transform.GetLocation().Z;
			if (ConvexPoly.bDungeonLocalTransform) {
				RasterData.Height += DungeonTransform.GetLocation().Z;
			}

			for (const FVector2D& LayoutPolyPoint : ConvexPoly.Points) {
				FVector WorldPoint = ConvexPoly.Transform.TransformPosition(FVector(LayoutPolyPoint, 0));
				if (ConvexPoly.bDungeonLocalTransform) {
					WorldPoint = DungeonTransform.TransformPosition(WorldPoint);
				}
				FVector LandscapePoint = WorldToLandscape.TransformPosition(WorldPoint);
				RasterData.Corners.Add(FVector2D(LandscapePoint));
			}
		}

		// Process circles
		for (const FDAShapeCircle& Circle : ChunkShape.Circles) {
			FDungeonCircleRasterData& RasterData = CircleRasterList.AddDefaulted_GetRef();
			RasterData.FalloffWidth = FalloffWidth;

			RasterData.Height = Circle.Transform.GetLocation().Z;
			if (Circle.bDungeonLocalTransform) {
				RasterData.Height += DungeonTransform.GetLocation().Z;
			}

			// Transform center to landscape space
			FVector WorldCenter = Circle.Transform.GetLocation();
			if (Circle.bDungeonLocalTransform) {
				WorldCenter = DungeonTransform.TransformPosition(WorldCenter);
			}
			FVector LandscapeCenter = WorldToLandscape.TransformPosition(WorldCenter);
			RasterData.Center = FVector2D(LandscapeCenter);

			// Transform radius to landscape space (use X scale assuming uniform XY scaling)
			const FVector RadiusVector = WorldToLandscape.TransformVector(FVector(Circle.Radius, 0, 0));
			RasterData.Radius = RadiusVector.Size();
		}
	}

	if (PolyRasterList.Num() == 0 && CircleRasterList.Num() == 0) {
		UE_LOG(LogLandscapeModifier, Warning, TEXT("No cells to rasterize"));
		return;
	}

	int32 InnerMinX = LandscapeMaxX, InnerMinY = LandscapeMaxY;
	int32 InnerMaxX = LandscapeMinX, InnerMaxY = LandscapeMinY;
	int32 MinX = LandscapeMaxX, MinY = LandscapeMaxY;
	int32 MaxX = LandscapeMinX, MaxY = LandscapeMinY;
	float MaxFalloffWidth = 0.0f;

	for (const FDungeonPolyRasterData& CellData : PolyRasterList) {
		const float Falloff = CellData.FalloffWidth;
		MaxFalloffWidth = FMath::Max(MaxFalloffWidth, Falloff);
		for (int32 i = 0; i < CellData.Corners.Num(); ++i) {
			InnerMinX = FMath::Min(InnerMinX, FMath::FloorToInt32(CellData.Corners[i].X));
			InnerMinY = FMath::Min(InnerMinY, FMath::FloorToInt32(CellData.Corners[i].Y));
			InnerMaxX = FMath::Max(InnerMaxX, FMath::CeilToInt32(CellData.Corners[i].X));
			InnerMaxY = FMath::Max(InnerMaxY, FMath::CeilToInt32(CellData.Corners[i].Y));
			MinX = FMath::Min(MinX, FMath::FloorToInt32(CellData.Corners[i].X - Falloff));
			MinY = FMath::Min(MinY, FMath::FloorToInt32(CellData.Corners[i].Y - Falloff));
			MaxX = FMath::Max(MaxX, FMath::CeilToInt32(CellData.Corners[i].X + Falloff));
			MaxY = FMath::Max(MaxY, FMath::CeilToInt32(CellData.Corners[i].Y + Falloff));
		}
	}

	// Include circles in bounds calculation
	for (const FDungeonCircleRasterData& CircleData : CircleRasterList) {
		const float Falloff = CircleData.FalloffWidth;
		const float R = CircleData.Radius;
		MaxFalloffWidth = FMath::Max(MaxFalloffWidth, Falloff);

		InnerMinX = FMath::Min(InnerMinX, FMath::FloorToInt32(CircleData.Center.X - R));
		InnerMinY = FMath::Min(InnerMinY, FMath::FloorToInt32(CircleData.Center.Y - R));
		InnerMaxX = FMath::Max(InnerMaxX, FMath::CeilToInt32(CircleData.Center.X + R));
		InnerMaxY = FMath::Max(InnerMaxY, FMath::CeilToInt32(CircleData.Center.Y + R));
		MinX = FMath::Min(MinX, FMath::FloorToInt32(CircleData.Center.X - R - Falloff));
		MinY = FMath::Min(MinY, FMath::FloorToInt32(CircleData.Center.Y - R - Falloff));
		MaxX = FMath::Max(MaxX, FMath::CeilToInt32(CircleData.Center.X + R + Falloff));
		MaxY = FMath::Max(MaxY, FMath::CeilToInt32(CircleData.Center.Y + R + Falloff));
	}

	MinX = FMath::Max(MinX, LandscapeMinX);
	MinY = FMath::Max(MinY, LandscapeMinY);
	MaxX = FMath::Min(MaxX, LandscapeMaxX);
	MaxY = FMath::Min(MaxY, LandscapeMaxY);
	InnerMinX = FMath::Clamp(InnerMinX, MinX, MaxX);
	InnerMinY = FMath::Clamp(InnerMinY, MinY, MaxY);
	InnerMaxX = FMath::Clamp(InnerMaxX, MinX, MaxX);
	InnerMaxY = FMath::Clamp(InnerMaxY, MinY, MaxY);

	if (MinX > MaxX || MinY > MaxY) {
		return;
	}

	const int32 DataWidth = 1 + MaxX - MinX;
	const int32 DataSize = (1 + MaxY - MinY) * DataWidth;

	TArray<uint16> BaseTerrainHeights;
	BaseTerrainHeights.AddZeroed(DataSize);
	{
		FLandscapeEditDataInterface BaseLandscapeEdit(LandscapeInfo);
		int32 BaseMinX = MinX, BaseMinY = MinY, BaseMaxX = MaxX, BaseMaxY = MaxY;
		BaseLandscapeEdit.GetHeightData(BaseMinX, BaseMinY, BaseMaxX, BaseMaxY, BaseTerrainHeights.GetData(), 0);
		if (BaseMinX != MinX || BaseMinY != MinY || BaseMaxX != MaxX || BaseMaxY != MaxY) {
			FLandscapeEditDataInterface::ShrinkData(BaseTerrainHeights, MinX, MinY, MaxX, MaxY, BaseMinX, BaseMinY,
			                                        BaseMaxX, BaseMaxY);
		}
	}

	FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, DungeonLayerGuid);
	TSet<ULandscapeComponent*> ModifiedComponents;

	{
		TArray<uint16> HeightData;
		HeightData.AddZeroed(DataSize);

		int32 ValidMinX = MinX;
		int32 ValidMinY = MinY;
		int32 ValidMaxX = MaxX;
		int32 ValidMaxY = MaxY;
		LandscapeEdit.GetHeightData(ValidMinX, ValidMinY, ValidMaxX, ValidMaxY, HeightData.GetData(), 0);

		if (ValidMinX > ValidMaxX || ValidMinY > ValidMaxY) {
			return;
		}

		FLandscapeEditDataInterface::ShrinkData(HeightData, MinX, MinY, MaxX, MaxY, ValidMinX, ValidMinY, ValidMaxX,
		                                        ValidMaxY);

		bool bIsEditingDungeonLayer = true;
		bool bCalculateNormals = false;

		TArray<uint16> HeightAlphaBlendData;
		TArray<uint8> HeightFlagsData;
		TArray<uint16>* HeightAlphaBlendDataPtr = nullptr;
		TArray<uint8>* HeightFlagsDataPtr = nullptr;

		if (bIsEditingDungeonLayer) {
			HeightAlphaBlendData.SetNumUninitialized(DataSize);
			for (int32 i = 0; i < DataSize; i++) {
				HeightAlphaBlendData[i] = LandscapeDataAccess::MaxValue;
			}
			int32 AlphaValidMinX = MinX;
			int32 AlphaValidMinY = MinY;
			int32 AlphaValidMaxX = MaxX;
			int32 AlphaValidMaxY = MaxY;
			LandscapeEdit.GetHeightAlphaBlendData(AlphaValidMinX, AlphaValidMinY, AlphaValidMaxX, AlphaValidMaxY,
			                                      HeightAlphaBlendData.GetData(), 0);
			FLandscapeEditDataInterface::ShrinkData(HeightAlphaBlendData, MinX, MinY, MaxX, MaxY, AlphaValidMinX,
			                                        AlphaValidMinY, AlphaValidMaxX, AlphaValidMaxY);
			HeightAlphaBlendDataPtr = &HeightAlphaBlendData;

			HeightFlagsData.AddZeroed(DataSize);
			int32 FlagsValidMinX = MinX;
			int32 FlagsValidMinY = MinY;
			int32 FlagsValidMaxX = MaxX;
			int32 FlagsValidMaxY = MaxY;
			LandscapeEdit.GetHeightFlagsData(FlagsValidMinX, FlagsValidMinY, FlagsValidMaxX, FlagsValidMaxY,
			                                 HeightFlagsData.GetData(), 0);
			FLandscapeEditDataInterface::ShrinkData(HeightFlagsData, MinX, MinY, MaxX, MaxY, FlagsValidMinX,
			                                        FlagsValidMinY, FlagsValidMaxX, FlagsValidMaxY);
			HeightFlagsDataPtr = &HeightFlagsData;
		}

		MinX = ValidMinX;
		MinY = ValidMinY;
		MaxX = ValidMaxX;
		MaxY = ValidMaxY;

		constexpr float BaseHeight = 0;
		const float BaseHeightValue = LandscapeDataAccess::GetTexHeight(
			(BaseHeight - LandscapeToWorld.GetTranslation().Z) / LandscapeToWorld.GetScale3D().Z);
		const uint16 BaseHeightUint = static_cast<uint16>(FMath::Clamp(BaseHeightValue, 0.0f, static_cast<float>(LandscapeDataAccess::MaxValue)));

		for (int32 i = 0; i < DataSize; i++) {
			HeightData[i] = BaseHeightUint;
			if (HeightAlphaBlendDataPtr) {
				(*HeightAlphaBlendDataPtr)[i] = 0;
			}
			if (HeightFlagsDataPtr) {
				(*HeightFlagsDataPtr)[i] = 3;
			}
		}

		const int32 HeightDataWidth = 1 + MaxX - MinX;

		TArray<bool> InsideMask;
		InsideMask.SetNumZeroed(DataSize);

		for (const FDungeonPolyRasterData& PolyData : PolyRasterList) {
			const float CellZ = PolyData.Height;
			const uint16 HeightValue = static_cast<uint16>(FMath::Clamp(
				static_cast<float>(LandscapeDataAccess::GetTexHeight(
					(CellZ - LandscapeToWorld.GetTranslation().Z) / LandscapeToWorld.GetScale3D().Z)),
				0.0f, static_cast<float>(LandscapeDataAccess::MaxValue)));

			ScanlineRasterizer::RasterizePolygonFan(PolyData.Corners, MinX, MinY, MaxX, MaxY,
			                                        [&HeightData, &InsideMask, &HeightDataWidth, MinX, MinY, HeightValue
			                                        ](int32 X, int32 Y)
			                                        {
				                                        const int32 Index = (Y - MinY) * HeightDataWidth + (X - MinX);
			                                        	if (InsideMask[Index]) {
			                                        		HeightData[Index] = FMath::Min(HeightData[Index], HeightValue);
			                                        	}
			                                        	else {
			                                        		HeightData[Index] = HeightValue;
															InsideMask[Index] = true;
			                                        	}
			                                        });
		}

		for (const FDungeonCircleRasterData& CircleData : CircleRasterList) {
			const float CellZ = CircleData.Height;
			const uint16 HeightValue = static_cast<uint16>(FMath::Clamp(
				static_cast<float>(LandscapeDataAccess::GetTexHeight(
					(CellZ - LandscapeToWorld.GetTranslation().Z) / LandscapeToWorld.GetScale3D().Z)),
				0.0f, static_cast<float>(LandscapeDataAccess::MaxValue)));

			ScanlineRasterizer::RasterizeFilledCircle(CircleData.Center, CircleData.Radius, MinX, MinY, MaxX, MaxY,
			                                          [&HeightData, &InsideMask, &HeightDataWidth, MinX, MinY, HeightValue
			                                          ](int32 X, int32 Y)
			                                          {
				                                          const int32 Index = (Y - MinY) * HeightDataWidth + (X - MinX);
				                                          if (InsideMask[Index]) {
					                                          HeightData[Index] = FMath::Min(HeightData[Index], HeightValue);
				                                          }
				                                          else {
					                                          HeightData[Index] = HeightValue;
					                                          InsideMask[Index] = true;
				                                          }
			                                          });
		}

		// Generate SDF for falloff blending and/or weight painting
		const int32 SDFWidth = HeightDataWidth;
		const int32 SDFHeight = 1 + MaxY - MinY;
		const int32 SDFSize = SDFWidth * SDFHeight;

		TArray<DA::SDF::FDungeonSDFPoint> SDFPoints;
		const bool bNeedSDF = (MaxFalloffWidth > 0.0f && HeightAlphaBlendDataPtr) || (PaintLayers.Num() > 0);
		if (bNeedSDF) {
			DA::SDF::GenerateSDF(SDFWidth, SDFHeight, InsideMask, SDFPoints);
		}

		if (MaxFalloffWidth > 0.0f && HeightAlphaBlendDataPtr && SDFPoints.Num() > 0) {

			TArray<uint16> H0;
			H0.SetNumUninitialized(SDFSize);
			for (int32 i = 0; i < SDFSize; ++i) {
				const FIntPoint& Seed = SDFPoints[i].NearestSeed;
				if (Seed.X >= 0 && Seed.Y >= 0) {
					const int32 SeedIndex = Seed.Y * SDFWidth + Seed.X;
					H0[i] = HeightData[SeedIndex];
				}
				else {
					H0[i] = BaseHeightUint;
				}
			}

			TArray<uint16> H1 = H0;
			if (BoundaryHeightBlurDistance > 0.0f && BoundaryHeightBlurRadius > 0) {
				const int32 KernelRadius = FMath::Clamp(BoundaryHeightBlurRadius, 1, 8);
				TArray<uint16> H1Temp;
				H1Temp.SetNumUninitialized(SDFSize);

				for (int32 Pass = 0; Pass < 4; ++Pass) {
					const TArray<uint16>& Src = (Pass % 2 == 0) ? H1 : H1Temp;
					TArray<uint16>& Dst = (Pass % 2 == 0) ? H1Temp : H1;

					
					ParallelFor(SDFSize, [&](int32 Index) {
						int32 X = Index % SDFWidth;
						int32 Y = Index / SDFWidth;
						if (SDFPoints[Index].SDF <= 0.0f) {
								Dst[Index] = Src[Index];
								return;
							}

							float Sum = 0.0f;
							int32 Count = 0;
							for (int32 KY = -KernelRadius; KY <= KernelRadius; ++KY) {
								for (int32 KX = -KernelRadius; KX <= KernelRadius; ++KX) {
									const int32 SampleX = FMath::Clamp(X + KX, 0, SDFWidth - 1);
									const int32 SampleY = FMath::Clamp(Y + KY, 0, SDFHeight - 1);
									const int32 SampleIndex = SampleY * SDFWidth + SampleX;
									Sum += Src[SampleIndex];
									++Count;
								}
							}
							Dst[Index] = static_cast<uint16>(Sum / Count);
					});
				}
			}
			
			float WorldBoundaryHeightBlurDistance = BoundaryHeightBlurDistance * WorldToLandscape.GetScale3D().X;

			ParallelFor(SDFSize, [&](int32 Index)
			{
				int32 X = Index % SDFWidth;
				int32 Y = Index / SDFWidth;
				
				const float Distance = SDFPoints[Index].SDF;

				if (Distance > 0.0f) {
					const float BlurBlendAlpha = FMath::Clamp(Distance / WorldBoundaryHeightBlurDistance, 0.0f, 1.0f);
					const float BoundaryHeight = FMath::Lerp(H0[Index], H1[Index], BlurBlendAlpha);

					float NormalizedDist = FMath::Clamp(Distance / MaxFalloffWidth, 0.0f, 1.0f);

					if (bEnableFalloffNoise && NormalizedDist > 0.0f) {
						const float WorldX = (X + MinX) * LandscapeToWorld.GetScale3D().X;
						const float WorldY = (Y + MinY) * LandscapeToWorld.GetScale3D().Y;

						constexpr float BASE_MULT_FALLOFF_NOISE_SCALE = 0.0005f;
						constexpr float BASE_MULT_FALLOFF_NOISE_AMP = 0.25;
						
						float FinalFalloffNoiseScale = FalloffNoiseScale * BASE_MULT_FALLOFF_NOISE_SCALE;
						float FinalFalloffNoiseAmp = FalloffNoiseAmplitude * BASE_MULT_FALLOFF_NOISE_AMP;
						
						float NoiseValue = 0.0f;
						float Amplitude = 1.0f;
						float Frequency = FinalFalloffNoiseScale;
						float MaxAmplitude = 0.0f;

						for (int32 Octave = 0; Octave < FalloffNoiseOctaves; ++Octave) {
							NoiseValue += FMath::PerlinNoise2D(FVector2D(WorldX * Frequency, WorldY * Frequency)) * Amplitude;
							MaxAmplitude += Amplitude;
							Amplitude *= 0.5f;
							Frequency *= 2.0f;
						}
						NoiseValue /= MaxAmplitude;

						float DistanceModulation = NormalizedDist * NormalizedDist;
						DistanceModulation = FMath::Abs(DistanceModulation * 2 - 1);
						NormalizedDist = FMath::Clamp(NormalizedDist + NoiseValue * FinalFalloffNoiseAmp * DistanceModulation, 0.0f, 1.0f);
					}

					float SmoothAlpha = NormalizedDist;

					if (FalloffCurve) {
						SmoothAlpha = FMath::Clamp(FalloffCurve->GetFloatValue(SmoothAlpha), 0.0f, 1.0f);
					}
					else {
						switch (FalloffEasing)
						{
						case ELandscapeFalloffEasing::Linear:
							break;
						case ELandscapeFalloffEasing::EaseIn:
							SmoothAlpha = FMath::Pow(SmoothAlpha, 2.0f * FalloffExponent);
							break;
						case ELandscapeFalloffEasing::EaseOut:
							SmoothAlpha = 1.0f - FMath::Pow(1.0f - SmoothAlpha, 2.0f * FalloffExponent);
							break;
						case ELandscapeFalloffEasing::EaseInOut:
							SmoothAlpha = SmoothAlpha < 0.5f
								? FMath::Pow(2.0f * SmoothAlpha, 2.0f * FalloffExponent) * 0.5f
								: 1.0f - FMath::Pow(2.0f * (1.0f - SmoothAlpha), 2.0f * FalloffExponent) * 0.5f;
							break;
						case ELandscapeFalloffEasing::SmoothStep:
							SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, SmoothAlpha);
							if (!FMath::IsNearlyEqual(FalloffExponent, 1.0f)) {
								SmoothAlpha = FMath::Pow(SmoothAlpha, FalloffExponent);
							}
							break;
						case ELandscapeFalloffEasing::SmootherStep:
							SmoothAlpha = SmoothAlpha * SmoothAlpha * SmoothAlpha * (SmoothAlpha * (SmoothAlpha * 6.0f - 15.0f) + 10.0f);
							if (!FMath::IsNearlyEqual(FalloffExponent, 1.0f)) {
								SmoothAlpha = FMath::Pow(SmoothAlpha, FalloffExponent);
							}
							break;
						}
						SmoothAlpha = FMath::Clamp(SmoothAlpha, 0.0f, 1.0f);
					}
					

					const uint16 BaseTerrainHeight = BaseTerrainHeights[Index];
					const float BlendedHeight = FMath::Lerp(BoundaryHeight, static_cast<float>(BaseTerrainHeight),
					                                        SmoothAlpha);
					HeightData[Index] = static_cast<uint16>(FMath::Clamp(
						BlendedHeight, 0.0f, static_cast<float>(LandscapeDataAccess::MaxValue)));

					const float BlendedAlpha = SmoothAlpha * static_cast<float>(LandscapeDataAccess::MaxValue);
					(*HeightAlphaBlendDataPtr)[Index] = static_cast<uint16>(FMath::Clamp(
						BlendedAlpha, 0.0f, static_cast<float>(LandscapeDataAccess::MaxValue)));
				}
			});
		}

		LandscapeEdit.SetHeightData(MinX, MinY, MaxX, MaxY, HeightData.GetData(), 0, bCalculateNormals, nullptr,
		                            HeightAlphaBlendDataPtr ? HeightAlphaBlendDataPtr->GetData() : nullptr,
		                            HeightFlagsDataPtr ? HeightFlagsDataPtr->GetData() : nullptr, false, nullptr,
		                            nullptr, true, !bIsEditingDungeonLayer);
		LandscapeEdit.GetComponentsInRegion(MinX, MinY, MaxX, MaxY, &ModifiedComponents);

		// Weight painting with SDF ranges
		if (PaintLayers.Num() > 0 && SDFPoints.Num() > 0) {
			const float LandscapeScale = LandscapeToWorld.GetScale3D().X;

			for (const FLandscapePaintLayerEntry& Entry : PaintLayers) {
				if (!Entry.LayerInfo || !Entry.SDFConfig) {
					continue;
				}

				float MinSDFWorld, MaxSDFWorld;
				Entry.SDFConfig->GetSDFRange(MinSDFWorld, MaxSDFWorld);

				// Convert to landscape space
				const float MinSDF = MinSDFWorld / LandscapeScale;
				const float MaxSDF = MaxSDFWorld / LandscapeScale;

				TArray<uint8> WeightData;
				WeightData.SetNumZeroed(SDFSize);

				// Initial paint pass
				ParallelFor(SDFSize, [&](int32 Index) {
					const float Distance = SDFPoints[Index].SDF;
					if (Distance >= MinSDF && Distance <= MaxSDF) {
						WeightData[Index] = Entry.Weight;
					}
				});

				// Apply blur
				const int32 BlurPasses = GetBlurPasses(Entry.BlurStrength);
				if (BlurPasses > 0) {
					const int32 KernelRadius = GetBlurKernelRadius(Entry.BlurSize);
					TArray<uint8> WeightTemp;
					WeightTemp.SetNumUninitialized(SDFSize);

					for (int32 Pass = 0; Pass < BlurPasses; ++Pass) {
						const TArray<uint8>& Src = (Pass % 2 == 0) ? WeightData : WeightTemp;
						TArray<uint8>& Dst = (Pass % 2 == 0) ? WeightTemp : WeightData;

						ParallelFor(SDFSize, [&](int32 Index) {
							const int32 X = Index % SDFWidth;
							const int32 Y = Index / SDFWidth;

							float Sum = 0.0f;
							int32 Count = 0;
							for (int32 KY = -KernelRadius; KY <= KernelRadius; ++KY) {
								for (int32 KX = -KernelRadius; KX <= KernelRadius; ++KX) {
									const int32 SampleX = FMath::Clamp(X + KX, 0, SDFWidth - 1);
									const int32 SampleY = FMath::Clamp(Y + KY, 0, SDFHeight - 1);
									Sum += Src[SampleY * SDFWidth + SampleX];
									++Count;
								}
							}
							Dst[Index] = static_cast<uint8>(Sum / Count);
						});
					}

					// Ensure result is in WeightData (even passes end in WeightTemp)
					if (BlurPasses % 2 != 0) {
						WeightData = WeightTemp;
					}
				}

				// Apply noise
				if (Entry.bEnableNoise && Entry.NoiseAmplitude > 0.0f) {
					constexpr float BASE_NOISE_SCALE = 0.001f;
					const float FinalNoiseScale = Entry.NoiseScale * BASE_NOISE_SCALE;

					ParallelFor(SDFSize, [&](int32 Index) {
						if (WeightData[Index] == 0) return;

						const int32 X = Index % SDFWidth;
						const int32 Y = Index / SDFWidth;
						const float WorldX = (X + MinX) * LandscapeToWorld.GetScale3D().X;
						const float WorldY = (Y + MinY) * LandscapeToWorld.GetScale3D().Y;

						float NoiseValue = 0.0f;
						float Amplitude = 1.0f;
						float Frequency = FinalNoiseScale;
						float MaxAmplitude = 0.0f;

						for (int32 Octave = 0; Octave < Entry.NoiseOctaves; ++Octave) {
							NoiseValue += FMath::PerlinNoise2D(FVector2D(WorldX * Frequency, WorldY * Frequency)) * Amplitude;
							MaxAmplitude += Amplitude;
							Amplitude *= 0.5f;
							Frequency *= 2.0f;
						}
						NoiseValue = NoiseValue / MaxAmplitude; // -1 to 1

						const float CurrentWeight = WeightData[Index];
						const float NoisedWeight = CurrentWeight + NoiseValue * Entry.NoiseAmplitude * 255.0f;
						WeightData[Index] = static_cast<uint8>(FMath::Clamp(NoisedWeight, 0.0f, 255.0f));
					});
				}

				LandscapeEdit.SetAlphaData(Entry.LayerInfo, MinX, MinY, MaxX, MaxY, WeightData.GetData(), 0);
			}
		}
	}

	LandscapeEdit.Flush();

	for (ULandscapeComponent* Component : ModifiedComponents) {
		Component->RequestHeightmapUpdate();
		Component->RequestWeightmapUpdate();
	}
}


#endif	// WITH_EDITOR

