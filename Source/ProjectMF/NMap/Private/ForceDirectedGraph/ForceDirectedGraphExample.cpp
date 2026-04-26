#include "ForceDirectedGraph/ForceDirectedGraphExample.h"
#include "Misc/OutputDevice.h"

void FForceDirectedGraphExample::RunExample()
{
    // Create sample nodes and edges
    TArray<FForceDirectedGraph::FNode> Nodes;
    TArray<FForceDirectedGraph::FEdge> Edges;
    CreateSampleGraph(Nodes, Edges);

    // Create force-directed graph instance
    FForceDirectedGraph Graph;
    Graph.Initialize(Nodes, Edges);

    // Set canvas size
    Graph.SetCanvasSize(800.0f, 600.0f);

    // Run the force-directed layout for several iterations
    const int NumIterations = 1000;
    const float DeltaTime = 0.1f;

    UE_LOG(LogTemp, Warning, TEXT("Running force-directed graph layout for %d iterations..."), NumIterations);

    for (int i = 0; i < NumIterations; i++)
    {
        Graph.Update(DeltaTime);
    }

    // Print the final node positions
    PrintNodePositions(Graph.GetNodes());

    UE_LOG(LogTemp, Warning, TEXT("Force-directed graph layout completed!"));
}

void FForceDirectedGraphExample::CreateSampleGraph(TArray<FForceDirectedGraph::FNode>& OutNodes, TArray<FForceDirectedGraph::FEdge>& OutEdges)
{
    // Create 10 nodes with random initial positions
    for (int i = 0; i < 10; i++)
    {
        FForceDirectedGraph::FNode Node;
        Node.Id = i;
        Node.Position = FVector2D(FMath::FRandRange(100.0f, 700.0f), FMath::FRandRange(100.0f, 500.0f));
        Node.Weight = 1.0f + FMath::FRand(); // Random weight between 1.0 and 2.0
        OutNodes.Add(Node);
    }

    // Create edges to form a simple graph
    // Connect node 0 to nodes 1-3
    OutEdges.Add({0, 1});
    OutEdges.Add({0, 2});
    OutEdges.Add({0, 3});

    // Connect node 1 to nodes 4-5
    OutEdges.Add({1, 4});
    OutEdges.Add({1, 5});

    // Connect node 2 to nodes 6-7
    OutEdges.Add({2, 6});
    OutEdges.Add({2, 7});

    // Connect node 3 to nodes 8-9
    OutEdges.Add({3, 8});
    OutEdges.Add({3, 9});

    // Add some additional edges
    OutEdges.Add({4, 6});
    OutEdges.Add({5, 7});
    OutEdges.Add({8, 9});
}

void FForceDirectedGraphExample::PrintNodePositions(const TArray<FForceDirectedGraph::FNode>& Nodes)
{
    UE_LOG(LogTemp, Warning, TEXT("Final node positions:"));
    for (const FForceDirectedGraph::FNode& Node : Nodes)
    {
        UE_LOG(LogTemp, Warning, TEXT("Node %d: (%.2f, %.2f)"), Node.Id, Node.Position.X, Node.Position.Y);
    }
}