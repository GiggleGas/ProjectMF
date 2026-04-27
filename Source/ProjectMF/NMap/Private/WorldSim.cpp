#include "WorldSim.h"
#include "Engine/Engine.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "ForceDirectedGraph/ForceDirectedGraph.h"
#include "ForceDirectedGraph/ForceDirectedParams.h"

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
    ForceDirectedNodes.Empty();
    ForceDirectedNodes.SetNumZeroed(Nodes.Num());

    for(int i = 0; i < Nodes.Num(); i++)
    {
        ForceDirectedNodes[i].Id = i;
        // 生成随机初始位置
        ForceDirectedNodes[i].Position = FVector2D(FMath::FRandRange(50.0f,CanvasWidth-50.f), FMath::FRandRange(50.0f,CanvasHeight-50.0f));
        ForceDirectedNodes[i].Velocity = FVector2D(0, 0);
        ForceDirectedNodes[i].Weight = 1.0f;
    }
    
    /*  
    int CurrentIndex = 0;
    for (auto& NodePair : Nodes)
    {
        const FWorldSimNode& Node = NodePair.Value;
        // Node.Id = CurrentIndex;
        递增索引
        CurrentIndex++;
    }
    */
}

void FWorldSim::GenerateForceDirectedGraphs()
{
    ForceDirectedGraphs.Empty();
    ForceDirectedGraphs.Reserve(Graphs.Num());
    
    // 遍历所有图，生成 ForceDirectedGraph
    for (auto& GraphPair : Graphs)
    {
        const FWorldSimGraph& Graph = GraphPair.Value;

        
        // 准备节点 ID 数组
        TArray<int> NodeIds;
        for (const FString& NodeId : Graph.NodeIds)
        {
            NodeIds.Add(Nodes[NodeId].Index);
        }
        
        // 准备边数组
        TArray<FForceDirectedEdge> Edges;
        
        // 添加内部边
        for (auto& EdgePair : Graph.InternalEdges)
        {
            const FWorldSimEdge& Edge = EdgePair.Value;
            FForceDirectedEdge ForceDirectedEdge;
            
            // 查找源节点和目标节点的索引
            
            ForceDirectedEdge.SourceId = Nodes[Edge.Node1Id].Index;
            ForceDirectedEdge.TargetId = Nodes[Edge.Node2Id].Index;

            Edges.Add(ForceDirectedEdge);
        }
        
        // 添加外部边
        for (auto& EdgePair : Graph.ExternalEdges)
        {
            const FWorldSimEdge& Edge = EdgePair.Value;
            FForceDirectedEdge ForceDirectedEdge;
            
            // 查找源节点和目标节点的索引
            ForceDirectedEdge.SourceId = Nodes[Edge.Node1Id].Index;
            ForceDirectedEdge.TargetId = Nodes[Edge.Node2Id].Index;

            Edges.Add(ForceDirectedEdge);
        }
        

        // 初始化 ForceDirectedGraph
        ForceDirectedGraphs.Emplace();
        FForceDirectedGraph &ForceDirectedGraph=ForceDirectedGraphs.Last();
        ForceDirectedGraph.Initialize(NodeIds, Edges);
    }
}

void FWorldSim::GenerateForceDirectedData()
{
    // 生成 ForceDirectedNode 数组
    GenerateForceDirectedNodes();
    
    // 生成 ForceDirectedGraph 数组
    GenerateForceDirectedGraphs();
    
    // 初始化 GraphIds 数组，长度等于 graph 数，数据是从 0 开始的 int
    GraphIds.Empty();
    for (int i = 0; i < Graphs.Num(); i++)
    {
        GraphIds.Add(i);
    }
    
    // 初始化模拟graph数据的node数组
    InitializeGraphNodes();
}

