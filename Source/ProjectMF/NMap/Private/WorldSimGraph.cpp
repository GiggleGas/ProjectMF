#include "WorldSimGraph.h"

// FWorldSimNodeManager implementation
FWorldSimGraph* FWorldSimNodeManager::GetNode(const FString& NodeId)
{
    if (NodeIdIndexMap.Contains(NodeId))
    {
        int Index = NodeIdIndexMap[NodeId];
        if (Index >= 0 && Index < Nodes.Num())
        {
            return &Nodes[Index];
        }
    }
    return nullptr;
}

FWorldSimGraph  * FWorldSimNodeManager::GetNodeAt(int NodeIndex)
{
    if (NodeIndex >= 0 && NodeIndex < Nodes.Num())
    {
        return &Nodes[NodeIndex];
    }
    return nullptr;
}

int FWorldSimNodeManager::GetNodeIndex(const FString& NodeId)
{
    if (NodeIdIndexMap.Contains(NodeId))
    {
        return NodeIdIndexMap[NodeId];
    }
    return -1;
}

void FWorldSimNodeManager::AddRootGraph(const FString& GraphId)
{
 
}
    

bool FWorldSimNodeManager::AddChild(const FString& ParentId, const FString& ChildId, const FString& Value,
    FLinearColor Color, const FString& Type, const FString& InternalType)
{
    // 检查父节点是否存在
    if (!NodeIdIndexMap.Contains(ParentId))
    {
        if (Nodes.IsEmpty())
        {
            RootGraphId = ParentId;
            NodeIdIndexMap.Add(ParentId,0); // 0
            FWorldSimGraph ChildGraph(*this);
            ChildGraph.Id = ParentId;
            Nodes.Add(ChildGraph); // first
        }
        else
        {
            // not first and not parent
            return false;
        }
    }


    // 检查子节点是否已存在
    if (NodeIdIndexMap.Contains(ChildId))
    {
        return false;
    }

    int ParentIndex = NodeIdIndexMap[ParentId];

    // 创建新的子节点
    FWorldSimGraph ChildGraph(*this);
    ChildGraph.Id = ChildId;
    ChildGraph.Value = Value;
    ChildGraph.Color = Color;
    ChildGraph.Type = Type;
    ChildGraph.InternalType = InternalType;
    ChildGraph.ParentIndex = ParentIndex;
    ChildGraph.Index = Nodes.Num();

    // 添加到节点数组
    Nodes.Add(ChildGraph);
    NodeIdIndexMap.Add(ChildId, ChildGraph.Index);

    // 更新父节点的子节点列表
    Nodes[ParentIndex].ChildrenIndex.Add(ChildGraph.Index);

    return true;
}

bool FWorldSimNodeManager::AddLink(const FString& Node1Id, const FString& Node2Id)
{
    // 检查两个节点是否都存在
    if (!NodeIdIndexMap.Contains(Node1Id) || !NodeIdIndexMap.Contains(Node2Id))
    {
        return false;
    }

    int Node1Index = NodeIdIndexMap[Node1Id];
    int Node2Index = NodeIdIndexMap[Node2Id];

    FWorldSimGraph& Node1 = Nodes[Node1Index];
    FWorldSimGraph& Node2 = Nodes[Node2Index];
    
    // 确保两个节点属于同一图（有共同的根节点）
    // 简单实现：假设它们已经在同一图中
    if (Node1.ParentIndex != Node2.ParentIndex)
    {
        return false;
    }

    // 检查链接是否已存在
    if (Node1.InternalLinksIndex.Contains(Node2Index) ||
        Node2.InternalLinksIndex.Contains(Node1Index))
    {
        return false;
    }

    // 双向添加内部链接
    Node1.InternalLinksIndex.Add(Node2Index);
    Node2.InternalLinksIndex.Add(Node1Index);

    return true;
}

