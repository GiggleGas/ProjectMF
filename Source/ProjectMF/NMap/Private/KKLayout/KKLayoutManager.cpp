#include "KKLayout/KKLayoutManager.h"

FKKLayoutManager::FKKLayoutManager()
    : CanvasWidth(1000.0f)
    , CanvasHeight(1000.0f)
    , bInitialized(false)
{
    // 使用默认参数
    Solver.SetParams(SolverParams);
}

FKKLayoutManager::~FKKLayoutManager()
{
    Clear();
}

int FKKLayoutManager::AddNode(const FString& NodeId,const FLinearColor &Color, const FVector2D& Position, float Weight)
{
    if (Graph.GetNodeIndexById(NodeId) >= 0)
    {
        // 节点已存在，返回现有索引
        return Graph.GetNodeIndexById(NodeId);
    }
    
    FKKNode Node;
    Node.Id = NodeId;
    Node.Color = Color;
    Node.Position = Position;
    Node.Weight = Weight;
    Node.Energy = 0.0f;
    
    return Graph.AddNode(Node);
}

bool FKKLayoutManager::AddEdge(const FString& Node1Id, const FString& Node2Id)
{
    return Graph.AddEdgeById(Node1Id, Node2Id);
}

bool FKKLayoutManager::AddEdgeByIndex(int Node1Index, int Node2Index)
{
    return Graph.AddEdge(Node1Index, Node2Index);
}

bool FKKLayoutManager::RemoveEdge(const FString& Node1Id, const FString& Node2Id)
{
    return Graph.RemoveEdgeById(Node1Id, Node2Id);
}

bool FKKLayoutManager::RemoveEdgeByIndex(int Node1Index, int Node2Index)
{
    return Graph.RemoveEdge(Node1Index, Node2Index);
}

int FKKLayoutManager::ClearNodeEdges(const FString& NodeId)
{
    return Graph.RemoveAllEdgesOfNodeById(NodeId);
}

int FKKLayoutManager::ClearNodeEdgesByIndex(int NodeIndex)
{
    return Graph.RemoveAllEdgesOfNode(NodeIndex);
}

void FKKLayoutManager::GenerateRandomNodePositions(float MinX, float MaxX, float MinY, float MaxY)
{
    const int NodeNum = Graph.GetNodeNum();
    for (int i = 0; i < NodeNum; ++i)
    {
        FKKNodeData* Node = Graph.GetNodeAt(i);
        if (Node)
        {
            Node->Position.X = FMath::FRandRange(MinX, MaxX);
            Node->Position.Y = FMath::FRandRange(MinY, MaxY);
        }
    }
}

void FKKLayoutManager::InitializeSolver()
{
    Solver.SetParams(SolverParams);
    Solver.Initialize(&Graph);
    bInitialized = true;
}

bool FKKLayoutManager::UpdateLayout(float DeltaTime)
{
    if (!bInitialized)
    {
        InitializeSolver();
    }
    
    // 使用 DeltaTime 调整收敛速度（通过调整容差）
    FKKParams CurrentParams = Solver.GetParams();
    float OriginalTolerance = CurrentParams.Tolerance;
    
    // 根据时间增量调整容差，使更新更平滑
    CurrentParams.Tolerance = OriginalTolerance * (1.0f + (DeltaTime - 1.0f) * 0.1f);
    CurrentParams.Tolerance = FMath::Clamp(CurrentParams.Tolerance, 0.0001f, 0.1f);
    
    Solver.SetParams(CurrentParams);
    bool Converged = Solver.Iterate(&Graph);
    
    // 恢复原始容差
    CurrentParams.Tolerance = OriginalTolerance;
    Solver.SetParams(CurrentParams);
    
    return Converged;
}

void FKKLayoutManager::SolveLayout()
{
    if (!bInitialized)
    {
        InitializeSolver();
    }
    
    Solver.Solve(&Graph);
}

void FKKLayoutManager::SetSolverParams(const FKKParams& Params)
{
    SolverParams = Params;
    Solver.SetParams(Params);
}

void FKKLayoutManager::SetCanvasSize(float Width, float Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;
}

void FKKLayoutManager::Clear()
{
    Graph.Clear();
    Solver.Reset();
    bInitialized = false;
}

bool FKKLayoutManager::HasNode(const FString& NodeId) const
{
    return Graph.GetNodeIndexById(NodeId) >= 0;
}

FVector2D FKKLayoutManager::GetNodePosition(const FString& NodeId) const
{
    const FKKNode* Node = Graph.GetNodeById(NodeId);
    return Node ? Node->Position : FVector2D(0, 0);
}

bool FKKLayoutManager::SetNodePosition(const FString& NodeId, const FVector2D& Position)
{
    FKKNode* Node = Graph.GetNodeById(NodeId);
    if (Node)
    {
        Node->Position = Position;
        return true;
    }
    return false;
}