void FWorldSim::AddChild(const FString& ParentId, const FString& ChildId, const FString& Value,
                        float R, float G, float B, float A, const FString& Type, const FString& InternalType)
{
    // Ensure parent graph exists
    FWorldSimGraph* ParentGraph = EnsureGraphExists(ParentId);
    
    // Check if node already exists
    if (Nodes.Contains(ChildId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Node %s already exists, cannot add as child"), *ChildId);
        return;
    }
    
    // Create new node
    FWorldSimNode NewNode;
    NewNode.Id = ChildId;
    NewNode.Value = Value;
    NewNode.Color = FColor(R * 255, G * 255, B * 255, A * 255);
    NewNode.Type = Type;
    NewNode.InternalType = InternalType;
    NewNode.GraphId = ParentId;
    NewNode.Index = Nodes.Num(); // 设置 index 为当前节点数量
    
    // Add node to global nodes map
    Nodes.Add(ChildId, NewNode);
    ParentGraph->NodeIds.Add(ChildId);
}

void FWorldSim::InitializeGraphNodes()
{
    // 清空并初始化模拟graph数据的node数组
    GraphNodes.Empty();
    GraphNodes.Reserve(Graphs.Num());
    
    // 为每个graph创建一个模拟node
    int GraphNodeId = 0;
    for (const auto& GraphPair : Graphs)
    {
        const FWorldSimGraph& Graph = GraphPair.Value;
        
        FForceDirectedNode GraphNode;
        GraphNode.Id = GraphNodeId++;
        GraphNode.Position = FVector2D(0, 0);
        GraphNode.Velocity = FVector2D(0, 0);
        GraphNode.Weight = 0.0f;
        
        // 计算初始位置和重量
        float TotalWeight = 0.0f;
        FVector2D TotalPosition = FVector2D(0, 0);
        
        for (const FString& NodeId : Graph.NodeIds)
        {
            if (Nodes.Contains(NodeId))
            {
                int NodeIndex = Nodes[NodeId].Index;
                if (NodeIndex >= 0 && NodeIndex < ForceDirectedNodes.Num())
                {
                    TotalPosition += ForceDirectedNodes[NodeIndex].Position;
                    TotalWeight += ForceDirectedNodes[NodeIndex].Weight;
                }
            }
        }
        
        // 计算平均位置
        if (Graph.NodeIds.Num() > 0)
        {
            GraphNode.Position = TotalPosition / Graph.NodeIds.Num();
        }
        
        // 设置重量为所有node的和
        GraphNode.Weight = TotalWeight;
        
        GraphNodes.Add(GraphNode);
    }
}

void FWorldSim::UpdateGraphNodes()
{
    // 更新模拟graph数据的node数组
    int GraphIndex = 0;
    for (const auto& GraphPair : Graphs)
    {
        const FWorldSimGraph& Graph = GraphPair.Value;
        
        if (GraphIndex >= GraphNodes.Num())
        {
            break;
        }
        
        FForceDirectedNode& GraphNode = GraphNodes[GraphIndex];
        
        // 计算平均位置和总重量
        // float TotalWeight = 0.0f;
        FVector2D TotalPosition = FVector2D(0, 0);
        
        for (const FString& NodeId : Graph.NodeIds)
        {
            if (Nodes.Contains(NodeId))
            {
                int NodeIndex = Nodes[NodeId].Index;
                if (NodeIndex >= 0 && NodeIndex < ForceDirectedNodes.Num())
                {
                    TotalPosition += ForceDirectedNodes[NodeIndex].Position;
                    // TotalWeight += ForceDirectedNodes[NodeIndex].Weight;
                }
            }
        }
        
        // 更新平均位置
        if (Graph.NodeIds.Num() > 0)
        {
            GraphNode.Position = TotalPosition / Graph.NodeIds.Num();
        }
        
        // 更新重量为所有node的和（只算一次）
        // GraphNode.Weight = TotalWeight;
        
        GraphIndex++;
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
    Solver.ResetNodeVelocities(GraphNodes);
    Solver.CalculateForces(GraphIds, GraphNodes, {}, GraphPosition);
    Solver.UpdatePositions(GraphNodes, DeltaTime);

    // 重置节点速度
    Solver.ResetNodeVelocities(ForceDirectedNodes);
    
    // 遍历所有力导向图，计算力
    for (int i = 0; i < ForceDirectedGraphs.Num(); i++)
    {
        FForceDirectedGraph& Graph = ForceDirectedGraphs[i];
        // 计算力
        Solver.CalculateForces(Graph.GetNodeIds(), ForceDirectedNodes, Graph.GetEdges(), GraphNodes[i].Position);
        // 计算 repulsion force ,from other graph nodes
        Solver.CalculateRepulsionForces(Graph.GetNodeIds(), ForceDirectedNodes, {i}, GraphNodes);
    }
    
    // 更新位置
    Solver.UpdatePositions(ForceDirectedNodes, DeltaTime);
    
    // 更新graph nodes的位置
    UpdateGraphNodes();
    
}

bool FWorldSim::AddLink(const FString& Node1Id, const FString& Node2Id)
{
    // Check if both nodes exist
    if (!Nodes.Contains(Node1Id) || !Nodes.Contains(Node2Id))
    {
        UE_LOG(LogTemp, Warning, TEXT("One or both nodes do not exist"));
        return false;
    }
    
    FWorldSimNode* Node1 = &Nodes[Node1Id];
    FWorldSimNode* Node2 = &Nodes[Node2Id];
    
    // Check if both nodes belong to the same graph
    if (Node1->GraphId != Node2->GraphId)
    {
        UE_LOG(LogTemp, Warning, TEXT("Nodes do not belong to the same graph"));
        return false;
    }
    
    // Get the graph
    FWorldSimGraph* Graph = &Graphs[Node1->GraphId];
    
    // Create internal edge
    FString EdgeId = FString::Printf(TEXT("edge_%s_%s"), *Node1Id, *Node2Id);
    FWorldSimEdge Edge;
    Edge.Id = EdgeId;
    Edge.Node1Id = Node1Id;
    Edge.Node2Id = Node2Id;
    Graph->InternalEdges.Add(EdgeId, Edge);
    
    return true;
}

bool FWorldSim::AddExternalLink(const FString& Node1Id, const FString& Node2Id)
{
    // Check if both nodes exist
    if (!Nodes.Contains(Node1Id) || !Nodes.Contains(Node2Id))
    {
        UE_LOG(LogTemp, Warning, TEXT("One or both nodes do not exist"));
        return false;
    }
    
    FWorldSimNode* Node1 = &Nodes[Node1Id];
    FWorldSimNode* Node2 = &Nodes[Node2Id];
    
    // Check if both nodes belong to different graphs
    if (Node1->GraphId == Node2->GraphId)
    {
        UE_LOG(LogTemp, Warning, TEXT("Nodes belong to the same graph, use AddLink instead"));
        return false;
    }
    
    // Get both graphs
    FWorldSimGraph* Graph1 = &Graphs[Node1->GraphId];
    FWorldSimGraph* Graph2 = &Graphs[Node2->GraphId];
    
    // Create external edge in both graphs
    FString EdgeId = FString::Printf(TEXT("external_edge_%s_%s"), *Node1Id, *Node2Id);
    FWorldSimEdge Edge;
    Edge.Id = EdgeId;
    Edge.Node1Id = Node1Id;
    Edge.Node2Id = Node2Id;
    
    Graph1->ExternalEdges.Add(EdgeId, Edge);
    Graph2->ExternalEdges.Add(EdgeId, Edge);
    
    return true;
}

void FWorldSim::ClearNodeLinks(const FString& NodeId)
{
    // Check if node exists
    if (!Nodes.Contains(NodeId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Node %s does not exist"), *NodeId);
        return;
    }
    
    FWorldSimNode* Node = &Nodes[NodeId];
    
    // Get the node's graph
    if (!Graphs.Contains(Node->GraphId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Graph %s does not exist"), *Node->GraphId);
        return;
    }
    
    FWorldSimGraph* Graph = &Graphs[Node->GraphId];
    
    // Collect edges to remove from internal edges
    TArray<FString> InternalEdgesToRemove;
    for (const TPair<FString, FWorldSimEdge>& EdgePair : Graph->InternalEdges)
    {
        const FWorldSimEdge& Edge = EdgePair.Value;
        if (Edge.Node1Id == NodeId || Edge.Node2Id == NodeId)
        {
            InternalEdgesToRemove.Add(EdgePair.Key);
        }
    }
    
    // Remove internal edges
    for (const FString& EdgeId : InternalEdgesToRemove)
    {
        Graph->InternalEdges.Remove(EdgeId);
    }
    
    // Collect edges to remove from external edges
    TArray<FString> ExternalEdgesToRemove;
    for (const TPair<FString, FWorldSimEdge>& EdgePair : Graph->ExternalEdges)
    {
        const FWorldSimEdge& Edge = EdgePair.Value;
        if (Edge.Node1Id == NodeId || Edge.Node2Id == NodeId)
        {
            ExternalEdgesToRemove.Add(EdgePair.Key);
        }
    }
    
    // Remove external edges from this graph and the linked node's graph
    for (const FString& EdgeId : ExternalEdgesToRemove)
    {
        const FWorldSimEdge& Edge = Graph->ExternalEdges[EdgeId];
        FString OtherNodeId = (Edge.Node1Id == NodeId) ? Edge.Node2Id : Edge.Node1Id;
        
        // Remove from current graph
        Graph->ExternalEdges.Remove(EdgeId);
        
        // Remove from other node's graph
        if (Nodes.Contains(OtherNodeId))
        {
            FWorldSimNode* OtherNode = &Nodes[OtherNodeId];
            if (Graphs.Contains(OtherNode->GraphId))
            {
                FWorldSimGraph* OtherGraph = &Graphs[OtherNode->GraphId];
                OtherGraph->ExternalEdges.Remove(EdgeId);
            }
        }
    }
}

FWorldSimNode* FWorldSim::EnsureNodeExists(const FString& NodeId)
{
    if (!Nodes.Contains(NodeId))
    {
        FWorldSimNode NewNode;
        NewNode.Id = NodeId;
        Nodes.Add(NodeId, NewNode);
    }
    return &Nodes[NodeId];
}

FWorldSimGraph* FWorldSim::EnsureGraphExists(const FString& GraphId)
{
    if (!Graphs.Contains(GraphId))
    {
        FWorldSimGraph NewGraph;
        NewGraph.Id = GraphId;
        Graphs.Add(GraphId, NewGraph);
    }
    return &Graphs[GraphId];
}