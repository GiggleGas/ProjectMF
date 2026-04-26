#include "ForceDirectedGraphActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include <Kismet/KismetRenderingLibrary.h>

AForceDirectedGraphActor::AForceDirectedGraphActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AForceDirectedGraphActor::BeginPlay()
{
    Super::BeginPlay();

    // Generate test data
    GenerateTestData();

    // Initialize the hierarchical graph
    RootGraph = MakeShared<FForceDirectedGraph>();
    RootGraph->Initialize(RootNodes, RootEdges);
    RootGraph->SetCanvasSize(CanvasWidth, CanvasHeight);
    RootGraph->SetCenterPull(CenterPull);
    RootGraph->SetRepulsion(Repulsion);
    RootGraph->SetSpringStrength(SpringStrength);
    RootGraph->SetSpringLength(SpringLength);
    RootGraph->SetDamping(Damping);
    RootGraph->SetPosition(FVector2D(CanvasWidth / 2.0f, CanvasHeight / 2.0f));

    // Create and initialize child graphs
    ChildGraphs.Empty();
    for (int i = 0; i < ChildNodesList.Num(); i++)
    {
        TSharedPtr<FForceDirectedGraph> ChildGraph = MakeShared<FForceDirectedGraph>();
        ChildGraph->Initialize(ChildNodesList[i], ChildEdgesList[i]);
        ChildGraph->SetCanvasSize(CanvasWidth, CanvasHeight);
        ChildGraph->SetCenterPull(CenterPull);
        ChildGraph->SetRepulsion(Repulsion);
        ChildGraph->SetSpringStrength(SpringStrength);
        ChildGraph->SetSpringLength(SpringLength);
        ChildGraph->SetDamping(Damping);
        ChildGraph->SetPosition(FVector2D(
                FMath::FRandRange(50.0f, CanvasWidth - 50.0f),
                FMath::FRandRange(50.0f, CanvasHeight - 50.0f)
            ));

        RootGraph->AddChild(ChildGraph);
        ChildGraphs.Add(ChildGraph);
    }
}

void AForceDirectedGraphActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update the root graph (this will recursively update all child graphs)
    if (RootGraph.IsValid())
    {
        RootGraph->Update(DeltaTime);
    }

    // Draw the graph to the render target
    DrawToRenderTarget();
}

void AForceDirectedGraphActor::GenerateTestData()
{
    CreateTestHierarchicalGraph();
}

void AForceDirectedGraphActor::CreateTestHierarchicalGraph()
{
    // Clear existing data
    RootNodes.Empty(); // Root graph has no nodes
    RootEdges.Empty(); // Root graph has no edges
    ChildNodesList.Empty();
    ChildEdgesList.Empty();

    // Create child graphs
    const int ChildGraphCount = 3;
    const int NodesPerChild = NodeCount / ChildGraphCount;

    // Define colors for each child graph
    TArray<FLinearColor> ChildColors = {
        FLinearColor(0.0f, 0.7f, 1.0f, 1.0f),   // Blue
        FLinearColor(0.0f, 1.0f, 0.5f, 1.0f),   // Teal
        FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)    // Orange
    };

    for (int g = 0; g < ChildGraphCount; g++)
    {
        TArray<FForceDirectedGraph::FNode> ChildNodes;
        TArray<FForceDirectedGraph::FEdge> ChildEdges;

        // Create child nodes with random positions
        int BaseId = g * NodesPerChild;
        for (int i = 0; i < NodesPerChild; i++)
        {
            FForceDirectedGraph::FNode Node;
            Node.Id = BaseId + i;
            // Generate completely random positions within the canvas
            Node.Position = FVector2D(
                FMath::FRandRange(50.0f, CanvasWidth - 50.0f),
                FMath::FRandRange(50.0f, CanvasHeight - 50.0f)
            );
            Node.Weight = 1.0f + FMath::FRand();
            Node.Color = ChildColors[g]; // Assign color based on child graph index
            ChildNodes.Add(Node);
        }

        // Create child edges (cycle)
        for (int i = 0; i < NodesPerChild; i++)
        {
            ChildEdges.Add({BaseId + i, BaseId + (i + 1) % NodesPerChild});
        }

        // Add some random edges
        for (int i = 0; i < NodesPerChild / 2; i++)
        {
            int Source = BaseId + FMath::RandRange(0, NodesPerChild - 1);
            int Target = BaseId + FMath::RandRange(0, NodesPerChild - 1);
            if (Source != Target)
            {
                ChildEdges.Add({Source, Target});
            }
        }

        ChildNodesList.Add(ChildNodes);
        ChildEdgesList.Add(ChildEdges);
    }
}

