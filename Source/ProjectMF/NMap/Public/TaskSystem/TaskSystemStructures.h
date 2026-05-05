// d:\Documents\Unreal Projects\ProjectMF\Source\ProjectMF\NMap\Public\TaskSystem\TaskSystemStructures.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TaskSystemStructures.generated.h"
/**
 * 锁和钥匙系统配置结构体
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FLockAndKeySystem
{
    GENERATED_BODY()

    // 所有可用的钥匙列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockAndKey")
    TArray<FString> Keys;

    // 所有锁的列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockAndKey")
    TArray<FString> Locks;

    // 锁和钥匙映射表 - 锁ID到所需钥匙列表的映射（一个锁可能需要多个钥匙）
   //  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockAndKey")
    TMap<FString, TArray<FString>> LocksKeys;

    FLockAndKeySystem() {}
    void Empty() { Keys.Empty(); Locks.Empty(); LocksKeys.Empty(); }
    // 从JSON对象加载锁和钥匙系统配置
    static bool LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FLockAndKeySystem& OutLockAndKeySystem);
};

/**
 * 任务结构体 - 复用UE5现有类型
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FTaskDefinition
{
    GENERATED_BODY()

    // 任务ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString TaskId;

    // 任务锁 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TArray<FString> Locks;

    // 任务给予的密钥 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TArray<FString> KeysGiven;

    // 区域ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString RegionId;

    // 入口房间
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString EntranceRoom;

    // 入口房间出现几率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EntranceRoomChance;

    // 房间选择 - 复用TMap<FString, float>，键为房间类型，值为出现次数或权重
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TMap<FString, float> RoomChoices;

    // 特殊房间选择 - 复用TMap<FString, float>
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TMap<FString, float> RoomChoicesSpecial;

    // 房间背景
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString RoomBg;

    // 背景房间
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString BackgroundRoom;

    // 海湾房间名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString CoveRoomName;

    // 海湾房间出现几率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CoveRoomChance;

    // 海湾房间最大边缘数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    int32 CoveRoomMaxEdges;

    // 任务颜色 - 复用FLinearColor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FLinearColor Colour;

    // 迷宫瓦片 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TArray<FString> MazeTiles;

    // 迷宫瓦片尺寸
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    int32 MazeTileSize;

    // 交叉链接因子
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    float CrosslinkFactor;

    // 是否制作循环
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    bool bMakeLoop;

    // 房间标签 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TArray<FString> RoomTags;

    // 必需预设 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TArray<FString> RequiredPrefabs;

    // 中心房间
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString HubRoom;

    // 级别设置片段阻挡器
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    bool bLevelSetPieceBlocker;

    FTaskDefinition()
        : EntranceRoomChance(1.0f)
        , CoveRoomChance(0.0f)
        , CoveRoomMaxEdges(0)
        , MazeTileSize(0)
        , CrosslinkFactor(0.0f)
        , bMakeLoop(false)
        , bLevelSetPieceBlocker(false)
        , Colour(FLinearColor::White)
    {}

    // 从JSON对象加载任务定义
    static bool LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskDefinition& OutTask);
};

/**
 * 房间结构体 - 复用UE5现有类型
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FRoomDefinition
{
    GENERATED_BODY()

    // 房间名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    FString RoomName;

    // 房间颜色 - 复用FLinearColor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    FLinearColor Colour;

    // 房间值（如地形类型）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    FString Value;

    // 房间类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    FString Type;

    // 房间标签 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    TArray<FString> Tags;

    // 静态布局 - 复用TMap<FString, float>，键为布局名，值为数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    TMap<FString, float> StaticLayouts;

    // 预制体计数 - 复用TMap<FString, float>，键为预制体名，值为数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    TMap<FString, float> CountPrefabs;

    // 分布预制体 - 复用TMap<FString, float>，键为预制体名，值为概率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    TMap<FString, float> DistributedPrefabs;

    // 分布百分比
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DistributePercent;

    FRoomDefinition()
        : DistributePercent(0.0f)
        , Colour(FLinearColor::White)
    {}

    // 从JSON对象加载房间定义
    static bool LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FRoomDefinition& OutRoom);
};

/**
 * 任务设置片段配置结构体 - 复用UE5现有类型
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FTaskSetPieceConfig
{
    GENERATED_BODY()

    // 出现次数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetPiece")
    int32 Count;

    // 关联的任务列表 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetPiece")
    TArray<FString> Tasks;

    FTaskSetPieceConfig() : Count(0) {}

    // 从JSON对象加载任务设置片段配置
    static bool LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetPieceConfig& OutConfig);
};
/**
 * 任务集结构体 - 复用UE5现有类型
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FTaskSetConfig
{
    GENERATED_BODY()

    // 任务集标识符
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    FString TaskSetId;

    // 任务集显示名称 - 复用FText
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    FText Name;

    // 位置/区域
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    FString Location;

    // 必需任务列表 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TArray<FString> Tasks;

    // 可选任务数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    int32 NumOptionalTasks;

    // 可选任务列表 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TArray<FString> OptionalTasks;

    // 有效的起始任务 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TArray<FString> ValidStartTasks;

    // 必需的预设物品 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TArray<FString> RequiredPrefabs;

    // 设置片段配置 - 复用TMap<FString, FTaskSetPieceConfig>
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TMap<FString, FTaskSetPieceConfig> SetPieces;

    // 海洋填充片段 - 复用TMap<FString, int32>
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TMap<FString, int32> OceanPrefillSetPieces;

    // 海洋人口 - 复用FString数组
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSet")
    TArray<FString> OceanPopulation;

    FTaskSetConfig() : NumOptionalTasks(0) {}

    // 从JSON对象加载任务集配置
    static bool LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetConfig& OutTaskSet);
};


