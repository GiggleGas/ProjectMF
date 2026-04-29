#include "ForceDirectedGraphActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include <Kismet/KismetRenderingLibrary.h>
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "WorldSim.h"

AForceDirectedGraphActor::AForceDirectedGraphActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AForceDirectedGraphActor::BeginPlay()
{
    Super::BeginPlay();

    // Create test data in WorldSim
    CreateTestData();

    // Set force-directed parameters from actor properties
    FForceDirectedParams Params;
    Params.CenterPull = CenterPull;
    Params.Repulsion = Repulsion;
    Params.SpringStrength = SpringStrength;
    Params.SpringLength = SpringLength;
    Params.Damping = Damping;
    Params.EdgeRepulsion = EdgeRepulsion;
    WorldSim.SetForceDirectedParams(Params);
    
    // Set canvas size
    WorldSim.SetCanvasSize(CanvasWidth, CanvasHeight);

    // Generate ForceDirectedGraph data using the composite function
    WorldSim.GenerateForceDirectedData();
    
    // Force-directed graph instances
    const TArray< FWorldSimGraph>& Nodes = WorldSim.GetNodes();
    if (Nodes.IsEmpty())
    {

    }
    else
    {
        // Log information about each graph
        for (auto& index : Nodes[0].ChildrenIndex)
        {
            UE_LOG(LogTemp, Log, TEXT("Graph %s: %d nodes, %d edges"), *Nodes[index].Id, Nodes[index].GetNodeNum(), Nodes[index].GetEdgeNum());
        }
    }
}

void AForceDirectedGraphActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update force-directed graphs using WorldSim
    FVector2D GraphPosition = FVector2D(CanvasWidth / 2.0f, CanvasHeight / 2.0f);
    WorldSim.UpdateForceDirectedGraphs(DeltaTime, GraphPosition, UpdateMode);
    
    // Draw the graph to the render target
    DrawToRenderTarget();
}

void AForceDirectedGraphActor::GenerateTestData()
{
    CreateTestData();
}

void AForceDirectedGraphActor::CreateRandomEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds, int EdgeCount)
{
    // Create random edges between nodes
    for (int i = 0; i < EdgeCount; i++)
    {
        int SourceIndex = FMath::RandRange(0, NodeIds.Num() - 1);
        int TargetIndex = FMath::RandRange(0, NodeIds.Num() - 1);
        if (SourceIndex != TargetIndex)
        {
            InWorldSim.AddLink(NodeIds[SourceIndex], NodeIds[TargetIndex]);
        }
    }
}

void AForceDirectedGraphActor::CreateCycleEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds)
{
    // Create a cycle of edges between nodes
    for (int i = 0; i < NodeIds.Num(); i++)
    {
        FString SourceNodeId = NodeIds[i];
        FString TargetNodeId = NodeIds[(i + 1) % NodeIds.Num()];
        InWorldSim.AddLink(SourceNodeId, TargetNodeId);
    }
}

void AForceDirectedGraphActor::CreateStarEdges(FWorldSim& InWorldSim, const TArray<FString>& NodeIds, int CenterNodeIndex)
{
    // Create star-shaped edges distribution
    if (NodeIds.Num() == 0)
    {
        return;
    }
    
    FString CenterNodeId = NodeIds[FMath::Clamp(CenterNodeIndex, 0, NodeIds.Num() - 1)];
    
    for (int i = 0; i < NodeIds.Num(); i++)
    {
        if (i != CenterNodeIndex)
        {
            InWorldSim.AddLink(CenterNodeId, NodeIds[i]);
        }
    }
}

