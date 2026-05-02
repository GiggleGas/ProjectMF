#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TaskSystem/TaskSystemStructures.h"

#include "TaskSystemDataAsset.generated.h"

/**
 * 任务系统数据资源 - 用于在编辑器中管理和组织任务数据
 */
UCLASS(Blueprintable, EditInlineNew)
class PROJECTMF_API UTaskSystemDataAsset : public UPrimaryDataAsset


{
    GENERATED_BODY()

public:
    // 任务定义列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tasks")
    TArray<FTaskDefinition> Tasks;

    // 房间定义列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms")
    TArray<FRoomDefinition> Rooms;

    // 任务集配置列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskSets")
    TArray<FTaskSetConfig> TaskSets;

    // 锁和钥匙系统配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockAndKey")
    FLockAndKeySystem LockAndKeySystem;

    // 获取特定任务
    // UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    FTaskDefinition* GetTaskById(const FString& TaskId);

    // 获取特定房间
    // UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    FRoomDefinition* GetRoomByName(const FString& RoomName);

    // 获取特定任务集
    // UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    FTaskSetConfig* GetTaskSetById(const FString& TaskSetId);

    // 获取锁和钥匙系统配置
    // UFUNCTION(BlueprintCallable, Category = "TaskSystem|LockAndKey")
    FLockAndKeySystem* GetLockAndKeySystem();

    // 查找任务索引
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    int32 FindTaskIndex(const FString& TaskId);

    // 查找房间索引
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    int32 FindRoomIndex(const FString& RoomName);

    // 查找任务集索引
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    int32 FindTaskSetIndex(const FString& TaskSetId);

    // 添加任务
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    void AddTask(const FTaskDefinition& Task);

    // 添加房间
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    void AddRoom(const FRoomDefinition& Room);

    // 添加任务集
    UFUNCTION(BlueprintCallable, Category = "TaskSystem")
    void AddTaskSet(const FTaskSetConfig& TaskSet);

    // 添加锁和钥匙系统配置
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|LockAndKey")
    void AddLockAndKeySystem(const FLockAndKeySystem& LockAndKeySys);

    // 从ID获取任务
    FTaskDefinition* GetTaskByIdInternal(const FString& TaskId);

    // 从名称获取房间
    FRoomDefinition* GetRoomByNameInternal(const FString& RoomName);

    // 从ID获取任务集
    FTaskSetConfig* GetTaskSetByIdInternal(const FString& TaskSetId);

    // 从JSON文件加载任务系统数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadFromJson(const FString& JsonFilePath);

    // 从JSON字符串加载任务系统数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadFromJsonString(const FString& JsonString);

    // 从JSON目录加载所有数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadFromJsonDirectory(const FString& JsonDirectoryPath);

    // 从资源包加载任务系统（支持多个JSON文件）
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadFromPackage(const FString& PackagePath);

    // 从指定路径加载任务
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadTasksFromJsonDirectory(const FString& JsonDirectoryPath);

    // 从指定路径加载房间
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadRoomsFromJsonDirectory(const FString& JsonDirectoryPath);

    // 从指定路径加载任务集
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadTaskSetsFromJsonDirectory(const FString& JsonDirectoryPath);

    // 从指定路径加载锁和钥匙系统配置
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadLockAndKeySystemFromJson(const FString& JsonFilePath);

    // 验证JSON数据格式
    UFUNCTION(BlueprintPure, Category = "TaskSystem|Validation")
    bool ValidateJson(const FString& JsonString);

    // 将任务系统数据保存为JSON
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Saving")
    bool SaveToJson(const FString& OutputPath);

    // 验证任务系统数据完整性
    UFUNCTION(BlueprintPure, Category = "TaskSystem|Validation")
    bool ValidateData();

    // 分别加载任务数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadTasksFromJson(const FString& JsonFilePath);

    // 分别加载房间数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadRoomsFromJson(const FString& JsonFilePath);

    // 分别加载任务集数据
    UFUNCTION(BlueprintCallable, Category = "TaskSystem|Loading")
    bool LoadTaskSetsFromJson(const FString& JsonFilePath);

    // 获取主要资产对象名称
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

protected:
    // 内部方法：从JSON目录加载任务
    bool LoadTasksFromJsonDirectoryInternal(const FString& JsonDirectoryPath);

    // 内部方法：从JSON目录加载房间
    bool LoadRoomsFromJsonDirectoryInternal(const FString& JsonDirectoryPath);

    // 内部方法：从JSON目录加载任务集
    bool LoadTaskSetsFromJsonDirectoryInternal(const FString& JsonDirectoryPath);

    // 内部方法：从JSON加载锁和钥匙系统配置
    bool LoadLockAndKeySystemFromJsonInternal(const FString& JsonFilePath);

    // 内部解析辅助函数
    TArray<FTaskDefinition> ParseTasksFromJson(const FString& JsonString);
    TArray<FRoomDefinition> ParseRoomsFromJson(const FString& JsonString);
    TArray<FTaskSetConfig> ParseTaskSetsFromJson(const FString& JsonString);

    // 内部解析辅助函数
    bool ParseTaskFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskDefinition& OutTask);
    bool ParseRoomFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FRoomDefinition& OutRoom);
    bool ParseTaskSetFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetConfig& OutTaskSet);
};