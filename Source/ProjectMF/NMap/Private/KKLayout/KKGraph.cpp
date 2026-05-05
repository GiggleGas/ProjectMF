#include "KKLayout/KKGraph.h"

int FKKGraph::AddNode(const FKKNode& Node)
{
    FKKNode NewNode = Node;
    NewNode.Index = Nodes.Num();
    Nodes.Add(NewNode);
    
    // Update the id-to-index map
    if (!Node.Id.IsEmpty())
    {
        NodeIdToIndexMap.Add(Node.Id, NewNode.Index);
    }
    
    return NewNode.Index;
}

int FKKGraph::AddNode(const FString& NodeId, const FString& GroupId, FLinearColor Color, int Type)
{
    FKKNode NewNode; 
    NewNode.Id = NodeId;
    NewNode.GroupId = GroupId;
    NewNode.Color = Color;
    NewNode.Type = Type;
    return AddNode(NewNode);
}

bool FKKGraph::AddEdge(int SourceIndex, int TargetIndex)
{
    if (SourceIndex < 0 || SourceIndex >= Nodes.Num() ||
        TargetIndex < 0 || TargetIndex >= Nodes.Num())
    {
        return false;
    }
    
    // Avoid self-loops
    if (SourceIndex == TargetIndex)
    {
        return false;
    }
    
    // Check if edge already exists
    if (HasEdge(SourceIndex, TargetIndex))
    {
        return false;
    }
    
    // Ensure SmallIndex < LargeIndex
    int SmallIndex = FMath::Min(SourceIndex, TargetIndex);
    int LargeIndex = FMath::Max(SourceIndex, TargetIndex);
    
    // Add to both maps
    EdgesSmallToLarge.FindOrAdd(SmallIndex).Add(LargeIndex);
    EdgesLargeToSmall.FindOrAdd(LargeIndex).Add(SmallIndex);
    
    EdgeCount++;
    return true;
}

bool FKKGraph::RemoveEdge(int SourceIndex, int TargetIndex)
{
    if (SourceIndex < 0 || SourceIndex >= Nodes.Num() ||
        TargetIndex < 0 || TargetIndex >= Nodes.Num())
    {
        return false;
    }
    
    if (SourceIndex == TargetIndex)
    {
        return false;
    }
    
    // Ensure SmallIndex < LargeIndex
    int SmallIndex = FMath::Min(SourceIndex, TargetIndex);
    int LargeIndex = FMath::Max(SourceIndex, TargetIndex);
    
    // Remove from both maps
    TArray<int>* SmallList = EdgesSmallToLarge.Find(SmallIndex);
    TArray<int>* LargeList = EdgesLargeToSmall.Find(LargeIndex);
    
    if (SmallList && LargeList)
    {
        int SmallIdx = SmallList->Find(LargeIndex);
        int LargeIdx = LargeList->Find(SmallIndex);
        
        if (SmallIdx != INDEX_NONE && LargeIdx != INDEX_NONE)
        {
            SmallList->RemoveAt(SmallIdx);
            LargeList->RemoveAt(LargeIdx);
            
            // Clean up empty arrays
            if (SmallList->Num() == 0)
            {
                EdgesSmallToLarge.Remove(SmallIndex);
            }
            if (LargeList->Num() == 0)
            {
                EdgesLargeToSmall.Remove(LargeIndex);
            }
            
            EdgeCount--;
            return true;
        }
    }
    
    return false;
}

bool FKKGraph::HasEdge(int SourceIndex, int TargetIndex) const
{
    if (SourceIndex < 0 || SourceIndex >= Nodes.Num() ||
        TargetIndex < 0 || TargetIndex >= Nodes.Num())
    {
        return false;
    }
    
    if (SourceIndex == TargetIndex)
    {
        return false;
    }
    
    int SmallIndex = FMath::Min(SourceIndex, TargetIndex);
    int LargeIndex = FMath::Max(SourceIndex, TargetIndex);
    
    const TArray<int>* SmallList = EdgesSmallToLarge.Find(SmallIndex);
    return SmallList && SmallList->Contains(LargeIndex);
}

bool FKKGraph::AddEdgeById(const FString& SourceId, const FString& TargetId)
{
    int SourceIndex = GetNodeIndexById(SourceId);
    int TargetIndex = GetNodeIndexById(TargetId);
    return AddEdge(SourceIndex, TargetIndex);
}