void AForceDirectedGraphActor::CreateTestData()
{
    // Create test data in WorldSim
    
    // Create more graphs (increased from 3 to 5)
    const int GraphCount = 5;
    const int NodesPerGraph = NodeCount / GraphCount;
    
    // Define colors for each graph
    TArray<FColor> GraphColors = {
        FColor(0, 179, 255, 255),   // Blue
        FColor(0, 255, 127, 255),   // Teal
        FColor(255, 127, 0, 255),   // Orange
        FColor(255, 0, 127, 255),   // Pink
        FColor(127, 0, 255, 255)    // Purple
    };
    
    for (int g = 0; g < GraphCount; g++)
    {

        FString GraphId = FString::Printf(TEXT("Graph_%d"), g);
        FColor Color = GraphColors[g % GraphColors.Num()];
        // add graph to root
        WorldSim.AddChild(
            "Root",
            GraphId,
            "value",
            Color,
            TEXT("Node"),
            TEXT("TestNode")
        );

        TArray<FString> NodeIds;
        
        // Create nodes for this graph
        for (int i = 0; i < NodesPerGraph; i++)
        {
            FString NodeId = FString::Printf(TEXT("Node_%d_%d"), g, i);
            
            // Add child (creates graph if it doesn't exist, and adds node to the graph)
            WorldSim.AddChild(
                GraphId, 
                NodeId, 
                FString::Printf(TEXT("Node %d-%d"), g, i),
                Color,
                TEXT("Node"),
                TEXT("TestNode")
            );
            
            NodeIds.Add(NodeId);
        }
        
        // Randomly choose an edge generation function
        int EdgeFunction = FMath::RandRange(0, 1);
        switch (EdgeFunction)
        {
        case 0:
            // Create cycle edges
            CreateCycleEdges(WorldSim, NodeIds);
            break;
        case 1:
            // Create star edges
            CreateStarEdges(WorldSim, NodeIds);
            break;
        case 2:
            // Create random edges
            CreateRandomEdges(WorldSim, NodeIds, NodesPerGraph * 2);
            break;
        }
    }
    
    // Add external links between graphs to form a tree structure using random selection
    // Create GraphCount-1 edges to connect all graphs in a tree
    if (GraphCount > 1)
    {
        // Create set A (unconnected graphs) and set B (connected graphs)
        TArray<int> SetA; // Unconnected graphs
        TArray<int> SetB; // Connected graphs
        
        // Initialize set A with all graph indices
        for (int i = 0; i < GraphCount; i++)
        {
            SetA.Add(i);
        }
        
        // Randomly select an initial graph for set B
        int InitialGraphIndex = FMath::RandRange(0, SetA.Num() - 1);
        SetB.Add(SetA[InitialGraphIndex]);
        SetA.RemoveAt(InitialGraphIndex);
        
        // Connect remaining graphs in set A to set B
        while (SetA.Num() > 0)
        {
            // Randomly select a graph from set A
            int AIndex = FMath::RandRange(0, SetA.Num() - 1);
            int GraphA = SetA[AIndex];
            
            // Randomly select a graph from set B
            int BIndex = FMath::RandRange(0, SetB.Num() - 1);
            int GraphB = SetB[BIndex];
            
            // Select random nodes from each graph
            int NodeAIndex = FMath::RandRange(0, NodesPerGraph - 1);
            int NodeBIndex = FMath::RandRange(0, NodesPerGraph - 1);
            
            FString NodeAId = FString::Printf(TEXT("Node_%d_%d"), GraphA, NodeAIndex);
            FString NodeBId = FString::Printf(TEXT("Node_%d_%d"), GraphB, NodeBIndex);
            WorldSim.AddExternalLink(NodeAId, NodeBId);
            
            // Move the selected graph from set A to set B
            SetB.Add(GraphA);
            SetA.RemoveAt(AIndex);
        }
    }
    
    // Add new nodes connected to existing nodes
    AddNewNodesToExistingNodes(WorldSim, GraphColors);
    
    // Add new nodes to nodes with fewer than 2 edges
    AddNewNodesToNodesWithFewEdges(WorldSim, GraphColors);
}


void AForceDirectedGraphActor::DrawToRenderTarget()
{
    if (!RenderTarget ) return;

    if (WorldSim.GetNodes().IsEmpty()) return;

    // clear
    UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget);

    FDrawToRenderTargetContext Context;
    UCanvas* Canvas;
    FVector2D Size;

    // 获取Canvas和渲染上下文
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
    if (!Canvas) return;

    // Draw the graph
    DrawGraph(Canvas);

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}


void AForceDirectedGraphActor::AddNewNodesToExistingNodes(FWorldSim& InWorldSim, const TArray<FColor>& GraphColors)
{
    // Add 1 to 2 new nodes connected to each existing node in each graph
    int NewNodeCounter = 0;
    
    const TArray< FWorldSimGraph>& Nodes = WorldSim.GetNodes();
    if (Nodes.IsEmpty()) return;

    // Log information about each graph
    // Get all graphs from WorldSim
    Nodes[0].ForEachWorldSimGraph([&](const FWorldSimGraph*, const FWorldSimGraph* Graphp, int GraphIndex)
    {
        const FWorldSimGraph& Graph = *Graphp;
        const FString& GraphId = Graph.Id;
        
        // Get color for this graph
        FColor BaseColor = GraphColors[GraphIndex % GraphColors.Num()];
        
        // Create a darker color for new nodes
        FColor NewNodeColor = FColor(
            FMath::Max(0, BaseColor.R - 50),
            FMath::Max(0, BaseColor.G - 50),
            FMath::Max(0, BaseColor.B - 50),
            BaseColor.A
        );
        
        // Get existing nodes in this graph
        // copy
        TArray<int> ExistingNodeIds = Graph.ChildrenIndex;

        // For each existing node, add 0 to 2 new nodes
        for (int ExistingNodeId : ExistingNodeIds)
        {
            int NumNewNodes = FMath::RandRange(1, 2);
            for (int i = 0; i < NumNewNodes; i++)
            {
                FString NewNodeId = FString::Printf(TEXT("NewNode_%d"), NewNodeCounter++);
                
                // Add new node to the graph with darker color
                InWorldSim.AddChild(
                    GraphId, 
                    NewNodeId, 
                    FString::Printf(TEXT("New Node %d"), NewNodeCounter - 1),
                    NewNodeColor,
                    TEXT("Node"),
                    TEXT("NewTestNode")
                );
                
                // Connect new node to existing node
                InWorldSim.AddLink(Nodes[ExistingNodeId].Id, NewNodeId);
            }
        }
    });
}

