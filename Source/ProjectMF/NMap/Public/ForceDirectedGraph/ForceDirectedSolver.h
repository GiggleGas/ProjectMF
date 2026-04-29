#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"

// Node structure for the solver
struct FForceDirectedNode
{
    // int Id;
    FVector2D Position;
    FVector2D Velocity;
    float Weight; // Node weight for repulsion calculation
};


// Structure for ForEachNode callback parameters
struct FForceDirectedNodeInfo
{
    int NodeIndex;
    float CenterGravityScale;
};

// Structure for ForEachEdge callback parameters
struct FForceDirectedEdgeInfo
{
    int EdgeIndex;
    float EdgeStrength;
    float EdgeLengthScale; // Scale factor for spring length calculation
};

class IForceDirectedGraph
{
protected:
    // Protected constructor to prevent direct instantiation
    IForceDirectedGraph() = default;
    
public:
    // Virtual destructor for proper cleanup
    virtual ~IForceDirectedGraph() = default;
    
    // Pure virtual methods - must be implemented by derived classes
    virtual const int GetNodeNum() const = 0;
    virtual const int GetEdgeNum() const = 0;
    
    virtual FForceDirectedNode* GetNodeAt(int NodeIndex) = 0;
    virtual const FForceDirectedNode* GetNodeAt(int NodeIndex) const = 0;
    
    // Get the number of edges connected to a specific node
    virtual const int GetEdgeNumOfNodeAt(int NodeIndex) const = 0;
    
    // Iterate over all nodes, calling the provided function for each node
    virtual void ForEachNode(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, const FForceDirectedNodeInfo&)> Func) = 0;
    
    // Const version
    virtual void ForEachNode(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNodeInfo&)> Func) const = 0;
    
    // Iterate over all edges, calling the provided function for each edge
    virtual void ForEachEdge(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) = 0;
    
    // Const version
    virtual void ForEachEdge(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) const = 0;
    
    // Iterate over all edges of a specific node, calling the provided function for each edge
    virtual void ForEachEdgeOfNode(int NodeIndex, TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) = 0;
    
    // Const version
    virtual void ForEachEdgeOfNode(int NodeIndex, TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) const = 0;
};


// Force-directed parameters struct
struct FForceDirectedParams
{
    float CenterPull;        // Strength of center gravity
    float Repulsion;          // Strength of node repulsion
    float SpringStrength;     // Strength of spring force
    float SpringLength;       // Desired edge length
    float Damping;             // Damping factor
    float EdgeRepulsion;       // Strength of edge repulsion (to spread out edges connected to the same node)

    // Constructor with default values
    FForceDirectedParams()
        : CenterPull(0.001f)
        , Repulsion(1000.0f)
        , SpringStrength(0.01f)
        , SpringLength(100.0f)
        , Damping(0.9f)
        , EdgeRepulsion(1.0f)
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
    void ResetNodeVelocities(IForceDirectedGraph* Graph);

    // Calculate forces on all nodes
    void CalculateForces(IForceDirectedGraph* Graph, const FVector2D& GraphPosition);
    
    // Calculate repulsion forces for nodes in A, ignoring nodes in IgnoreNodeIdsB
    void CalculateRepulsionForces(IForceDirectedGraph* GraphA, IForceDirectedGraph* GraphB, const TArray<int>& IgnoreNodeIdsB);

    // Update node positions based on forces
    void UpdatePositions(IForceDirectedGraph* Graph, float DeltaTime);

    // Calculate center gravity for nodes
    void CalculateCenterGravity(IForceDirectedGraph* Graph, const FVector2D& GraphPosition);

    // Apply offset to all nodes' position and velocity
    void ApplyNodeOffset(IForceDirectedGraph* Graph, const FVector2D& PositionOffset, const FVector2D& VelocityOffset);

    // Calculate edge repulsion forces to spread out edges connected to the same node
    void CalculateEdgeRepulsion(IForceDirectedGraph* Graph);
private:


    // Calculate node forces (center gravity and repulsion)
    void CalculateNodeForces(IForceDirectedGraph* Graph);
    

    // Calculate edge forces (spring forces)
    void CalculateEdgeForces(IForceDirectedGraph* Graph);
    
    // Calculate edge midpoint repulsion forces
    void CalculateEdgeMidpointRepulsion(IForceDirectedGraph* Graph);

private:
    // Force-directed parameters
    FForceDirectedParams Params;

    // Canvas size for center calculation
    float CanvasWidth;
    float CanvasHeight;
};