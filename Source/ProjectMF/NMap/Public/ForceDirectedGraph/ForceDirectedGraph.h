#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"

// Force-directed parameters struct
struct FForceDirectedParams
{
    float CenterPull;    // Strength of center gravity
    float Repulsion;      // Strength of node repulsion
    float SpringStrength; // Strength of spring force
    float SpringLength;   // Desired edge length
    float Damping;         // Damping factor

    // Constructor with default values
    FForceDirectedParams()
        : CenterPull(0.001f)
        , Repulsion(1000.0f)
        , SpringStrength(0.01f)
        , SpringLength(100.0f)
        , Damping(0.9f)
    {}
};

class FForceDirectedGraph : public TSharedFromThis<FForceDirectedGraph>
{
public:
    // Node structure
    struct FNode
    {
        int Id;
        FVector2D Position;
        FVector2D Velocity;
        float Weight; // Node weight for repulsion calculation
        FLinearColor Color; // Node color
    };

    // Edge structure
    struct FEdge
    {
        int SourceId;
        int TargetId;
    };

    // Constructor
    FForceDirectedGraph();

    // Initialize with nodes and edges
    void Initialize(const TArray<FNode>& InNodes, const TArray<FEdge>& InEdges);

    // Run one iteration of the force-directed layout
    void Update(float DeltaTime);

    // Get the updated nodes and edges
    const TArray<FNode>& GetNodes() const { return Nodes; }
    const TArray<FEdge>& GetEdges() const { return Edges; }

    // Get and set parameters
    const FForceDirectedParams& GetParams() const { return Params; }
    void SetParams(const FForceDirectedParams& NewParams) { Params = NewParams; }

    // Set individual parameters
    void SetCenterPull(float Value) { Params.CenterPull = Value; }
    void SetRepulsion(float Value) { Params.Repulsion = Value; }
    void SetSpringStrength(float Value) { Params.SpringStrength = Value; }
    void SetSpringLength(float Value) { Params.SpringLength = Value; }
    void SetDamping(float Value) { Params.Damping = Value; }

    // Set canvas size for center calculation
    void SetCanvasSize(float Width, float Height);

   
    // Get and set graph position
    FVector2D GetPosition() const { return Position; }
    void SetPosition(const FVector2D& NewPosition) { Position = NewPosition; }

    // Get total weight of the graph (sum of all nodes and child graphs)
    float GetTotalWeight() const;

private:
    // Calculate forces on all nodes
    void CalculateForces();

    // Update node positions based on forces
    void UpdatePositions(float DeltaTime);

    // Calculate forces for child graphs (treating each as a single node)
    void CalculateChildGraphForces();

    // Update child graph positions based on forces
    void UpdateChildGraphPositions(float DeltaTime);

    // Update node positions relative to the graph's position
    void UpdateNodesRelativeToGraphPosition();

    // Nodes and edges
    TArray<FNode> Nodes;
    TArray<FEdge> Edges;

    // Force-directed parameters
    FForceDirectedParams Params;

    // Graph position and velocity
    FVector2D Position;
    FVector2D Velocity;

    // Canvas size for center calculation
    float CanvasWidth;
    float CanvasHeight;

    // Hierarchical graph structure
    TWeakPtr<FForceDirectedGraph> ParentGraph;
    TArray<TSharedPtr<FForceDirectedGraph>> ChildrenGraphs;

public:
    // Hierarchy methods
    void SetParent(TSharedPtr<FForceDirectedGraph> InParent);
    void AddChild(TSharedPtr<FForceDirectedGraph> InChild);
    void RemoveChild(TSharedPtr<FForceDirectedGraph> InChild);
    TSharedPtr<FForceDirectedGraph> GetParent() const;
    const TArray<TSharedPtr<FForceDirectedGraph>>& GetChildren() const;

};