void AForceDirectedGraphActor::AddNewNodesToNodesWithFewEdges(FWorldSim& InWorldSim, const TArray<FColor>& GraphColors)
{
    // Add new nodes to nodes with fewer than 2 edges
    int NewNodeCounter = 0;
    
    const TArray< FWorldSimGraph>& Nodes = WorldSim.GetNodes();
    if (Nodes.IsEmpty()) return;
    
    Nodes[0].ForEachWorldSimGraph([&](const FWorldSimGraph*, const FWorldSimGraph* Graphp, int)
    {
        const FWorldSimGraph& Graph = *Graphp;
        const FString& GraphId = Graph.Id;
        
        // Use fixed gray color for new nodes
        FColor NewNodeColor = FColor(128, 128, 128, 255); // Gray color
        
        // Get existing nodes in this graph
        // copy
        TArray<int> ExistingNodeIds = Graph.ChildrenIndex;
        
        // For each existing node, check edge count and add new node if needed
        for (int ExistingNodeId : ExistingNodeIds) // nodes
        {
            // Count edges for this node
            int EdgeCount = Nodes[ExistingNodeId].InternalLinksIndex.Num();
            
            
            // If edge count is less than 2, add a new node with 50% probability
            if (EdgeCount < 2 && FMath::FRand() < 0.5f)
            {
                FString NewNodeId = FString::Printf(TEXT("NewNodeExtra_%d"), NewNodeCounter++);
                
                // Add new node to the graph with darker color
                InWorldSim.AddChild(
                    GraphId, 
                    NewNodeId, 
                    FString::Printf(TEXT("New Node Extra %d"), NewNodeCounter - 1),
                    NewNodeColor,
                    TEXT("Node"),
                    TEXT("NewTestNode")
                );
                
                // Connect new node to existing node
                InWorldSim.AddLink(Nodes[ExistingNodeId].Id, NewNodeId);
            }
        }
    });
}

void AForceDirectedGraphActor::DrawGraph(UCanvas* Canvas)
{
    if (!Canvas) return;

    const TArray<FWorldSimGraph>& WorldSimNodes = WorldSim.GetNodes();
    if (WorldSimNodes.IsEmpty()) return;

    // Calculate canvas center for relative scaling
    FVector2D CanvasCenter(CanvasWidth / 2, CanvasHeight / 2);

    // Helper function to apply scale transformation relative to center
    auto ApplyScale = [&](const FVector2D& Position) -> FVector2D
    {
        // Translate to origin relative to center, scale, then translate back
        return (Position - CanvasCenter) * Scale + CanvasCenter;
    };

    // Draw edges for all graphs with node colors
    WorldSimNodes[0].ForEachWorldSimGraph([&](const FWorldSimGraph*,const FWorldSimGraph* Graph, int)
    {
        Graph->ForEachInternalLink([&](const FWorldSimGraph*, const FWorldSimGraph* Source, const FWorldSimGraph* Target, int)
        {
            FVector2D SourcePos = ApplyScale(Source->FDNode.Position);
            FVector2D TargetPos = ApplyScale(Target->FDNode.Position);
            
            // Use the source node's color for edges
            FLinearColor EdgeColor = Source->Color * 0.7f; // Darken the color for edges

            Canvas->K2_DrawLine(SourcePos, TargetPos, 1.0f, EdgeColor);
        });

        // Draw external links
        Graph->ForEachExternalLink([&](const FWorldSimGraph*, const FWorldSimGraph* Source, const FWorldSimGraph* Target, int)
        {
            FVector2D SourcePos = ApplyScale(Source->FDNode.Position);
            FVector2D TargetPos = ApplyScale(Target->FDNode.Position);
            
            // Use the source node's color for edges
            FLinearColor EdgeColor = Source->Color * 0.7f; // Darken the color for edges

            Canvas->K2_DrawLine(SourcePos, TargetPos, 1.0f, EdgeColor);
        });
    });

    // Draw nodes with their own color
    WorldSimNodes[0].ForEachWorldSimGraph([&](const FWorldSimGraph*, const FWorldSimGraph* Graph, int NodeIndex)
    {
        Graph->ForEachWorldSimGraph([&](const FWorldSimGraph*, const FWorldSimGraph* Node, int NodeIndex)
        {
            const FWorldSimGraph& node = *Node;

            float Radius = 10.0f * node.FDNode.Weight*Scale;

            // Apply scale to node position
            FVector2D ScaledPosition = ApplyScale(node.FDNode.Position);

            // Use the node's own color from FWorldSimNode
            FLinearColor NodeColor = node.Color; // Default color
            Canvas->K2_DrawBox(ScaledPosition - Radius / 2, FVector2D{ Radius }, Radius / 2, NodeColor);
        });
    }); 
}