#pragma once

#include "CoreMinimal.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
#include "WorldSimGraph.h"
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
    
    // 生成 ForceDirectedNode 数组并保存在成员变量中
    void GenerateForceDirectedNodes();
    
    // 生成 ForceDirectedGraph 数组并保存在成员变量中
    void GenerateForceDirectedGraphs();
    
    // 合成函数：生成所有 ForceDirected 数据
    void GenerateForceDirectedData();
    
    // 执行力导向计算
    void UpdateForceDirectedGraphs(float DeltaTime, const FVector2D& GraphPosition);
    
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