#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"

// Forward declaration

// Node structure for Kamada-Kawai solver
struct FKKNodeData
{
    int Index;              // Node index in the graph
    FVector2D Position;
    float Weight;           // Node weight
    float Energy;           // Cached energy value for this node
};

class IKKGraph
{
protected:
    IKKGraph() = default;
    
public:
    virtual ~IKKGraph() = default;
    
    // Pure virtual methods - must be implemented by derived classes
    virtual const int GetNodeNum() const = 0;
    virtual const int GetEdgeNum() const = 0;
    
    virtual FKKNodeData* GetNodeAt(int NodeIndex) = 0;
    virtual const FKKNodeData* GetNodeAt(int NodeIndex) const = 0;
    
    // Get the number of edges connected to a specific node
    virtual const int GetEdgeNumOfNodeAt(int NodeIndex) const = 0;
    
    // Iterate over all nodes
    virtual void ForEachNode(TFunction<void(IKKGraph*, FKKNodeData*)> Func) = 0;
    virtual void ForEachNode(TFunction<void(const IKKGraph*, const FKKNodeData*)> Func) const = 0;
    
    // Iterate over all edges
    virtual void ForEachEdge(TFunction<void(IKKGraph*, FKKNodeData*, FKKNodeData*, float EdgeLengthScale)> Func) = 0;
    virtual void ForEachEdge(TFunction<void(const IKKGraph*, const FKKNodeData*, const FKKNodeData*, float EdgeLengthScale)> Func) const = 0;       
    
    // Get edge between two nodes (returns true if edge exists)
    virtual bool GetEdge(int NodeIndexA, int NodeIndexB, float& OutEdgeLengthScale) const = 0;
    
    // Get/Set cached max energy node index
    virtual int GetMaxEnergyNodeIndex() const = 0;
    virtual void SetMaxEnergyNodeIndex(int NodeIndex) = 0;
};

// Kamada-Kawai parameters struct
struct FKKParams
{
    float K;                   // Global spring constant (K)
    float IdealEdgeLength;     // Ideal edge length (L)
    float Tolerance;           // Convergence tolerance for gradient
    float EnergyTolerance;     // Convergence tolerance for total energy change
    int MaxIterations;         // Maximum number of iterations per node
    int MaxGlobalIterations;   // Maximum global iterations

    FKKParams()
        : K(1000.0f)
        , IdealEdgeLength(100.0f)
        , Tolerance(0.001f)
        , EnergyTolerance(0.01f)
        , MaxIterations(10)
        , MaxGlobalIterations(1000)
    {}
};

class FKKLayoutSolver
{
public:
    FKKLayoutSolver();
    
    // Set parameters
    void SetParams(const FKKParams& NewParams);
    const FKKParams& GetParams() const { return Params; }
    
    // Initialize the solver - computes all-pairs shortest paths
    void Initialize(IKKGraph* Graph);
    
    // Perform one iteration of the Kamada-Kawai algorithm
    bool Iterate(IKKGraph* Graph);
    
    // Run the algorithm until convergence or max iterations
    void Solve(IKKGraph* Graph);
    
    // Reset the solver
    void Reset();

private:
    // Compute all-pairs shortest paths using Floyd-Warshall algorithm
    void ComputeAllPairsShortestPaths(IKKGraph* Graph);
    
    // Calculate the gradient of energy function at a node
    void CalculateGradient(IKKGraph* Graph, int NodeIndex, float& OutGradientX, float& OutGradientY);
    
    // Calculate the Hessian matrix at a node
    void CalculateHessian(IKKGraph* Graph, int NodeIndex, float& Hxx, float& Hxy, float& Hyy);
    
    // Calculate the total energy of the system
    float CalculateTotalEnergy(IKKGraph* Graph);
    
private:
    FKKParams Params;
    
    // Distance matrix storing shortest paths between all pairs of nodes (d_ij)
    TArray<TArray<float>> DistanceMatrix;
    
    // Spring constants matrix (k_ij = K / d_ij^2)
    TArray<TArray<float>> SpringConstants;
    
    // Whether the solver has been initialized
    bool bInitialized;
    
    // Helper methods
    float GetDistance(FKKNodeData* NodeA, FKKNodeData* NodeB) const;
    float GetIdealLength(int i, int j) const { return Params.IdealEdgeLength * DistanceMatrix[i][j]; }
    float GetSpringConstant(int i, int j) const { return SpringConstants[i][j]; }
};