bool FKKGraph::RemoveEdgeById(const FString& SourceId, const FString& TargetId)
{
    int SourceIndex = GetNodeIndexById(SourceId);
    int TargetIndex = GetNodeIndexById(TargetId);
    return RemoveEdge(SourceIndex, TargetIndex);
}

int FKKGraph::RemoveAllEdgesOfNode(int NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return 0;
    }
    
    int RemovedCount = 0;
    
    {
        // Remove edges where this node is the smaller index
        TArray<int>* SmallList = EdgesSmallToLarge.Find(NodeIndex);
        if (SmallList)
        {
            // Collect all large indices to remove from EdgesLargeToSmall
            TArray<int> LargeIndices = *SmallList;
            for (int LargeIndex : LargeIndices)
            {
                TArray<int>* LargeList = EdgesLargeToSmall.Find(LargeIndex);
                if (LargeList)
                {
                    int Idx = LargeList->Find(NodeIndex);
                    if (Idx != INDEX_NONE)
                    {
                        LargeList->RemoveAt(Idx);
                        if (LargeList->Num() == 0)
                        {
                            EdgesLargeToSmall.Remove(LargeIndex);
                        }
                    }
                }
                RemovedCount++;
            }
            EdgesSmallToLarge.Remove(NodeIndex);
        }
    }
    {
        // Remove edges where this node is the larger index
        TArray<int>* LargeList = EdgesLargeToSmall.Find(NodeIndex);
        if (LargeList)
        {
            // Collect all small indices to remove from EdgesSmallToLarge
            TArray<int> SmallIndices = *LargeList;
            for (int SmallIndex : SmallIndices)
            {
                TArray<int>* SmallList = EdgesSmallToLarge.Find(SmallIndex);
                if (SmallList)
                {
                    int Idx = SmallList->Find(NodeIndex);
                    if (Idx != INDEX_NONE)
                    {
                        SmallList->RemoveAt(Idx);
                        if (SmallList->Num() == 0)
                        {
                            EdgesSmallToLarge.Remove(SmallIndex);
                        }
                    }
                }
                RemovedCount++;
            }
            EdgesLargeToSmall.Remove(NodeIndex);
        }
    }
    EdgeCount -= RemovedCount;
    return RemovedCount;
}

int FKKGraph::RemoveAllEdgesOfNodeById(const FString& NodeId)
{
    int NodeIndex = GetNodeIndexById(NodeId);
    return RemoveAllEdgesOfNode(NodeIndex);
}

bool FKKGraph::HasEdgeById(const FString& SourceId, const FString& TargetId) const
{
    int SourceIndex = GetNodeIndexById(SourceId);
    int TargetIndex = GetNodeIndexById(TargetId);
    return HasEdge(SourceIndex, TargetIndex);
}

int FKKGraph::GetNodeIndexById(const FString& Id) const
{
    const int* IndexPtr = NodeIdToIndexMap.Find(Id);
    return IndexPtr ? *IndexPtr : -1;
}

FKKNode* FKKGraph::GetNodeById(const FString& Id)
{
    int Index = GetNodeIndexById(Id);
    return Index >= 0 ? &Nodes[Index] : nullptr;
}

const FKKNode* FKKGraph::GetNodeById(const FString& Id) const
{
    int Index = GetNodeIndexById(Id);
    return Index >= 0 ? &Nodes[Index] : nullptr;
}

void FKKGraph::Clear()
{
    Nodes.Empty();
    NodeIdToIndexMap.Empty();
    EdgesSmallToLarge.Empty();
    EdgesLargeToSmall.Empty();
    EdgeCount = 0;
    MaxEnergyNodeIndex = -1;
}

void FKKGraph::ForEachKKNode(TFunction<void(FKKGraph*, FKKNode&)> Func)
{
    for (int i = 0; i < Nodes.Num(); ++i)
    {
        Func(this, Nodes[i]);
    }
}

void FKKGraph::ForEachKKNode(TFunction<void(const FKKGraph*, const FKKNode&)> Func) const
{
    for (int i = 0; i < Nodes.Num(); ++i)
    {
        Func(this, Nodes[i]);
    }
}

