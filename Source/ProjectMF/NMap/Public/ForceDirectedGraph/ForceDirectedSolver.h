#pragma once

#include "CoreMinimal.h"

// Node structure for the solver
struct FForceDirectedNode
{
    int Id;
    FVector2D Position;
    FVector2D Velocity;
    float Weight; // Node weight for repulsion calculation
};

// Edge structure for the solver
struct FForceDirectedEdge
{
    int SourceId;
    int TargetId;
};

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

class FForceDirectedSolver
{
public:
    // Constructor
    FForceDirectedSolver();

    // Set parameters
    void SetParams(const FForceDirectedParams& NewParams);
    const FForceDirectedParams& GetParams() const { return Params; }

    // Set canvas size for center calculation
    void SetCanvasSize(float Width, float Height);

    // Reset node velocities with damping
    void ResetNodeVelocities(TArray<FForceDirectedNode>& Nodes);

    // Calculate forces on all nodes
    void CalculateForces(const TArray<int>& NodeIds, TArray<FForceDirectedNode>& Nodes, const TArray<FForceDirectedEdge>& Edges, const FVector2D& GraphPosition);
    
    // Calculate forces for all nodes in all graphs
    void CalculateForcesForAllNodes(TArray<FForceDirectedNode>& Nodes, const TArray<FForceDirectedEdge>& Edges, const FVector2D& GraphPosition);
    
    // Calculate forces for all nodes (for graph nodes)
    void CalculateAllNodeForces(TArray<FForceDirectedNode>& Nodes, const FVector2D& GraphPosition);
    
    // Calculate repulsion forces for nodes in A, ignoring nodes in IgnoreNodeIdsB
    void CalculateRepulsionForces(const TArray<int>& NodeIdsA, TArray<FForceDirectedNode>& NodesA, const TArray<int>& IgnoreNodeIdsB, const TArray<FForceDirectedNode>& NodesB);

    // Update node positions based on forces
    void UpdatePositions(TArray<FForceDirectedNode>& Nodes, float DeltaTime);
private:


    // Calculate node forces (center gravity and repulsion)
    void CalculateNodeForces(const TArray<int>& NodeIds, TArray<FForceDirectedNode>& Nodes, const FVector2D& GraphPosition);

    // Calculate edge forces (spring forces)
    void CalculateEdgeForces(const TArray<FForceDirectedEdge>& Edges, TArray<FForceDirectedNode>& Nodes);



private:
    // Force-directed parameters
    FForceDirectedParams Params;

    // Canvas size for center calculation
    float CanvasWidth;
    float CanvasHeight;
};