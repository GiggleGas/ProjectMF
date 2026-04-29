#pragma once

#include "CoreMinimal.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"


struct FWorldSimNodeManager;
class FWorldSimGraph : public IForceDirectedGraph
{
public:
    FWorldSimGraph(FWorldSimNodeManager& NodeManager) : Manager(NodeManager) {};
    ~FWorldSimGraph() {};


    FString Id;
    int Index=0;

    // graph
    int ParentIndex=-1;
    TArray<int>ChildrenIndex;

    // data
    FString Value;
    FLinearColor Color;
    FString Type;
    FString InternalType;

    //
    TArray<int> InternalLinksIndex;
    TArray<int> ExternalLinksIndex;
    int ChildrenExternalLinkCount=0;
    // Returns total edge count (internal + external)
    int GetEdgeCount() const;

    // 
    FForceDirectedNode FDNode;
    FWorldSimNodeManager& Manager;



    // ForceDirectedGraphBase interface
    const int GetNodeNum() const override;
    const int GetEdgeNum() const override;
    FForceDirectedNode* GetNodeAt(int NodeIndex) override;
    const FForceDirectedNode* GetNodeAt(int NodeIndex) const override;
    const int GetEdgeNumOfNodeAt(int NodeIndex) const override;
    void ForEachNode(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, const FForceDirectedNodeInfo&)> Func) override;
    void ForEachNode(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNodeInfo&)> Func) const override;
    void ForEachEdge(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) override;
    void ForEachEdge(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) const override;
    void ForEachEdgeOfNode(int NodeIndex, TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) override;
    void ForEachEdgeOfNode(int NodeIndex, TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNode*, const FForceDirectedEdgeInfo&)> Func) const override;
    
    // simnode

    // Iterate over all child nodes with FWorldSimGraph
    void ForEachWorldSimGraph(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, int)> Func);
    void ForEachWorldSimGraph(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const;

    // Iterate over all internal links
    void ForEachInternalLink(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, FWorldSimGraph*, int)> Func);
    void ForEachInternalLink(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const;

    // Iterate over all external links
    void ForEachExternalLink(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, FWorldSimGraph*, int)> Func);
    void ForEachExternalLink(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const;

};


struct FWorldSimNodeManager
{
    FString RootGraphId;
    TMap<FString, int> NodeIdIndexMap;
    TArray<FWorldSimGraph> Nodes;

    bool AddChild(const FString& ParentId, const FString& ChildId, const FString& Value,
        FLinearColor Color, const FString& Type, const FString& InternalType);

    
    bool AddLink(const FString& Node1Id, const FString& Node2Id);
    bool AddExternalLink(const FString& Node1Id, const FString& Node2Id);
    void ClearNodeLinks(const FString& NodeId);    

    void AddRootGraph(const FString& GraphId);

    FWorldSimGraph* GetNode(const FString& NodeId);
    FWorldSimGraph* GetNodeAt(int NodeIndex);
    int GetNodeIndex(const FString& NodeId);
};