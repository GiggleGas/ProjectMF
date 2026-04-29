#pragma once

#include "CoreMinimal.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "WorldSimGraph.h"

// 更新模式枚举
UENUM(BlueprintType)
enum class EWorldSimUpdateMode : uint8
{
    FullUpdate UMETA(DisplayName = "Full Update"),           // 同时更新 graph 和 node
    GraphOnly UMETA(DisplayName = "Graph Only"),            // 只更新 graph，node 跟随 graph 移动
    NodeOnly UMETA(DisplayName = "Node Only")              // 只更新 node，不更新 graph
};

class FWorldSim
{
public:
    FWorldSim();
    ~FWorldSim();

    void AddChild(const FString& ParentId, const FString& ChildId, const FString& Value,
         FLinearColor Color, const FString& Type, const FString& InternalType);
    
    bool AddLink(const FString& Node1Id, const FString& Node2Id);
    
    bool AddExternalLink(const FString& Node1Id, const FString& Node2Id);
    
    void ClearNodeLinks(const FString& NodeId);
    
    // 随机生成 graph 的坐标
    void GenerateRandomGraphPositions();
    
    // 根据 graph 坐标生成 node 坐标
    void GenerateForceDirectedNodes();
    
    // 生成 ForceDirectedGraph 数组并保存在成员变量中
    void GenerateForceDirectedGraphs();
    
    // 合成函数：生成所有 ForceDirected 数据
    void GenerateForceDirectedData();
    
    // 执行力导向计算
    void UpdateForceDirectedGraphs(float DeltaTime, const FVector2D& GraphPosition, EWorldSimUpdateMode UpdateMode = EWorldSimUpdateMode::FullUpdate);
    
    // 初始化并更新模拟graph数据的node数组
    void InitializeGraphNodes();
    void UpdateGraphNodes();
    
    // 设置力导向解算器参数
    void SetForceDirectedParams(const FForceDirectedParams& Params);
    
    // 设置画布大小
    void SetCanvasSize(float Width, float Height);
    
    // 获取画布大小
    float GetCanvasWidth() const { return CanvasWidth; }
    float GetCanvasHeight() const { return CanvasHeight; }
    
    // 获取 Graphs 和 Nodes
    const TArray<FWorldSimGraph>& GetNodes() const { return Manager.Nodes; }

private:

    FWorldSimNodeManager Manager;

    // 画布大小
    float CanvasWidth;
    float CanvasHeight;
    
    // 力导向解算器
    FForceDirectedSolver Solver;
};