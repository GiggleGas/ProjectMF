#include "TaskSystem/TaskSystemDataAsset.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "Json.h"
// 内部解析辅助函数
bool UTaskSystemDataAsset::ParseTaskFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskDefinition& OutTask)
{
    return FTaskDefinition::LoadFromJsonObject(JsonObject, OutTask);
}

bool UTaskSystemDataAsset::ParseRoomFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FRoomDefinition& OutRoom)
{
    return FRoomDefinition::LoadFromJsonObject(JsonObject, OutRoom);
}

bool UTaskSystemDataAsset::ParseTaskSetFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetConfig& OutTaskSet)
{
    return FTaskSetConfig::LoadFromJsonObject(JsonObject, OutTaskSet);
}

// 实现其他方法
FTaskDefinition* UTaskSystemDataAsset::GetTaskById(const FString& TaskId)
{
    return GetTaskByIdInternal(TaskId);
}

FRoomDefinition* UTaskSystemDataAsset::GetRoomByName(const FString& RoomName)
{
    return GetRoomByNameInternal(RoomName);
}

FTaskSetConfig* UTaskSystemDataAsset::GetTaskSetById(const FString& TaskSetId)
{
    return GetTaskSetByIdInternal(TaskSetId);
}

int32 UTaskSystemDataAsset::FindTaskIndex(const FString& TaskId)
{
    for (int32 i = 0; i < Tasks.Num(); ++i)
    {
        if (Tasks[i].TaskId.Equals(TaskId, ESearchCase::IgnoreCase))
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UTaskSystemDataAsset::FindRoomIndex(const FString& RoomName)
{
    for (int32 i = 0; i < Rooms.Num(); ++i)
    {
        if (Rooms[i].RoomName.Equals(RoomName, ESearchCase::IgnoreCase))
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UTaskSystemDataAsset::FindTaskSetIndex(const FString& TaskSetId)
{
    for (int32 i = 0; i < TaskSets.Num(); ++i)
    {
        if (TaskSets[i].TaskSetId.Equals(TaskSetId, ESearchCase::IgnoreCase))
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void UTaskSystemDataAsset::AddTask(const FTaskDefinition& Task)
{
    Tasks.Add(Task);
}

void UTaskSystemDataAsset::AddRoom(const FRoomDefinition& Room)
{
    Rooms.Add(Room);
}

void UTaskSystemDataAsset::AddTaskSet(const FTaskSetConfig& TaskSet)
{
    TaskSets.Add(TaskSet);
}

FTaskDefinition* UTaskSystemDataAsset::GetTaskByIdInternal(const FString& TaskId)
{
    for (FTaskDefinition& Task : Tasks)
    {
        if (Task.TaskId.Equals(TaskId, ESearchCase::IgnoreCase))
        {
            return &Task;
        }
    }
    return nullptr;
}

FRoomDefinition* UTaskSystemDataAsset::GetRoomByNameInternal(const FString& RoomName)
{
    for (FRoomDefinition& Room : Rooms)
    {
        if (Room.RoomName.Equals(RoomName, ESearchCase::IgnoreCase))
        {
            return &Room;
        }
    }
    return nullptr;
}

FTaskSetConfig* UTaskSystemDataAsset::GetTaskSetByIdInternal(const FString& TaskSetId)
{
    for (FTaskSetConfig& TaskSet : TaskSets)
    {
        if (TaskSet.TaskSetId.Equals(TaskSetId, ESearchCase::IgnoreCase))
        {
            return &TaskSet;
        }
    }
    return nullptr;
}

FPrimaryAssetId UTaskSystemDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId("TaskSystemDataAsset", GetFName());
}

// 获取锁和钥匙系统配置
FLockAndKeySystem* UTaskSystemDataAsset::GetLockAndKeySystem()
{
    return &LockAndKeySystem;
}

// 添加锁和钥匙系统配置
void UTaskSystemDataAsset::AddLockAndKeySystem(const FLockAndKeySystem& LockAndKeySys)
{
    LockAndKeySystem = LockAndKeySys;
}



// 从指定路径加载锁和钥匙系统配置
bool UTaskSystemDataAsset::LoadLockAndKeySystemFromJson(const FString& JsonFilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *JsonFilePath);
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
        return false;
    }

    // 解析锁和钥匙系统配置
    if (JsonObject->HasField(TEXT("lock_and_key_system")))
    {
        const TSharedPtr<FJsonObject>& LockAndKeyObj = JsonObject->GetObjectField(TEXT("lock_and_key_system"));
        if (LockAndKeyObj.IsValid())
        {
            FLockAndKeySystem::LoadFromJsonObject(LockAndKeyObj, LockAndKeySystem);
        }
    }
    // 如果直接就是锁和钥匙系统的配置
    else
    {
        FLockAndKeySystem::LoadFromJsonObject(JsonObject, LockAndKeySystem);
    }

    return true;
}



// 从JSON文件加载任务系统数据
bool UTaskSystemDataAsset::LoadFromJson(const FString& JsonFilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *JsonFilePath);
        return false;
    }

    return LoadFromJsonString(JsonString);
}

// 从JSON字符串加载任务系统数据
bool UTaskSystemDataAsset::LoadFromJsonString(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
        return false;
    }

    // 清空现有数据
    Tasks.Empty();
    Rooms.Empty();
    TaskSets.Empty();

    // 解析任务
    if (JsonObject->HasField(TEXT("tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskArray = JsonObject->GetArrayField(TEXT("tasks"));
        for (const auto& TaskValue : TaskArray)
        {
            if (TaskValue.IsValid() && TaskValue->Type == EJson::Object)
            {
                FTaskDefinition Task;
                if (ParseTaskFromJsonObject(TaskValue->AsObject(), Task))
                {
                    Tasks.Add(Task);
                }
            }
        }
    }

    // 解析房间
    if (JsonObject->HasField(TEXT("rooms")))
    {
        const TArray<TSharedPtr<FJsonValue>>& RoomArray = JsonObject->GetArrayField(TEXT("rooms"));
        for (const auto& RoomValue : RoomArray)
        {
            if (RoomValue.IsValid() && RoomValue->Type == EJson::Object)
            {
                FRoomDefinition Room;
                if (ParseRoomFromJsonObject(RoomValue->AsObject(), Room))
                {
                    Rooms.Add(Room);
                }
            }
        }
    }

    // 解析任务集
    if (JsonObject->HasField(TEXT("tasksets")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskSetArray = JsonObject->GetArrayField(TEXT("tasksets"));
        for (const auto& TaskSetValue : TaskSetArray)
        {
            if (TaskSetValue.IsValid() && TaskSetValue->Type == EJson::Object)
            {
                FTaskSetConfig TaskSet;
                if (ParseTaskSetFromJsonObject(TaskSetValue->AsObject(), TaskSet))
                {
                    TaskSets.Add(TaskSet);
                }
            }
        }
    }

    return true;
}

// 从JSON目录加载所有数据
bool UTaskSystemDataAsset::LoadFromJsonDirectory(const FString& JsonDirectoryPath)
{
    // 清空现有数据
    Tasks.Empty();
    Rooms.Empty();
    TaskSets.Empty();
    LockAndKeySystem.Empty();
    
    // 加载所有JSON目录中的数据
    bool bSuccess = true;
    {
        FString LockAndKeyFile = FPaths::Combine(JsonDirectoryPath, TEXT("LocksAndKeys.json"));
        bSuccess &= LoadLockAndKeySystemFromJson(LockAndKeyFile);
    }
    bSuccess &= LoadTasksFromJsonDirectoryInternal(JsonDirectoryPath);
    bSuccess &= LoadRoomsFromJsonDirectoryInternal(JsonDirectoryPath);
    bSuccess &= LoadTaskSetsFromJsonDirectoryInternal(JsonDirectoryPath);

    return bSuccess;
}

// 从资源包加载任务系统（支持多个JSON文件）
bool UTaskSystemDataAsset::LoadFromPackage(const FString& PackagePath)
{
    // 清空现有数据
    Tasks.Empty();
    Rooms.Empty();
    TaskSets.Empty();

    // 假设PackagePath是一个目录，包含Task、Room、TaskSet子目录
    FString BasePath = PackagePath;
    if (!BasePath.EndsWith(TEXT("/")) && !BasePath.EndsWith(TEXT("\\")))
    {
        BasePath += TEXT("/");
    }

    // 加载任务
    FString TaskPath = BasePath + TEXT("Task/");
    LoadTasksFromJsonDirectoryInternal(TaskPath);

    // 加载房间
    FString RoomPath = BasePath + TEXT("Room/");
    LoadRoomsFromJsonDirectoryInternal(RoomPath);

    // 加载任务集
    FString TaskSetPath = BasePath + TEXT("TaskSet/");
    LoadTaskSetsFromJsonDirectoryInternal(TaskSetPath);

    return true;
}

// 从指定路径加载任务
bool UTaskSystemDataAsset::LoadTasksFromJsonDirectory(const FString& JsonDirectoryPath)
{
    Tasks.Empty();
    return LoadTasksFromJsonDirectoryInternal(JsonDirectoryPath);
}

// 从指定路径加载房间
bool UTaskSystemDataAsset::LoadRoomsFromJsonDirectory(const FString& JsonDirectoryPath)
{
    Rooms.Empty();
    return LoadRoomsFromJsonDirectoryInternal(JsonDirectoryPath);
}

// 从指定路径加载任务集
bool UTaskSystemDataAsset::LoadTaskSetsFromJsonDirectory(const FString& JsonDirectoryPath)
{
    TaskSets.Empty();
    return LoadTaskSetsFromJsonDirectoryInternal(JsonDirectoryPath);
}

// 验证JSON数据格式
bool UTaskSystemDataAsset::ValidateJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid JSON format"));
        return false;
    }

    // 检查是否包含必需的字段
    bool bHasTasks = JsonObject->HasField(TEXT("tasks"));
    bool bHasRooms = JsonObject->HasField(TEXT("rooms"));
    bool bHasTaskSets = JsonObject->HasField(TEXT("tasksets"));

    // 至少应包含其中一个部分
    return bHasTasks || bHasRooms || bHasTaskSets;
}

// 将任务系统数据保存为JSON
bool UTaskSystemDataAsset::SaveToJson(const FString& OutputPath)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    // 序列化任务
    if (Tasks.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> TaskArray;
        for (const FTaskDefinition& Task : Tasks)
        {
            TSharedPtr<FJsonObject> TaskObj = MakeShareable(new FJsonObject);
            
            TaskObj->SetStringField(TEXT("id"), Task.TaskId);
            
            // 锁定数组
            TArray<TSharedPtr<FJsonValue>> LocksArray;
            for (const FString& Lock : Task.Locks)
            {
                LocksArray.Add(MakeShareable(new FJsonValueString(Lock)));
            }
            TaskObj->SetArrayField(TEXT("locks"), LocksArray);
            
            // 密钥数组
            TArray<TSharedPtr<FJsonValue>> KeysArray;
            for (const FString& Key : Task.KeysGiven)
            {
                KeysArray.Add(MakeShareable(new FJsonValueString(Key)));
            }
            TaskObj->SetArrayField(TEXT("keys_given"), KeysArray);
            
            TaskObj->SetStringField(TEXT("region_id"), Task.RegionId);
            TaskObj->SetStringField(TEXT("entrance_room"), Task.EntranceRoom);
            TaskObj->SetNumberField(TEXT("entrance_room_chance"), Task.EntranceRoomChance);
            TaskObj->SetStringField(TEXT("room_bg"), Task.RoomBg);
            TaskObj->SetStringField(TEXT("background_room"), Task.BackgroundRoom);
            
            // 颜色
            TSharedPtr<FJsonObject> ColorObj = MakeShareable(new FJsonObject);
            ColorObj->SetNumberField(TEXT("r"), Task.Colour.R);
            ColorObj->SetNumberField(TEXT("g"), Task.Colour.G);
            ColorObj->SetNumberField(TEXT("b"), Task.Colour.B);
            ColorObj->SetNumberField(TEXT("a"), Task.Colour.A);
            TaskObj->SetObjectField(TEXT("colour"), ColorObj);
            
            // 房间选择
            TSharedPtr<FJsonObject> RoomChoicesObj = MakeShareable(new FJsonObject);
            for (const auto& Pair : Task.RoomChoices)
            {
                RoomChoicesObj->SetNumberField(Pair.Key, Pair.Value);
            }
            TaskObj->SetObjectField(TEXT("room_choices"), RoomChoicesObj);
            
            // 必需预设
            TArray<TSharedPtr<FJsonValue>> RequiredPrefabsArray;
            for (const FString& Prefab : Task.RequiredPrefabs)
            {
                RequiredPrefabsArray.Add(MakeShareable(new FJsonValueString(Prefab)));
            }
            TaskObj->SetArrayField(TEXT("required_prefabs"), RequiredPrefabsArray);
            
            TaskArray.Add(MakeShareable(new FJsonValueObject(TaskObj)));
        }
        JsonObject->SetArrayField(TEXT("tasks"), TaskArray);
    }

    // 序列化房间（类似的方式）
    if (Rooms.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> RoomArray;
        for (const FRoomDefinition& Room : Rooms)
        {
            TSharedPtr<FJsonObject> RoomObj = MakeShareable(new FJsonObject);
            
            RoomObj->SetStringField(TEXT("name"), Room.RoomName);
            
            // 颜色
            TSharedPtr<FJsonObject> ColorObj = MakeShareable(new FJsonObject);
            ColorObj->SetNumberField(TEXT("r"), Room.Colour.R);
            ColorObj->SetNumberField(TEXT("g"), Room.Colour.G);
            ColorObj->SetNumberField(TEXT("b"), Room.Colour.B);
            ColorObj->SetNumberField(TEXT("a"), Room.Colour.A);
            RoomObj->SetObjectField(TEXT("colour"), ColorObj);
            
            RoomObj->SetStringField(TEXT("value"), Room.Value);
            RoomObj->SetStringField(TEXT("type"), Room.Type);
            
            // 标签数组
            TArray<TSharedPtr<FJsonValue>> TagsArray;
            for (const FString& Tag : Room.Tags)
            {
                TagsArray.Add(MakeShareable(new FJsonValueString(Tag)));
            }
            RoomObj->SetArrayField(TEXT("tags"), TagsArray);
            
            // 内容
            TSharedPtr<FJsonObject> ContentsObj = MakeShareable(new FJsonObject);
            
            // 静态布局
            if (Room.StaticLayouts.Num() > 0)
            {
                TSharedPtr<FJsonObject> StaticLayoutsObj = MakeShareable(new FJsonObject);
                for (const auto& Pair : Room.StaticLayouts)
                {
                    StaticLayoutsObj->SetNumberField(Pair.Key, Pair.Value);
                }
                ContentsObj->SetObjectField(TEXT("countstaticlayouts"), StaticLayoutsObj);
            }
            
            // 预制体计数
            if (Room.CountPrefabs.Num() > 0)
            {
                TSharedPtr<FJsonObject> CountPrefabsObj = MakeShareable(new FJsonObject);
                for (const auto& Pair : Room.CountPrefabs)
                {
                    CountPrefabsObj->SetNumberField(Pair.Key, Pair.Value);
                }
                ContentsObj->SetObjectField(TEXT("countprefabs"), CountPrefabsObj);
            }
            
            // 分布预制体
            if (Room.DistributedPrefabs.Num() > 0)
            {
                TSharedPtr<FJsonObject> DistPrefabsObj = MakeShareable(new FJsonObject);
                for (const auto& Pair : Room.DistributedPrefabs)
                {
                    DistPrefabsObj->SetNumberField(Pair.Key, Pair.Value);
                }
                ContentsObj->SetObjectField(TEXT("distributeprefabs"), DistPrefabsObj);
            }
            
            ContentsObj->SetNumberField(TEXT("distributepercent"), Room.DistributePercent);
            
            RoomObj->SetObjectField(TEXT("contents"), ContentsObj);
            
            RoomArray.Add(MakeShareable(new FJsonValueObject(RoomObj)));
        }
        JsonObject->SetArrayField(TEXT("rooms"), RoomArray);
    }

    // 序列化任务集
    if (TaskSets.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> TaskSetArray;
        for (const FTaskSetConfig& TaskSet : TaskSets)
        {
            TSharedPtr<FJsonObject> TaskSetObj = MakeShareable(new FJsonObject);
            
            TaskSetObj->SetStringField(TEXT("id"), TaskSet.TaskSetId);
            TaskSetObj->SetStringField(TEXT("name"), TaskSet.Name.ToString());
            TaskSetObj->SetStringField(TEXT("location"), TaskSet.Location);
            
            // 任务数组
            TArray<TSharedPtr<FJsonValue>> TasksArray;
            for (const FString& Task : TaskSet.Tasks)
            {
                TasksArray.Add(MakeShareable(new FJsonValueString(Task)));
            }
            TaskSetObj->SetArrayField(TEXT("tasks"), TasksArray);
            
            TaskSetObj->SetNumberField(TEXT("numoptionaltasks"), TaskSet.NumOptionalTasks);
            
            // 可选任务数组
            TArray<TSharedPtr<FJsonValue>> OptionalTasksArray;
            for (const FString& Task : TaskSet.OptionalTasks)
            {
                OptionalTasksArray.Add(MakeShareable(new FJsonValueString(Task)));
            }
            TaskSetObj->SetArrayField(TEXT("optionaltasks"), OptionalTasksArray);
            
            // 有效起始任务数组
            TArray<TSharedPtr<FJsonValue>> ValidStartTasksArray;
            for (const FString& Task : TaskSet.ValidStartTasks)
            {
                ValidStartTasksArray.Add(MakeShareable(new FJsonValueString(Task)));
            }
            TaskSetObj->SetArrayField(TEXT("valid_start_tasks"), ValidStartTasksArray);
            
            // 必需预设数组
            TArray<TSharedPtr<FJsonValue>> RequiredPrefabsArray;
            for (const FString& Prefab : TaskSet.RequiredPrefabs)
            {
                RequiredPrefabsArray.Add(MakeShareable(new FJsonValueString(Prefab)));
            }
            TaskSetObj->SetArrayField(TEXT("required_prefabs"), RequiredPrefabsArray);
            
            // 设置片段
            TSharedPtr<FJsonObject> SetPiecesObj = MakeShareable(new FJsonObject);
            for (const auto& Pair : TaskSet.SetPieces)
            {
                TSharedPtr<FJsonObject> PieceObj = MakeShareable(new FJsonObject);
                PieceObj->SetNumberField(TEXT("count"), Pair.Value.Count);
                
                TArray<TSharedPtr<FJsonValue>> PieceTasksArray;
                for (const FString& PieceTask : Pair.Value.Tasks)
                {
                    PieceTasksArray.Add(MakeShareable(new FJsonValueString(PieceTask)));
                }
                PieceObj->SetArrayField(TEXT("tasks"), PieceTasksArray);
                
                SetPiecesObj->SetObjectField(Pair.Key, PieceObj);
            }
            TaskSetObj->SetObjectField(TEXT("set_pieces"), SetPiecesObj);
            
            TaskSetArray.Add(MakeShareable(new FJsonValueObject(TaskSetObj)));
        }
        JsonObject->SetArrayField(TEXT("tasksets"), TaskSetArray);
    }

    // 将JSON写入文件
    FString OutputString;
    auto Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
    {
        return FFileHelper::SaveStringToFile(OutputString, *OutputPath);
    }
    
    return false;
}