bool FWorldSimNodeManager::AddExternalLink(const FString& Node1Id, const FString& Node2Id)
{
    // 检查两个节点是否都存在
    if (!NodeIdIndexMap.Contains(Node1Id) || !NodeIdIndexMap.Contains(Node2Id))
    {
        return false;
    }

    int Node1Index = NodeIdIndexMap[Node1Id];
    int Node2Index = NodeIdIndexMap[Node2Id];
    FWorldSimGraph& Node1 = Nodes[Node1Index];
    FWorldSimGraph& Node2 = Nodes[Node2Index];
    
    if(Node1.ParentIndex != Node2.ParentIndex)
    {
        return false;
    }

    // 检查外部链接是否已存在
    if (Node1.ExternalLinksIndex.Contains(Node2Index) ||
        Node2.ExternalLinksIndex.Contains(Node1Index))
    {
        return false;
    }

    // 双向添加外部链接
    Node1.ExternalLinksIndex.Add(Node2Index);
    Node2.ExternalLinksIndex.Add(Node1Index);

    return true;
}

void FWorldSimNodeManager::ClearNodeLinks(const FString& NodeId)
{
    // 检查节点是否存在
    if (!NodeIdIndexMap.Contains(NodeId))
    {
        return;
    }

    int NodeIndex = NodeIdIndexMap[NodeId];
    FWorldSimGraph& Node = Nodes[NodeIndex];
    
    // 清除该节点的所有内部链接
    for (int LinkedIndex : Node.InternalLinksIndex)
    {
        // 从链接的节点中移除对当前节点的引用
        Nodes[LinkedIndex].InternalLinksIndex.Remove(NodeIndex);
    }
    Node.InternalLinksIndex.Empty();

    // 清除该节点的所有外部链接
    for (int LinkedIndex : Node.ExternalLinksIndex)
    {
        // 从链接的节点中移除对当前节点的引用
        Nodes[LinkedIndex].ExternalLinksIndex.Remove(NodeIndex);
    }
    Node.ExternalLinksIndex.Empty();
}

// FWorldSimGraph implementation
const int FWorldSimGraph::GetNodeNum() const
{
    return ChildrenIndex.Num();
}

int FWorldSimGraph::GetEdgeCount() const
{
    return InternalLinksIndex.Num() + ExternalLinksIndex.Num();
}

const int FWorldSimGraph::GetEdgeNum() const
{
    int AllChildrenEdgeCount = 0;
    for( auto CIndex : ChildrenIndex)
    {
        AllChildrenEdgeCount += Manager.Nodes[CIndex].GetEdgeCount();
    }
    return AllChildrenEdgeCount;
}

const int FWorldSimGraph::GetEdgeNumOfNodeAt(int NodeIndex) const
{
    if (NodeIndex < 0 || NodeIndex >= ChildrenIndex.Num())
    {
        return 0;
    }
    return Manager.Nodes[ChildrenIndex[NodeIndex]].GetEdgeCount();
}

FForceDirectedNode* FWorldSimGraph::GetNodeAt(int NodeIndex)
{
    if (NodeIndex >= 0 && NodeIndex < ChildrenIndex.Num())
    {
        int WorldSimNodeIndex = ChildrenIndex[NodeIndex];
        if (WorldSimNodeIndex >= 0 && WorldSimNodeIndex < Manager.Nodes.Num())
        {
            return &Manager.Nodes[WorldSimNodeIndex].FDNode;
        }
    }
    return nullptr;
}

const FForceDirectedNode* FWorldSimGraph::GetNodeAt(int NodeIndex) const
{
    if (NodeIndex >= 0 && NodeIndex < ChildrenIndex.Num())
    {
        int WorldSimNodeIndex = ChildrenIndex[NodeIndex];
        if (WorldSimNodeIndex >= 0 && WorldSimNodeIndex < Manager.Nodes.Num())
        {
            return &Manager.Nodes[WorldSimNodeIndex].FDNode;
        }
    }
    return nullptr;
}

// IForceDirectedGraph interface implementation
void FWorldSimGraph::ForEachNode(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, int)> Func)
{
    for (int i = 0; i < ChildrenIndex.Num(); i++)
    {
        FWorldSimGraph& Node =Manager.Nodes[ ChildrenIndex[i]];
        Func(this, &Node.FDNode, i);
    }
}

void FWorldSimGraph::ForEachNode(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, int)> Func) const
{
    for (int i = 0; i < ChildrenIndex.Num(); i++)
    {
        FWorldSimGraph& Node = Manager.Nodes[ChildrenIndex[i]];
        Func(this, &Node.FDNode, i);
    }
}

