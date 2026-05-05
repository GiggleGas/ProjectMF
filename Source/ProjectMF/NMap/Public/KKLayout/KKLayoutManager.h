#pragma once

#include "CoreMinimal.h"
#include "KKLayoutSolver.h"
#include "KKGraph.h"

// Forward declaration
class FKKGraph;
class FKKLayoutSolver;

class FKKLayoutManager
{
public:
    FKKLayoutManager();
    ~FKKLayoutManager();

    // 添加节点
    int AddNode(const FString& NodeId,const FLinearColor &Color, const FVector2D& Position = FVector2D(0, 0), float Weight = 1.0f);
    
    // 添加边（通过ID）
    bool AddEdge(const FString& Node1Id, const FString& Node2Id);
    
    // 添加边（通过索引）
    bool AddEdgeByIndex(int Node1Index, int Node2Index);
    
    // 移除边（通过ID）
    bool RemoveEdge(const FString& Node1Id, const FString& Node2Id);
    
    // 移除边（通过索引）
    bool RemoveEdgeByIndex(int Node1Index, int Node2Index);
    
    // 清除某个节点的所有边
    int ClearNodeEdges(const FString& NodeId);
    
    // 清除某个节点的所有边（通过索引）
    int ClearNodeEdgesByIndex(int NodeIndex);
    
    // 随机生成节点位置
    void GenerateRandomNodePositions(float MinX = 0, float MaxX = 1000, float MinY = 0, float MaxY = 1000);
    
    // 初始化布局求解器
    void InitializeSolver();
    
    // 执行一次布局迭代
    bool UpdateLayout(float DeltaTime = 1.0f);
    
    // 运行完整的布局求解
    void SolveLayout();
    
    // 设置求解器参数
    void SetSolverParams(const FKKParams& Params);
    
    // 获取当前求解器参数
    const FKKParams& GetSolverParams() const { return SolverParams; }
    
    // 设置画布大小
    void SetCanvasSize(float Width, float Height);
    
    // 获取画布大小
    float GetCanvasWidth() const { return CanvasWidth; }
    float GetCanvasHeight() const { return CanvasHeight; }
    
    // 获取图
    FKKGraph* GetGraph() { return &Graph; }
    const FKKGraph* GetGraph() const { return &Graph; }
    
    // 获取求解器
    FKKLayoutSolver* GetSolver() { return &Solver; }
    const FKKLayoutSolver* GetSolver() const { return &Solver; }
    
    // 清除所有数据
    void Clear();
    
    // 检查节点是否存在
    bool HasNode(const FString& NodeId) const;
    
    // 获取节点位置
    FVector2D GetNodePosition(const FString& NodeId) const;
    
    // 设置节点位置
    bool SetNodePosition(const FString& NodeId, const FVector2D& Position);

private:
    // 图数据结构
    FKKGraph Graph;
    
    // 布局求解器
    FKKLayoutSolver Solver;
    
    // 求解器参数
    FKKParams SolverParams;
    
    // 画布大小
    float CanvasWidth;
    float CanvasHeight;
    
    // 是否已初始化
    bool bInitialized;
};