void FKKGraph::ForEachKKEdge(TFunction<void(FKKGraph*, FKKNode&, FKKNode&, float EdgeLengthScale)> Func)
{
    // Iterate through all edges stored in EdgesSmallToLarge
    for (const auto& Pair : EdgesSmallToLarge)
    {
        int SmallIndex = Pair.Key;
        for (int LargeIndex : Pair.Value)
        {
            FKKNode& SmallNode = Nodes[SmallIndex];
            FKKNode& LargeNode = Nodes[LargeIndex];
            Func(this, SmallNode, LargeNode, 1.0f);
        }
    }
}

void FKKGraph::ForEachKKEdge(TFunction<void(const FKKGraph*, const FKKNode&, const FKKNode&, float EdgeLengthScale)> Func) const
{
    // Iterate through all edges stored in EdgesSmallToLarge
    for (const auto& Pair : EdgesSmallToLarge)
    {
        int SmallIndex = Pair.Key;
        for (int LargeIndex : Pair.Value)
        {
            const FKKNode& SmallNode = Nodes[SmallIndex];
            const FKKNode& LargeNode = Nodes[LargeIndex];
            Func(this, SmallNode, LargeNode, 1.0f);
        }
    }
}
// IKKGraph interface implementation
FKKNodeData* FKKGraph::GetNodeAt(int NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return nullptr;
    }
    return &Nodes[NodeIndex];
}

const FKKNodeData* FKKGraph::GetNodeAt(int NodeIndex) const
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return nullptr;
    }
    return &Nodes[NodeIndex];
}

const int FKKGraph::GetEdgeNumOfNodeAt(int NodeIndex) const
{
    int Count = 0;
    
    // Check edges where NodeIndex is the smaller index
    const TArray<int>* SmallList = EdgesSmallToLarge.Find(NodeIndex);
    if (SmallList)
    {
        Count += SmallList->Num();
    }
    
    // Check edges where NodeIndex is the larger index
    const TArray<int>* LargeList = EdgesLargeToSmall.Find(NodeIndex);
    if (LargeList)
    {
        Count += LargeList->Num();
    }
    
    return Count;
}

int FKKGraph::GetEdgeNumOfNodeById(const FString& NodeId) const
{
    // Get the node index from the ID
    const int* NodeIndexPtr = NodeIdToIndexMap.Find(NodeId);
    if (!NodeIndexPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetEdgeNumOfNodeById: Node with ID '%s' not found"), *NodeId);
        return 0;
    }
    
    // Use the existing GetEdgeNumOfNodeAt implementation
    return GetEdgeNumOfNodeAt(*NodeIndexPtr);
}

void FKKGraph::ForEachNode(TFunction<void(IKKGraph*, FKKNodeData*)> Func)
{
    for (int i = 0; i < Nodes.Num(); ++i)
    {
        Func(this, &Nodes[i]);
    }
}

void FKKGraph::ForEachNode(TFunction<void(const IKKGraph*, const FKKNodeData*)> Func) const
{
    for (int i = 0; i < Nodes.Num(); ++i)
    {
        Func(this, &Nodes[i]);
    }
}

void FKKGraph::ForEachEdge(TFunction<void(IKKGraph*, FKKNodeData*, FKKNodeData*, float EdgeLengthScale)> Func)
{
    // Iterate through all edges stored in EdgesSmallToLarge
    for (const auto& Pair : EdgesSmallToLarge)
    {
        int SmallIndex = Pair.Key;
        for (int LargeIndex : Pair.Value)
        {
            FKKNodeData* SmallNode = &Nodes[SmallIndex];
            FKKNodeData* LargeNode = &Nodes[LargeIndex];
            Func(this, SmallNode, LargeNode, 1.0f);
        }
    }
}

void FKKGraph::ForEachEdge(TFunction<void(const IKKGraph*, const FKKNodeData*, const FKKNodeData*, float EdgeLengthScale)> Func) const
{
    // Iterate through all edges stored in EdgesSmallToLarge
    for (const auto& Pair : EdgesSmallToLarge)
    {
        int SmallIndex = Pair.Key;
        for (int LargeIndex : Pair.Value)
        {
            const FKKNodeData* SmallNode = &Nodes[SmallIndex];
            const FKKNodeData* LargeNode = &Nodes[LargeIndex];
            Func(this, SmallNode, LargeNode, 1.0f);
        }
    }
}

bool FKKGraph::GetEdge(int NodeIndexA, int NodeIndexB, float& OutEdgeLengthScale) const
{
    if (HasEdge(NodeIndexA, NodeIndexB))
    {
        OutEdgeLengthScale = 1.0f;
        return true;
    }
    return false;
}