// 验证任务系统数据完整性
bool UTaskSystemDataAsset::ValidateData()
{
    // 验证任务数据
    for (const FTaskDefinition& Task : Tasks)
    {
        if (Task.TaskId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Task with empty ID found"));
            return false;
        }
    }

    // 验证房间数据
    for (const FRoomDefinition& Room : Rooms)
    {
        if (Room.RoomName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Room with empty name found"));
            return false;
        }
    }

    // 验证任务集数据
    for (const FTaskSetConfig& TaskSet : TaskSets)
    {
        if (TaskSet.TaskSetId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("TaskSet with empty ID found"));
            return false;
        }
    }

    return true;
}

// 分别加载任务数据
bool UTaskSystemDataAsset::LoadTasksFromJson(const FString& JsonFilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *JsonFilePath);
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
        return false;
    }

    // 解析任务
    if (JsonObject->HasField(TEXT("tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskArray = JsonObject->GetArrayField(TEXT("tasks"));
        for (const auto& TaskValue : TaskArray)
        {
            if (TaskValue.IsValid() && TaskValue->Type == EJson::Object)
            {
                FTaskDefinition Task;
                if (ParseTaskFromJsonObject(TaskValue->AsObject(), Task))
                {
                    Tasks.Add(Task);
                }
            }
        }
    }

    return true;
}

// 分别加载房间数据
bool UTaskSystemDataAsset::LoadRoomsFromJson(const FString& JsonFilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *JsonFilePath);
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
        return false;
    }

    // 解析房间
    if (JsonObject->HasField(TEXT("rooms")))
    {
        const TArray<TSharedPtr<FJsonValue>>& RoomArray = JsonObject->GetArrayField(TEXT("rooms"));
        for (const auto& RoomValue : RoomArray)
        {
            if (RoomValue.IsValid() && RoomValue->Type == EJson::Object)
            {
                FRoomDefinition Room;
                if (ParseRoomFromJsonObject(RoomValue->AsObject(), Room))
                {
                    Rooms.Add(Room);
                }
            }
        }
    }

    return true;
}

