#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "ForceDirectedGraph/ForceDirectedGraph.h"

struct FWorldSimNode;
struct FWorldSimEdge;
struct FWorldSimGraph;

struct FWorldSimNode
{
    FString Id;
    FString GraphId;
    FString Value;
    FColor Color;
    FString Type;
    FString InternalType;
    int Index; // 添加 index 属性
};

struct FWorldSimEdge
{
    FString Id;
    FString Node1Id;
    FString Node2Id;
    TMap<FString, FString> LockData;
};

struct FWorldSimGraph
{
    FString Id;
    TArray<FString> NodeIds;
    TMap<FString, FWorldSimEdge> InternalEdges;
    TMap<FString, FWorldSimEdge> ExternalEdges;
};

class FWorldSim
{
public:
    FWorldSim();
    ~FWorldSim();

    void AddChild(const FString& ParentId, const FString& ChildId, const FString& Value,
                 float R, float G, float B, float A, const FString& Type, const FString& InternalType);
    
    bool AddLink(const FString& Node1Id, const FString& Node2Id);
    
    bool AddExternalLink(const FString& Node1Id, const FString& Node2Id);
    
    void ClearNodeLinks(const FString& NodeId);
    
    // Helper method to ensure node exists
    FWorldSimNode* EnsureNodeExists(const FString& NodeId);
    
    // Helper method to ensure graph exists
    FWorldSimGraph* EnsureGraphExists(const FString& GraphId);
    
    // 生成 ForceDirectedNode 数组并保存在成员变量中
    void GenerateForceDirectedNodes();
    
    // 生成 ForceDirectedGraph 数组并保存在成员变量中
    void GenerateForceDirectedGraphs();
    
    // 合成函数：生成所有 ForceDirected 数据
    void GenerateForceDirectedData();
    
    // 获取更新后的 ForceDirectedNode 数组
    // TArray<FForceDirectedNode>& GetMutableForceDirectedNodes() { return ForceDirectedNodes; }
    
    // 获取生成的 ForceDirectedGraph 数组
    const TArray<FForceDirectedGraph>& GetForceDirectedGraphs() const { return ForceDirectedGraphs; }
    
    // 执行力导向计算
    void UpdateForceDirectedGraphs(float DeltaTime, const FVector2D& GraphPosition);
    
    // 初始化并更新模拟graph数据的node数组
    void InitializeGraphNodes();
    void UpdateGraphNodes();
    
    // 获取更新后的 ForceDirectedNode 数组
    const TArray<FForceDirectedNode>& GetForceDirectedNodes() const { return ForceDirectedNodes; }
    
    // 设置力导向解算器参数
    void SetForceDirectedParams(const FForceDirectedParams& Params);
    
    // 设置画布大小
    void SetCanvasSize(float Width, float Height);
    
    // 获取画布大小
    float GetCanvasWidth() const { return CanvasWidth; }
    float GetCanvasHeight() const { return CanvasHeight; }
    
    // 获取 Graphs 和 Nodes
    const TMap<FString, FWorldSimGraph>& GetGraphs() const { return Graphs; }
    const TMap<FString, FWorldSimNode>& GetNodes() const { return Nodes; }

private:
    TMap<FString, FWorldSimGraph> Graphs;
    TMap<FString, FWorldSimNode> Nodes;
    
    // 存储生成的力导向数据
    TArray<FForceDirectedNode> ForceDirectedNodes;
    TArray<FForceDirectedGraph> ForceDirectedGraphs;
    
    // 模拟graph数据的node数组
    TArray<FForceDirectedNode> GraphNodes;
    
    // GraphIds数组，长度等于graph数，数据是从0开始的int
    TArray<int> GraphIds;
    
    // 画布大小
    float CanvasWidth;
    float CanvasHeight;
    
    // 力导向解算器
    FForceDirectedSolver Solver;
};