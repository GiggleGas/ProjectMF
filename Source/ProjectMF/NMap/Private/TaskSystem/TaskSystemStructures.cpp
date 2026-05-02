// d:\Documents\Unreal Projects\ProjectMF\Source\ProjectMF\NMap\Private\TaskSystem\TaskSystemDataAsset.cpp
#include "TaskSystem/TaskSystemStructures.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "Json.h"


// 实现FLockAndKeySystem的静态加载函数
bool FLockAndKeySystem::LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FLockAndKeySystem& OutLockAndKeySystem)
{
    if (!JsonObject.IsValid())
        return false;

    // 加载Keys数组
    if (JsonObject->HasField(TEXT("keys")))
    {
        const TArray<TSharedPtr<FJsonValue>>& KeysArray = JsonObject->GetArrayField(TEXT("keys"));
        for (const auto& KeyValue : KeysArray)
        {
            if (KeyValue.IsValid() && KeyValue->Type == EJson::String)
            {
                OutLockAndKeySystem.Keys.Add(KeyValue->AsString());
            }
        }
    }

    // 加载Locks数组
    if (JsonObject->HasField(TEXT("locks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& LocksArray = JsonObject->GetArrayField(TEXT("locks"));
        for (const auto& LockValue : LocksArray)
        {
            if (LockValue.IsValid() && LockValue->Type == EJson::String)
            {
                OutLockAndKeySystem.Locks.Add(LockValue->AsString());
            }
        }
    }

    // 加载LocksKeys映射（锁ID到钥匙列表的映射）
    if (JsonObject->HasField(TEXT("locks_keys")))
    {
        const TSharedPtr<FJsonObject>& LocksKeysObj = JsonObject->GetObjectField(TEXT("locks_keys"));
        for (auto LocksKeysIt = LocksKeysObj->Values.CreateIterator(); LocksKeysIt; ++LocksKeysIt)
        {
            const FString& LockId = LocksKeysIt.Key();
            const TSharedPtr<FJsonValue>& Value = LocksKeysIt.Value();
            
            if (Value.IsValid() && Value->Type == EJson::Array)
            {
                TArray<FString> KeyList;
                const TArray<TSharedPtr<FJsonValue>>& KeyArray = Value->AsArray();
                for (const auto& KeyItem : KeyArray)
                {
                    if (KeyItem.IsValid() && KeyItem->Type == EJson::String)
                    {
                        KeyList.Add(KeyItem->AsString());
                    }
                }
                OutLockAndKeySystem.LocksKeys.Add(LockId, KeyList);
            }
        }
    }

    return true;
}

// 实现结构体的静态加载函数
bool FTaskDefinition::LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskDefinition& OutTask)
{
    if (!JsonObject.IsValid())
        return false;

    // 解析基本字段
    if (JsonObject->HasField(TEXT("id")))
        OutTask.TaskId = JsonObject->GetStringField(TEXT("id"));

    if (JsonObject->HasField(TEXT("locks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& LockArray = JsonObject->GetArrayField(TEXT("locks"));
        for (const auto& LockValue : LockArray)
        {
            if (LockValue.IsValid() && LockValue->Type == EJson::String)
            {
                OutTask.Locks.Add(LockValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("keys_given")))
    {
        const TArray<TSharedPtr<FJsonValue>>& KeyArray = JsonObject->GetArrayField(TEXT("keys_given"));
        for (const auto& KeyValue : KeyArray)
        {
            if (KeyValue.IsValid() && KeyValue->Type == EJson::String)
            {
                OutTask.KeysGiven.Add(KeyValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("region_id")))
        OutTask.RegionId = JsonObject->GetStringField(TEXT("region_id"));

    if (JsonObject->HasField(TEXT("entrance_room")))
        OutTask.EntranceRoom = JsonObject->GetStringField(TEXT("entrance_room"));

    if (JsonObject->HasField(TEXT("entrance_room_chance")))
        OutTask.EntranceRoomChance = JsonObject->GetNumberField(TEXT("entrance_room_chance"));

    if (JsonObject->HasField(TEXT("room_bg")))
        OutTask.RoomBg = JsonObject->GetStringField(TEXT("room_bg"));

    if (JsonObject->HasField(TEXT("background_room")))
        OutTask.BackgroundRoom = JsonObject->GetStringField(TEXT("background_room"));

    if (JsonObject->HasField(TEXT("colour")))
    {
        const TSharedPtr<FJsonObject>& ColorObj = JsonObject->GetObjectField(TEXT("colour"));
        if (ColorObj.IsValid())
        {
            float R = ColorObj->HasField(TEXT("r")) ? ColorObj->GetNumberField(TEXT("r")) : 1.0f;
            float G = ColorObj->HasField(TEXT("g")) ? ColorObj->GetNumberField(TEXT("g")) : 1.0f;
            float B = ColorObj->HasField(TEXT("b")) ? ColorObj->GetNumberField(TEXT("b")) : 1.0f;
            float A = ColorObj->HasField(TEXT("a")) ? ColorObj->GetNumberField(TEXT("a")) : 1.0f;
            OutTask.Colour = FLinearColor(R, G, B, A);
        }
    }

    if (JsonObject->HasField(TEXT("room_choices")))
    {
        const TSharedPtr<FJsonObject>& ChoicesObj = JsonObject->GetObjectField(TEXT("room_choices"));
        for (const auto& ChoicePair : ChoicesObj->Values)
        {
            if (ChoicePair.Value.IsValid() && ChoicePair.Value->Type == EJson::Number)
            {
                OutTask.RoomChoices.Add(ChoicePair.Key, ChoicePair.Value->AsNumber());
            }
        }
    }

    if (JsonObject->HasField(TEXT("room_choices_special")))
    {
        const TSharedPtr<FJsonObject>& SpecialChoicesObj = JsonObject->GetObjectField(TEXT("room_choices_special"));
        for (const auto& SpecialChoicePair : SpecialChoicesObj->Values)
        {
            if (SpecialChoicePair.Value.IsValid() && SpecialChoicePair.Value->Type == EJson::Number)
            {
                OutTask.RoomChoicesSpecial.Add(SpecialChoicePair.Key, SpecialChoicePair.Value->AsNumber());
            }
        }
    }

    if (JsonObject->HasField(TEXT("required_prefabs")))
    {
        const TArray<TSharedPtr<FJsonValue>>& PrefabArray = JsonObject->GetArrayField(TEXT("required_prefabs"));
        for (const auto& PrefabValue : PrefabArray)
        {
            if (PrefabValue.IsValid() && PrefabValue->Type == EJson::String)
            {
                OutTask.RequiredPrefabs.Add(PrefabValue->AsString());
            }
        }
    }

    // 解析可选字段
    if (JsonObject->HasField(TEXT("cove_room_name")))
        OutTask.CoveRoomName = JsonObject->GetStringField(TEXT("cove_room_name"));

    if (JsonObject->HasField(TEXT("cove_room_chance")))
        OutTask.CoveRoomChance = JsonObject->GetNumberField(TEXT("cove_room_chance"));

    if (JsonObject->HasField(TEXT("cove_room_max_edges")))
        OutTask.CoveRoomMaxEdges = JsonObject->GetIntegerField(TEXT("cove_room_max_edges"));

    if (JsonObject->HasField(TEXT("maze_tiles")))
    {
        const TArray<TSharedPtr<FJsonValue>>& MazeTilesArray = JsonObject->GetArrayField(TEXT("maze_tiles"));
        for (const auto& TileValue : MazeTilesArray)
        {
            if (TileValue.IsValid() && TileValue->Type == EJson::String)
            {
                OutTask.MazeTiles.Add(TileValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("maze_tile_size")))
        OutTask.MazeTileSize = JsonObject->GetIntegerField(TEXT("maze_tile_size"));

    if (JsonObject->HasField(TEXT("crosslink_factor")))
        OutTask.CrosslinkFactor = JsonObject->GetNumberField(TEXT("crosslink_factor"));

    if (JsonObject->HasField(TEXT("make_loop")))
        OutTask.bMakeLoop = JsonObject->GetBoolField(TEXT("make_loop"));

    if (JsonObject->HasField(TEXT("room_tags")))
    {
        const TArray<TSharedPtr<FJsonValue>>& RoomTagsArray = JsonObject->GetArrayField(TEXT("room_tags"));
        for (const auto& TagValue : RoomTagsArray)
        {
            if (TagValue.IsValid() && TagValue->Type == EJson::String)
            {
                OutTask.RoomTags.Add(TagValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("hub_room")))
        OutTask.HubRoom = JsonObject->GetStringField(TEXT("hub_room"));

    if (JsonObject->HasField(TEXT("level_setpiece_blocker")))
        OutTask.bLevelSetPieceBlocker = JsonObject->GetBoolField(TEXT("level_setpiece_blocker"));

    return true;
}

bool FRoomDefinition::LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FRoomDefinition& OutRoom)
{
    if (!JsonObject.IsValid())
        return false;

    // 尝试解析 id 或 name 字段作为房间名称
    if (JsonObject->HasField(TEXT("id")))
        OutRoom.RoomName = JsonObject->GetStringField(TEXT("id"));
    else if (JsonObject->HasField(TEXT("name")))
        OutRoom.RoomName = JsonObject->GetStringField(TEXT("name"));

    if (JsonObject->HasField(TEXT("colour")))
    {
        const TSharedPtr<FJsonObject>& ColorObj = JsonObject->GetObjectField(TEXT("colour"));
        if (ColorObj.IsValid())
        {
            float R = ColorObj->HasField(TEXT("r")) ? ColorObj->GetNumberField(TEXT("r")) : 1.0f;
            float G = ColorObj->HasField(TEXT("g")) ? ColorObj->GetNumberField(TEXT("g")) : 1.0f;
            float B = ColorObj->HasField(TEXT("b")) ? ColorObj->GetNumberField(TEXT("b")) : 1.0f;
            float A = ColorObj->HasField(TEXT("a")) ? ColorObj->GetNumberField(TEXT("a")) : 1.0f;
            OutRoom.Colour = FLinearColor(R, G, B, A);
        }
    }

    if (JsonObject->HasField(TEXT("value")))
        OutRoom.Value = JsonObject->GetStringField(TEXT("value"));

    if (JsonObject->HasField(TEXT("type")))
        OutRoom.Type = JsonObject->GetStringField(TEXT("type"));

    if (JsonObject->HasField(TEXT("tags")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TagArray = JsonObject->GetArrayField(TEXT("tags"));
        for (const auto& TagValue : TagArray)
        {
            if (TagValue.IsValid() && TagValue->Type == EJson::String)
            {
                OutRoom.Tags.Add(TagValue->AsString());
            }
        }
    }

    // 解析可能存在的其他内容字段
    if (JsonObject->HasField(TEXT("contents")))
    {
        const TSharedPtr<FJsonObject>& ContentsObj = JsonObject->GetObjectField(TEXT("contents"));
        
        if (ContentsObj->HasField(TEXT("countstaticlayouts")))
        {
            const TSharedPtr<FJsonObject>& LayoutsObj = ContentsObj->GetObjectField(TEXT("countstaticlayouts"));
            for (const auto& LayoutPair : LayoutsObj->Values)
            {
                if (LayoutPair.Value.IsValid() && LayoutPair.Value->Type == EJson::Number)
                {
                    OutRoom.StaticLayouts.Add(LayoutPair.Key, LayoutPair.Value->AsNumber());
                }
            }
        }

        if (ContentsObj->HasField(TEXT("countprefabs")))
        {
            const TSharedPtr<FJsonObject>& PrefabsObj = ContentsObj->GetObjectField(TEXT("countprefabs"));
            for (const auto& PrefabPair : PrefabsObj->Values)
            {
                if (PrefabPair.Value.IsValid() && PrefabPair.Value->Type == EJson::Number)
                {
                    OutRoom.CountPrefabs.Add(PrefabPair.Key, PrefabPair.Value->AsNumber());
                }
            }
        }

        if (ContentsObj->HasField(TEXT("distributeprefabs")))
        {
            const TSharedPtr<FJsonObject>& DistPrefabsObj = ContentsObj->GetObjectField(TEXT("distributeprefabs"));
            for (const auto& DistPrefabPair : DistPrefabsObj->Values)
            {
                if (DistPrefabPair.Value.IsValid() && DistPrefabPair.Value->Type == EJson::Number)
                {
                    OutRoom.DistributedPrefabs.Add(DistPrefabPair.Key, DistPrefabPair.Value->AsNumber());
                }
            }
        }

        if (ContentsObj->HasField(TEXT("distributepercent")))
        {
            OutRoom.DistributePercent = ContentsObj->GetNumberField(TEXT("distributepercent"));
        }
        
        // 处理prefabdata等其他字段（虽然FRoomDefinition没有专门字段存储，但我们可以扩展处理）
        if (ContentsObj->HasField(TEXT("prefabdata")))
        {
            // 目前忽略prefabdata字段，因为FRoomDefinition没有对应字段
            // 可以根据需要添加额外的处理
        }
    }

    return true;
}

bool FTaskSetPieceConfig::LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetPieceConfig& OutConfig)
{
    if (!JsonObject.IsValid())
        return false;

    if (JsonObject->HasField(TEXT("count")))
        OutConfig.Count = JsonObject->GetIntegerField(TEXT("count"));
    
    if (JsonObject->HasField(TEXT("tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskArray = JsonObject->GetArrayField(TEXT("tasks"));
        for (const auto& TaskValue : TaskArray)
        {
            if (TaskValue.IsValid() && TaskValue->Type == EJson::String)
            {
                OutConfig.Tasks.Add(TaskValue->AsString());
            }
        }
    }
    
    return true;
}

bool FTaskSetConfig::LoadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTaskSetConfig& OutTaskSet)
{
    if (!JsonObject.IsValid())
        return false;

    // 解析基本字段
    if (JsonObject->HasField(TEXT("id")))
        OutTaskSet.TaskSetId = JsonObject->GetStringField(TEXT("id"));

    if (JsonObject->HasField(TEXT("name")))
        OutTaskSet.Name = FText::FromString(JsonObject->GetStringField(TEXT("name")));

    if (JsonObject->HasField(TEXT("location")))
        OutTaskSet.Location = JsonObject->GetStringField(TEXT("location"));

    if (JsonObject->HasField(TEXT("tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& TaskArray = JsonObject->GetArrayField(TEXT("tasks"));
        for (const auto& TaskValue : TaskArray)
        {
            if (TaskValue.IsValid() && TaskValue->Type == EJson::String)
            {
                OutTaskSet.Tasks.Add(TaskValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("numoptionaltasks")))
        OutTaskSet.NumOptionalTasks = JsonObject->GetIntegerField(TEXT("numoptionaltasks"));

    if (JsonObject->HasField(TEXT("optionaltasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& OptionalTaskArray = JsonObject->GetArrayField(TEXT("optionaltasks"));
        for (const auto& OptionalTaskValue : OptionalTaskArray)
        {
            if (OptionalTaskValue.IsValid() && OptionalTaskValue->Type == EJson::String)
            {
                OutTaskSet.OptionalTasks.Add(OptionalTaskValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("valid_start_tasks")))
    {
        const TArray<TSharedPtr<FJsonValue>>& ValidStartArray = JsonObject->GetArrayField(TEXT("valid_start_tasks"));
        for (const auto& ValidStartValue : ValidStartArray)
        {
            if (ValidStartValue.IsValid() && ValidStartValue->Type == EJson::String)
            {
                OutTaskSet.ValidStartTasks.Add(ValidStartValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("required_prefabs")))
    {
        const TArray<TSharedPtr<FJsonValue>>& PrefabArray = JsonObject->GetArrayField(TEXT("required_prefabs"));
        for (const auto& PrefabValue : PrefabArray)
        {
            if (PrefabValue.IsValid() && PrefabValue->Type == EJson::String)
            {
                OutTaskSet.RequiredPrefabs.Add(PrefabValue->AsString());
            }
        }
    }

    if (JsonObject->HasField(TEXT("set_pieces")))
    {
        const TSharedPtr<FJsonObject>& SetPiecesObj = JsonObject->GetObjectField(TEXT("set_pieces"));
        for (auto SetPieceIt = SetPiecesObj->Values.CreateIterator(); SetPieceIt; ++SetPieceIt)
        {
            const FString& PieceName = SetPieceIt.Key();
            const TSharedPtr<FJsonValue>& PieceValue = SetPieceIt.Value();
            
            if (PieceValue.IsValid() && PieceValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& PieceObj = PieceValue->AsObject();
                FTaskSetPieceConfig PieceConfig;
                
                if (PieceObj->HasField(TEXT("count")))
                    PieceConfig.Count = PieceObj->GetIntegerField(TEXT("count"));
                
                if (PieceObj->HasField(TEXT("tasks")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& TaskArray = PieceObj->GetArrayField(TEXT("tasks"));
                    for (const auto& TaskValue : TaskArray)
                    {
                        if (TaskValue.IsValid() && TaskValue->Type == EJson::String)
                        {
                            PieceConfig.Tasks.Add(TaskValue->AsString());
                        }
                    }
                }
                
                OutTaskSet.SetPieces.Add(PieceName, PieceConfig);
            }
        }
    }

    if (JsonObject->HasField(TEXT("ocean_prefill_setpieces")))
    {
        const TSharedPtr<FJsonObject>& OceanPiecesObj = JsonObject->GetObjectField(TEXT("ocean_prefill_setpieces"));
        for (auto OceanPieceIt = OceanPiecesObj->Values.CreateIterator(); OceanPieceIt; ++OceanPieceIt)
        {
            const FString& PieceName = OceanPieceIt.Key();
            const TSharedPtr<FJsonValue>& PieceValue = OceanPieceIt.Value();
            
            if (PieceValue.IsValid() && PieceValue->Type == EJson::Number)
            {
                OutTaskSet.OceanPrefillSetPieces.Add(PieceName, PieceValue->AsNumber());
            }
        }
    }

    if (JsonObject->HasField(TEXT("ocean_population")))
    {
        const TArray<TSharedPtr<FJsonValue>>& OceanPopArray = JsonObject->GetArrayField(TEXT("ocean_population"));
        for (const auto& OceanPopValue : OceanPopArray)
        {
            if (OceanPopValue.IsValid() && OceanPopValue->Type == EJson::String)
            {
                OutTaskSet.OceanPopulation.Add(OceanPopValue->AsString());
            }
        }
    }

    return true;
}