// 分别加载任务集数据
bool UTaskSystemDataAsset::LoadTaskSetsFromJson(const FString& JsonFilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *JsonFilePath);
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
        return false;
    }

    // 解析任务集
    if (JsonObject->HasField(TEXT("tasksets")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskSetArray = JsonObject->GetArrayField(TEXT("tasksets"));
        for (const auto& TaskSetValue : TaskSetArray)
        {
            if (TaskSetValue.IsValid() && TaskSetValue->Type == EJson::Object)
            {
                FTaskSetConfig TaskSet;
                if (ParseTaskSetFromJsonObject(TaskSetValue->AsObject(), TaskSet))
                {
                    TaskSets.Add(TaskSet);
                }
            }
        }
    }

    return true;
}

// 内部方法：从JSON目录加载任务
bool UTaskSystemDataAsset::LoadTasksFromJsonDirectoryInternal(const FString& JsonDirectoryPath)
{
    FString TaskDir = FPaths::Combine(JsonDirectoryPath, TEXT("Task"));
    
    IFileManager& FileManager = FFileManagerGeneric::Get();
    
    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *TaskDir, TEXT("json"));

    bool bSuccess = true;
    for (const FString& FileName : FoundFiles)
    {
        FString FullPath = FPaths::Combine(TaskDir, FileName);
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *FullPath))
        {
            TArray<FTaskDefinition> LoadedTasks = ParseTasksFromJson(JsonString);
            
            if (LoadedTasks.Num() > 0)
            {
                Tasks.Append(LoadedTasks);
                UE_LOG(LogTemp, Log, TEXT("Successfully loaded %d tasks from %s"), LoadedTasks.Num(), *FullPath);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No tasks found in %s"), *FullPath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FullPath);
            bSuccess = false;
        }
    }

    return bSuccess;
}

