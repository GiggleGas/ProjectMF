#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "KKLayoutSolver.h"

// Forward declaration
class IKKGraph;

// Node structure for Kamada-Kawai solver
struct FKKNode : public FKKNodeData
{
    FString Id; // Node unique identifier
    FString GroupId;
    FLinearColor Color;
    int Type;
};

// Concrete implementation of IKKGraph
class FKKGraph : public IKKGraph
{
public:
    // Constructor
    FKKGraph() : MaxEnergyNodeIndex(-1), EdgeCount(0) {}
    
    // Add a node to the graph
    int AddNode(const FKKNode& Node);
    int AddNode(const FString& NodeId, const FString& GroupId, FLinearColor Color, int Type);
    
    // Add an edge to the graph (by index)
    bool AddEdge(int SourceIndex, int TargetIndex);
    
    // Add an edge to the graph (by id)
    bool AddEdgeById(const FString& SourceId, const FString& TargetId);
    
    // Remove an edge from the graph (by index)
    bool RemoveEdge(int SourceIndex, int TargetIndex);
    
    // Remove an edge from the graph (by id)
    bool RemoveEdgeById(const FString& SourceId, const FString& TargetId);
    
    // Remove all edges connected to a specific node
    int RemoveAllEdgesOfNode(int NodeIndex);
    
    // Remove all edges connected to a specific node (by id)
    int RemoveAllEdgesOfNodeById(const FString& NodeId);
    
    // Check if an edge exists (by index)
    bool HasEdge(int SourceIndex, int TargetIndex) const;
    
    // Check if an edge exists (by id)
    bool HasEdgeById(const FString& SourceId, const FString& TargetId) const;
    
    int GetEdgeNumOfNodeById(const FString& NodeId) const;

    // Get node index by id
    int GetNodeIndexById(const FString& Id) const;
    
    // Get node by id
    FKKNode* GetNodeById(const FString& Id);
    const FKKNode* GetNodeById(const FString& Id) const;
    
    // Clear all nodes and edges
    void Clear();
    


 // New FKKNode-specific iteration functions following the same pattern as existing ForEachNode
    void ForEachKKNode(TFunction<void(FKKGraph*, FKKNode&)> Func);
    void ForEachKKNode(TFunction<void(const FKKGraph*, const FKKNode&)> Func) const;
 
    // FKKNode-specific edge iteration functions
    void ForEachKKEdge(TFunction<void(FKKGraph*, FKKNode&, FKKNode&, float EdgeLengthScale)> Func);
    void ForEachKKEdge(TFunction<void(const FKKGraph*, const FKKNode&, const FKKNode&, float EdgeLengthScale)> Func) const;
    

    // IKKGraph interface implementation
    virtual const int GetNodeNum() const override { return Nodes.Num(); }
    virtual const int GetEdgeNum() const override { return EdgeCount; }
    
    virtual FKKNodeData* GetNodeAt(int NodeIndex) override;
    virtual const FKKNodeData* GetNodeAt(int NodeIndex) const override;
    
    virtual const int GetEdgeNumOfNodeAt(int NodeIndex) const override;
    
    virtual void ForEachNode(TFunction<void(IKKGraph*, FKKNodeData*)> Func) override;
    virtual void ForEachNode(TFunction<void(const IKKGraph*, const FKKNodeData*)> Func) const override;
    
    virtual void ForEachEdge(TFunction<void(IKKGraph*, FKKNodeData*, FKKNodeData*, float EdgeLengthScale)> Func) override;
    virtual void ForEachEdge(TFunction<void(const IKKGraph*, const FKKNodeData*, const FKKNodeData*, float EdgeLengthScale)> Func) const override;
    
    virtual bool GetEdge(int NodeIndexA, int NodeIndexB, float& OutEdgeLengthScale) const override;
    
    virtual int GetMaxEnergyNodeIndex() const override { return MaxEnergyNodeIndex; }
    virtual void SetMaxEnergyNodeIndex(int NodeIndex) override { MaxEnergyNodeIndex = NodeIndex; }
    
private:
    TArray<FKKNode> Nodes;
    TMap<FString, int> NodeIdToIndexMap;
    
    // Adjacency list using two maps for efficient storage
    // For edge (a, b) where a < b:
    // - EdgesSmallToLarge[a] contains b
    // - EdgesLargeToSmall[b] contains a
    TMap<int, TArray<int>> EdgesSmallToLarge;
    TMap<int, TArray<int>> EdgesLargeToSmall;
    int EdgeCount;
    
    int MaxEnergyNodeIndex;
};