void AForceDirectedGraphActor::DrawToRenderTarget()
{
    if (!RenderTarget || !RootGraph.IsValid()) return;

    // clear
    UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget);

    FDrawToRenderTargetContext Context;
    UCanvas* Canvas;
    FVector2D Size;

    // 获取Canvas和渲染上下文
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
    if (!Canvas) return;

    // Draw the entire graph hierarchy
    DrawGraphHierarchy(Canvas, RootGraph);

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

void AForceDirectedGraphActor::DrawGraphHierarchy(UCanvas* Canvas, TSharedPtr<FForceDirectedGraph> GraphToDraw, int Depth)
{
    if (!Canvas || !GraphToDraw.IsValid()) return;

    // Get the current nodes
    const TArray<FForceDirectedGraph::FNode>& CurrentNodes = GraphToDraw->GetNodes();

    // Draw edges for this graph
    // For demonstration, we'll create a map of node positions for edge drawing
    TMap<int, FVector2D> NodePositions;
    for (const FForceDirectedGraph::FNode& Node : CurrentNodes)
    {
        NodePositions.Add(Node.Id, Node.Position);
    }

    // Draw edges using the graph's own edge data
    const TArray<FForceDirectedGraph::FEdge>& Edges = GraphToDraw->GetEdges();
    for (const FForceDirectedGraph::FEdge& Edge : Edges)
    {
        if (NodePositions.Contains(Edge.SourceId) && NodePositions.Contains(Edge.TargetId))
        {
            FVector2D SourcePos = NodePositions[Edge.SourceId];
            FVector2D TargetPos = NodePositions[Edge.TargetId];
            
            // Use a slightly darker version of the node color for edges
            FLinearColor EdgeColor = FLinearColor::Black;
            if (NodePositions.Contains(Edge.SourceId))
            {
                // Find the node to get its color
                for (const FForceDirectedGraph::FNode& Node : CurrentNodes)
                {
                    if (Node.Id == Edge.SourceId)
                    {
                        EdgeColor = Node.Color * 0.7f; // Darken the color
                        break;
                    }
                }
            }
            
            Canvas->K2_DrawLine(SourcePos, TargetPos, 1.0f, EdgeColor);
        }
    }

    // Draw nodes using their assigned color
    for (const FForceDirectedGraph::FNode& Node : CurrentNodes)
    {
        float Radius = 10.0f * Node.Weight;
        Canvas->K2_DrawBox(Node.Position - Radius / 2, FVector2D{ Radius }, Radius / 2, Node.Color);
    }

    // Draw child graphs
    for (TSharedPtr<FForceDirectedGraph> ChildGraph : GraphToDraw->GetChildren())
    {
        if (ChildGraph.IsValid())
        {
            // Draw a connection between this graph and the child graph
            FVector2D ChildPosition = ChildGraph->GetPosition();
            FLinearColor ConnectionColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
            Canvas->K2_DrawLine(GraphToDraw->GetPosition(), ChildPosition, 1.0f, ConnectionColor);
            
            // Draw the child graph
            DrawGraphHierarchy(Canvas, ChildGraph, Depth + 1);
        }
    }
}