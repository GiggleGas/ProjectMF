#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForceDirectedGraph/ForceDirectedGraph.h"
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
    int32 NodeCount = 50; // Increased default node count

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
    UTextureRenderTarget2D* RenderTarget;

    // Generate test data
    UFUNCTION(BlueprintCallable, Category = "Force Directed Graph")
    void GenerateTestData();

    // Draw the graph to the render target
    UFUNCTION(BlueprintCallable, Category = "Force Directed Graph")
    void DrawToRenderTarget();

private:
    // Force-directed graph instances (hierarchical)
    TSharedPtr<FForceDirectedGraph> RootGraph;
    TArray<TSharedPtr<FForceDirectedGraph>> ChildGraphs;

    // Test data
    TArray<FForceDirectedGraph::FNode> RootNodes;
    TArray<FForceDirectedGraph::FEdge> RootEdges;
    TArray<TArray<FForceDirectedGraph::FNode>> ChildNodesList;
    TArray<TArray<FForceDirectedGraph::FEdge>> ChildEdgesList;

    // Create sample nodes and edges for hierarchical graph
    void CreateTestHierarchicalGraph();

    // Draw a graph and its children
    void DrawGraphHierarchy(UCanvas* Canvas, TSharedPtr<FForceDirectedGraph> GraphToDraw, int Depth = 0);
};