#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "WorldSim.h"
#include "ForceDirectedGraphActor.generated.h"

UCLASS()
class PROJECTMF_API AForceDirectedGraphActor : public AActor
{
    GENERATED_BODY()

public:
    AForceDirectedGraphActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    int32 NodeCount = 30; // Increased default node count

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float CanvasWidth = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float CanvasHeight = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float CenterPull = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float Repulsion = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float SpringStrength = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float SpringLength = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float Damping = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float EdgeRepulsion = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    float Scale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    EWorldSimUpdateMode UpdateMode = EWorldSimUpdateMode::FullUpdate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Directed Graph")
    UTextureRenderTarget2D* RenderTarget;

    // Generate test data
    UFUNCTION(BlueprintCallable, Category = "Force Directed Graph")
    void GenerateTestData();

    // Draw the graph to the render target
    UFUNCTION(BlueprintCallable, Category = "Force Directed Graph")
    void DrawToRenderTarget();

private:
    // WorldSim instance for data management
    FWorldSim WorldSim;
    
    
    // Create test data in WorldSim
    void CreateTestData();
    
    // Create random edges between nodes
    void CreateRandomEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds, int EdgeCount);
    
    // Create a cycle of edges between nodes
    void CreateCycleEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds);
    
    // Create star-shaped edges distribution
    void CreateStarEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds, int CenterNodeIndex = 0);
    
    // Add new nodes connected to existing nodes
    void AddNewNodesToExistingNodes(FWorldSim& InWorldSim, const TArray<FColor>& GraphColors);
    
    // Add new nodes to nodes with fewer than 2 edges
    void AddNewNodesToNodesWithFewEdges(FWorldSim& InWorldSim, const TArray<FColor>& GraphColors);
    
    // Draw the graph
    void DrawGraph(UCanvas* Canvas);
};