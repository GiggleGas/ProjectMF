#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NMGraph.h"
#include "IslandShape.h"
#include <PaperTileMapActor.h>
#include "PaperTileSet.h"
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
	APaperTileMapActor* TileMapActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	UTextureRenderTarget2D* RTBiome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	UTextureRenderTarget2D* RTBiomeCopy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	TMap<ENMBiome, TObjectPtr<UPaperTileSet>>  BiomeTileSets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
	TObjectPtr<UPaperTileSet>  BiomeTileSet_Global;

	// Generate the map
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void GenerateMap();

	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void DrawToRT();

	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void PullFromRT();

	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void UpdateTileMap();

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

	// Generate random points for Voronoi
	static TArray<FVector2D> GenerateRandomPoints2D(int32 Count, float Width, float Height);

	// generate voronoi corners 
	static TArray<TArray<FVector2D>>  GenerateCornerPoints(TArray<FVector2D>& InOutPoints, float Width, float Height, int LloydRelaxations = 2);

	// Get island check function based on selected type
	TFunction<bool(FVector2D)> GetIslandChecker() const;
private:
	// The generated map
	TUniquePtr<FNMGraph> Graph;

	// Island check function
	TFunction<bool(FVector2D)> IslandChecker;

	TArray<ENMBiome> BiomeMap;


};