#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KKLayout/KKLayoutManager.h"
#include "TaskSystem/TaskSystemDataAsset.h"
#include <VNGraph.h>

#include "KKLayoutGraphActor.generated.h"

UCLASS()
class PROJECTMF_API AKKLayoutGraphActor : public AActor
{
    GENERATED_BODY()

public:
    AKKLayoutGraphActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    int32 NodeCount = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    int32 GroupCount = 5; // Number of groups/graphs

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float CanvasWidth = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float CanvasHeight = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float K = 1000.0f; // Global spring constant

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float IdealEdgeLength = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float Tolerance = 0.001f; // Gradient convergence tolerance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float EnergyTolerance = 0.01f; // Energy change convergence tolerance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    int32 MaxIterations = 10; // Max iterations per node

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    int32 MaxGlobalIterations = 1000; // Max global iterations

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    float Scale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamada-Kawai Layout")
    UTextureRenderTarget2D* RenderTarget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voronoi")
    UTextureRenderTarget2D* RTBioronoi;

    // Generate test data
    UFUNCTION(BlueprintCallable, Category = "Kamada-Kawai Layout")
    void GenerateTestData();

    // Draw the graph to the render target
    UFUNCTION(BlueprintCallable, Category = "Kamada-Kawai Layout")
    void DrawToRenderTarget();

    // Run full layout solve
    UFUNCTION(BlueprintCallable, Category = "Kamada-Kawai Layout")
    void SolveLayout();

    // Draw Voronoi diagram
    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void BakeAndDrawVoronoiDiagram();

private:
    // KKLayoutManager instance for data management
    FKKLayoutManager LayoutManager;
    
    // Whether to run layout update
    bool bUpdateLayout = true;

   // Load task system data and generate nodes
    void LoadTaskSystemAndGenerateNodes();
    
    // Generate nodes from task system data
    void GenerateNodesFromTaskSystem(UTaskSystemDataAsset* TaskSystemData);
    
    // Create nodes from a task set
    void CreateNodesFromTaskSet(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& OutGroupNodeIds);
    
    // Create nodes for a single task
    void CreateNodesForTask(FTaskDefinition* Task, UTaskSystemDataAsset* TaskSystemData, int GroupIndex, int TotalGroups, TArray<FString>& OutNodeIds);
    
    // Generate a random position for a node within its group's region
    FVector2D GenerateNodePosition(int GroupIndex, int TotalGroups);
    
    // Create edges for a single task
    void CreateEdgesForTask(FTaskDefinition* Task, const TArray<FString>& NodeIds, const FString& HubNodeId);
    
    // Create chain edges between nodes
    void CreateChainEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds);
    
    // Create edges within each group
    void CreateEdgesWithinGroups(TArray<TArray<FString>>& GroupNodeIds);
    
    // Create test data in LayoutManager
    void CreateTestData();
    
    // Create random edges between nodes
    void CreateRandomEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, int EdgeCount);
    
    // Create a cycle of edges between nodes
    void CreateCycleEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds);
    
    // Create star-shaped edges distribution (with center node index)
    void CreateStarEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, int CenterNodeIndex = 0);
    
    // Create star-shaped edges distribution (with center node id)
    void CreateStarEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, const FString& CenterNodeId);
    
    // Connect groups using tree structure
    void ConnectGroups(FKKLayoutManager& InManager, const TArray<TArray<FString>>& GroupNodeIds, int NodesPerGroup);
    
    void ConnectTaskGroups(FKKLayoutManager& InManager, const TArray<TArray<FString>>& GroupNodeIds, UTaskSystemDataAsset* TaskSystemData);
    
    // Add background room nodes for each task
    void AddBackgroundRoomNodes(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& GroupNodeIds);

    // Add cove room nodes for nodes with at most 1 edge
    void AddCoveRoomNodes(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& GroupNodeIds);

// Generate a random position for a background node
FVector2D GenerateBackgroundNodePosition(int GroupIndex, int TotalGroups);
    // Draw the graph
    void DrawGraph(UCanvas* Canvas);


    // voronoi diagram
    // VNGraph instance created from layout nodes
    TSharedPtr<FVNGraph> VNGraph;
    
    // Create VNGraph from LayoutManager nodes
    void CreateVNGraphFromLayoutManager();
    
    // Generate Voronoi corner points from node positions
    TArray<TArray<FVector2D>> GenerateCornerPoints(TArray<FVector2D>& InOutPoints, float Width, float Height, int LloydRelaxations);

    void DrawVoronoiDiagram();
    
};

