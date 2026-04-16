#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapTypes.h"
#include "Graph.h"
#include "IslandShape.h"
#include "MapGenerator.generated.h"

/**
 * AMapGenerator - Main actor for generating procedural maps
 * This is the Unreal equivalent of Map.cs from the original C# implementation
 *
 * Usage:
 *   AMapGenerator* MapGen = GetWorld()->SpawnActor<AMapGenerator>();
 *   MapGen->SetPointCount(500);
 *   MapGen->GenerateMap();
 */


 // Island shape selection
UENUM(BlueprintType)
enum class EIslandType : uint8
{
	Perlin = 0,
	Radial = 1,
	Square = 2
};

UCLASS()
class PROJECTMF_API AMapGenerator : public AActor
{
	GENERATED_BODY()

public:
	AMapGenerator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	int32 PointCount = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	float LakeThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	float MapWidth = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	float MapHeight = 50.0f;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	EIslandType IslandType = EIslandType::Perlin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	int32 LloydRelaxations = 2;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	UTextureRenderTarget2D* RenderTarget;

	// Generate the map
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void GenerateMap();

	// Accessors
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	int32 GetCenterCount() const { return Graph ? Graph->GetCenters().Num() : 0; }

	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	int32 GetCornerCount() const { return Graph ? Graph->GetCorners().Num() : 0; }

	// UFUNCTION(BlueprintCallable, Category = "Map Generation")
	const TArray<FNMCenter>& GetCenters() const;

	// UFUNCTION(BlueprintCallable, Category = "Map Generation")
	const TArray<FNMCorner>& GetCorners() const;

	// UFUNCTION(BlueprintCallable, Category = "Map Generation")
	const TArray<FNMEdge>& GetEdges() const;

	// Get biome at a specific location
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	ENMBiome GetBiomeAtLocation(FVector Location) const;

private:
	// The generated map
	TUniquePtr<FNMGraph> Graph;

	// Island check function
	TFunction<bool(FVector)> IslandChecker;

	// Get island check function based on selected type
	TFunction<bool(FVector)> GetIslandChecker() const;

	// Generate random points for Voronoi
	TArray<FVector> GenerateRandomPoints3D(int32 Count);
	TArray<FVector2D> GenerateRandomPoints2D(int32 Count);
};