// IForceDirectedGraph interface implementation
void FWorldSimGraph::ForEachEdge(TFunction<void(IForceDirectedGraph*, FForceDirectedNode*, FForceDirectedNode*, int)> Func)
{
    int index = 0;

    for (auto i : ChildrenIndex)
    {
        FWorldSimGraph& snode = Manager.Nodes[i];
        for (auto j : snode.InternalLinksIndex)
        {
            FWorldSimGraph&tnode= Manager.Nodes[j];
            Func(this, &snode.FDNode, &tnode.FDNode, index);
            ++index;
        }
    }
}

void FWorldSimGraph::ForEachEdge(TFunction<void(const IForceDirectedGraph*, const FForceDirectedNode*, const FForceDirectedNode*, int)> Func) const
{
    int index = 0;
    for (auto i : ChildrenIndex)
    {
        FWorldSimGraph& snode = Manager.Nodes[i];
        for (auto j : snode.InternalLinksIndex)
        {
            FWorldSimGraph&tnode= Manager.Nodes[j];
            Func(this, &snode.FDNode, &tnode.FDNode, index);
            ++index;
        }
    }
}

// Iterate over all child nodes with FWorldSimGraph
void FWorldSimGraph::ForEachWorldSimGraph(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, int)> Func)
{
    for (int i = 0; i < ChildrenIndex.Num(); i++)
    {
        FWorldSimGraph* ChildNode = &Manager.Nodes[ChildrenIndex[i]];
        Func(this, ChildNode, i);
    }
}

void FWorldSimGraph::ForEachWorldSimGraph(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const
{
    for (int i = 0; i < ChildrenIndex.Num(); i++)
    {
        const FWorldSimGraph* ChildNode = &Manager.Nodes[ChildrenIndex[i]];
        Func(this, ChildNode, i);
    }
}

// Iterate over all internal links
void FWorldSimGraph::ForEachInternalLink(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, FWorldSimGraph*, int)> Func)
{
    int index = 0;
    for (int ChildIndex : ChildrenIndex)
    {
        FWorldSimGraph& SourceNode = Manager.Nodes[ChildIndex];
        for (int TargetIndex : SourceNode.InternalLinksIndex)
        {
            // 避免重复处理同一链接（只处理一次）
            if (ChildIndex < TargetIndex)
            {
                FWorldSimGraph& TargetNode = Manager.Nodes[TargetIndex];
                Func(this, &SourceNode, &TargetNode, index);
                ++index;
            }
        }
    }
}

void FWorldSimGraph::ForEachInternalLink(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const
{
    int index = 0;
    for (int ChildIndex : ChildrenIndex)
    {
        const FWorldSimGraph& SourceNode = Manager.Nodes[ChildIndex];
        for (int TargetIndex : SourceNode.InternalLinksIndex)
        {
            // 避免重复处理同一链接（只处理一次）
            if (ChildIndex < TargetIndex)
            {
                const FWorldSimGraph& TargetNode = Manager.Nodes[TargetIndex];
                Func(this, &SourceNode, &TargetNode, index);
                ++index;
            }
        }
    }
}

// Iterate over all external links
void FWorldSimGraph::ForEachExternalLink(TFunction<void(FWorldSimGraph*, FWorldSimGraph*, FWorldSimGraph*, int)> Func)
{
    int index = 0;
    for (int ChildIndex : ChildrenIndex)
    {
        FWorldSimGraph& SourceNode = Manager.Nodes[ChildIndex];
        for (int TargetIndex : SourceNode.ExternalLinksIndex)
        {
            // 避免重复处理同一链接（只处理一次）
            if (ChildIndex < TargetIndex)
            {
                FWorldSimGraph& TargetNode = Manager.Nodes[TargetIndex];
                Func(this, &SourceNode, &TargetNode, index);
                ++index;
            }
        }
    }
}

void FWorldSimGraph::ForEachExternalLink(TFunction<void(const FWorldSimGraph*, const FWorldSimGraph*, const FWorldSimGraph*, int)> Func) const
{
    int index = 0;
    for (int ChildIndex : ChildrenIndex)
    {
        const FWorldSimGraph& SourceNode = Manager.Nodes[ChildIndex];
        for (int TargetIndex : SourceNode.ExternalLinksIndex)
        {
            // 避免重复处理同一链接（只处理一次）
            if (ChildIndex < TargetIndex)
            {
                const FWorldSimGraph& TargetNode = Manager.Nodes[TargetIndex];
                Func(this, &SourceNode, &TargetNode, index);
                ++index;
            }
        }
    }
}