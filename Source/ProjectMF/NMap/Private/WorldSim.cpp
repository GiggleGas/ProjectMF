#include "WorldSim.h"
#include "Engine/Engine.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"

FWorldSim::FWorldSim()
    : CanvasWidth(800.0f)
    , CanvasHeight(600.0f)
{
    // 设置解算器参数
    FForceDirectedParams Params;
    Params.CenterPull = 0.01f;
    Params.Repulsion = 1000.0f;
    Params.SpringStrength = 0.01f;
    Params.SpringLength = 100.0f;
    Params.Damping = 0.9f;
    Solver.SetParams(Params);
    
    // 设置画布大小
    Solver.SetCanvasSize(CanvasWidth, CanvasHeight);
}

FWorldSim::~FWorldSim()
{
    // Destructor
}

void FWorldSim::GenerateForceDirectedNodes()
{
    if (Manager.Nodes.IsEmpty()) return;

    const float CenterX = CanvasWidth / 2.0f;
    const float CenterY = CanvasHeight / 2.0f;
    float MinCanvasSize = FMath::Min(CanvasWidth, CanvasHeight);
    const float MinDistance = MinCanvasSize * 0.3f; // 最小距离中心
    const float MaxDistance = MinCanvasSize * 0.45f; // 最大距离中心

    for(auto index:Manager.Nodes[0].ChildrenIndex)
    {
        FWorldSimGraph& Graph =Manager.Nodes[index];

        // Graph.FDNode.Position = { CenterX,CenterY };

        for(auto& NodeId : Graph.ChildrenIndex)
        {
            FWorldSimGraph& Node = Manager.Nodes[NodeId];

            FForceDirectedNode& FDNode = Node.FDNode;
            
            // 远离中心的随机位置
            
            // 随机角度和距离
            const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
            const float Distance = FMath::FRandRange(MinDistance, MaxDistance);
            
            // 转换为笛卡尔坐标
            const float X = CenterX + FMath::Cos(Angle) * Distance;
            const float Y = CenterY + FMath::Sin(Angle) * Distance;
            
            FDNode.Position = FVector2D(FMath::Clamp(X, 50.0f, CanvasWidth - 50.0f), FMath::Clamp(Y, 50.0f, CanvasHeight - 50.0f));
            FDNode.Velocity = FVector2D(0, 0);
            FDNode.Weight = 1.0f;
        }
    }
}


void FWorldSim::GenerateForceDirectedData()
{
    // 生成 ForceDirectedNode 数组
    GenerateForceDirectedNodes();
    
    // 初始化模拟graph数据的node数组
    InitializeGraphNodes();
}

void FWorldSim::AddChild(const FString& ParentId, const FString& ChildId, const FString& Value, FLinearColor Color, const FString& Type, const FString& InternalType)
{
    Manager.AddChild(ParentId,ChildId,Value,Color,Type, InternalType);
}

void FWorldSim::InitializeGraphNodes()
{
    if (Manager.Nodes.IsEmpty()) return;
    for (auto index : Manager.Nodes[0].ChildrenIndex)
    {
        FWorldSimGraph& Graph = Manager.Nodes[index];

        // 计算初始位置和重量
        float TotalWeight = 0.0f;
        FVector2D TotalPosition = FVector2D(0, 0);

        for (auto& NodeId : Graph.ChildrenIndex)
        {
            FWorldSimGraph& Node = Manager.Nodes[NodeId];
            TotalPosition += Node.FDNode.Position;
            TotalWeight += Node.FDNode.Weight;
        }

        FForceDirectedNode& FDNode = Graph.FDNode;

        // 计算平均位置
        if (Graph.ChildrenIndex.Num() > 0)
        {
            FDNode.Position = TotalPosition / Graph.ChildrenIndex.Num();
        }

        // 设置重量为所有node的和
        FDNode.Weight = TotalWeight;
    }
}

void FWorldSim::UpdateGraphNodes()
{
    if (Manager.Nodes.IsEmpty()) return;
    for (auto index : Manager.Nodes[0].ChildrenIndex)
    {
        FWorldSimGraph& Graph = Manager.Nodes[index];

        FVector2D TotalPosition = FVector2D(0, 0);
        for (auto& NodeId : Graph.ChildrenIndex)
        {
            FWorldSimGraph& Node = Manager.Nodes[NodeId];
            TotalPosition += Node.FDNode.Position;
        }

        FForceDirectedNode& FDNode = Graph.FDNode;

        // 计算平均位置
        if (Graph.ChildrenIndex.Num() > 0)
        {
            FDNode.Position = TotalPosition / Graph.ChildrenIndex.Num();
        }
    }
}

void FWorldSim::SetForceDirectedParams(const FForceDirectedParams& Params)
{
    // 设置力导向解算器参数
    Solver.SetParams(Params);
}

void FWorldSim::SetCanvasSize(float Width, float Height)
{
    // 保存画布大小
    CanvasWidth = Width;
    CanvasHeight = Height;
    
    // 设置画布大小
    Solver.SetCanvasSize(Width, Height);
}

void FWorldSim::UpdateForceDirectedGraphs(float DeltaTime, const FVector2D& GraphPosition)
{

    // update graph nodes
    //Solver.ResetNodeVelocities(GraphNodes);
    //Solver.CalculateForces(GraphIds, GraphNodes, {}, GraphPosition);
    //Solver.UpdatePositions(GraphNodes, DeltaTime);

    // 重置节点速度

    if (Manager.Nodes.IsEmpty()) return;
    for (auto index : Manager.Nodes[0].ChildrenIndex)
    {
        FWorldSimGraph& Graph = Manager.Nodes[index];
        Solver.ResetNodeVelocities(  &Graph);
    }
    
    // 遍历所有力导向图，计算力
    for (int i=0;i<Manager.Nodes[0].ChildrenIndex.Num();++i)
    {
        int index = Manager.Nodes[0].ChildrenIndex[i];
        FWorldSimGraph& Graph = Manager.Nodes[index];
        // 计算力
        Solver.CalculateForces(&Graph, Graph.FDNode.Position);
        // 计算 repulsion force ,from other graph nodes
        Solver.CalculateRepulsionForces(&Graph, &Manager.Nodes[0], { i });
    }
    
    // 更新位置
    if (Manager.Nodes.IsEmpty()) return;
    for (auto index : Manager.Nodes[0].ChildrenIndex)
    {
        FWorldSimGraph& Graph = Manager.Nodes[index];
        Solver.UpdatePositions(&Graph, DeltaTime);
    }
    
    // 更新graph nodes的位置
    UpdateGraphNodes();
    
}

bool FWorldSim::AddLink(const FString& Node1Id, const FString& Node2Id)
{
    return  Manager.AddLink(Node1Id, Node2Id);
}

bool FWorldSim::AddExternalLink(const FString& Node1Id, const FString& Node2Id)
{
    return  Manager.AddExternalLink(Node1Id, Node2Id);
    
}

void FWorldSim::ClearNodeLinks(const FString& NodeId)
{
    Manager.ClearNodeLinks(NodeId);
}