// 内部方法：从JSON目录加载房间
bool UTaskSystemDataAsset::LoadRoomsFromJsonDirectoryInternal(const FString& JsonDirectoryPath)
{
    FString RoomDir = FPaths::Combine(JsonDirectoryPath, TEXT("Room"));
    
    IFileManager& FileManager = FFileManagerGeneric::Get();
    
    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *RoomDir, TEXT("json"));

    bool bSuccess = true;
    for (const FString& FileName : FoundFiles)
    {
        FString FullPath = FPaths::Combine(RoomDir, FileName);
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *FullPath))
        {
            TArray<FRoomDefinition> LoadedRooms = ParseRoomsFromJson(JsonString);
            
            if (LoadedRooms.Num() > 0)
            {
                Rooms.Append(LoadedRooms);
                UE_LOG(LogTemp, Log, TEXT("Successfully loaded %d rooms from %s"), LoadedRooms.Num(), *FullPath);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No rooms found in %s"), *FullPath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FullPath);
            bSuccess = false;
        }
    }

    return bSuccess;
}

// 内部方法：从JSON目录加载任务集
bool UTaskSystemDataAsset::LoadTaskSetsFromJsonDirectoryInternal(const FString& JsonDirectoryPath)
{
    FString TaskSetDir = FPaths::Combine(JsonDirectoryPath, TEXT("TaskSet"));
    
    IFileManager& FileManager = FFileManagerGeneric::Get();
    
    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *TaskSetDir, TEXT("json"));

    bool bSuccess = true;
    for (const FString& FileName : FoundFiles)
    {
        FString FullPath = FPaths::Combine(TaskSetDir, FileName);
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *FullPath))
        {
            TArray<FTaskSetConfig> LoadedTaskSets = ParseTaskSetsFromJson(JsonString);
            
            if (LoadedTaskSets.Num() > 0)
            {
                TaskSets.Append(LoadedTaskSets);
                UE_LOG(LogTemp, Log, TEXT("Successfully loaded %d tasksets from %s"), LoadedTaskSets.Num(), *FullPath);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No tasksets found in %s"), *FullPath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FullPath);
            bSuccess = false;
        }
    }

    return bSuccess;
}

// 内部解析辅助函数
TArray<FTaskDefinition> UTaskSystemDataAsset::ParseTasksFromJson(const FString& JsonString)
{
    TArray<FTaskDefinition> Result;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string in ParseTasksFromJson"));
        return Result;
    }

    // 解析任务
    if (JsonObject->HasField(TEXT("tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskArray = JsonObject->GetArrayField(TEXT("tasks"));
        for (const auto& TaskValue : TaskArray)
        {
            if (TaskValue.IsValid() && TaskValue->Type == EJson::Object)
            {
                FTaskDefinition Task;
                if (ParseTaskFromJsonObject(TaskValue->AsObject(), Task))
                {
                    Result.Add(Task);
                }
            }
        }
    }
    
    return Result;
}

TArray<FRoomDefinition> UTaskSystemDataAsset::ParseRoomsFromJson(const FString& JsonString)
{
    TArray<FRoomDefinition> Result;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string in ParseRoomsFromJson"));
        return Result;
    }

    // 解析房间
    if (JsonObject->HasField(TEXT("rooms")))
    {
        const TArray<TSharedPtr<FJsonValue>>& RoomArray = JsonObject->GetArrayField(TEXT("rooms"));
        for (const auto& RoomValue : RoomArray)
        {
            if (RoomValue.IsValid() && RoomValue->Type == EJson::Object)
            {
                FRoomDefinition Room;
                if (ParseRoomFromJsonObject(RoomValue->AsObject(), Room))
                {
                    Result.Add(Room);
                }
            }
        }
    }
    
    return Result;
}

TArray<FTaskSetConfig> UTaskSystemDataAsset::ParseTaskSetsFromJson(const FString& JsonString)
{
    TArray<FTaskSetConfig> Result;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string in ParseTaskSetsFromJson"));
        return Result;
    }

    // 解析任务集
    if (JsonObject->HasField(TEXT("tasksets")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskSetArray = JsonObject->GetArrayField(TEXT("tasksets"));
        for (const auto& TaskSetValue : TaskSetArray)
        {
            if (TaskSetValue.IsValid() && TaskSetValue->Type == EJson::Object)
            {
                FTaskSetConfig TaskSet;
                if (ParseTaskSetFromJsonObject(TaskSetValue->AsObject(), TaskSet))
                {
                    Result.Add(TaskSet);
                }
            }
        }
    }
    
    return Result;
}
