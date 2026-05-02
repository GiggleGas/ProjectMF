// d:\Documents\Unreal Projects\ProjectMF\Source\ProjectMF\NMap\Private\KKLayout\KKLayoutGraphActor.cpp
#include "KKLayout/KKLayoutGraphActor.h"
#include "CanvasItem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include <Kismet/KismetRenderingLibrary.h>
#include "TaskSystem/TaskSystemStructures.h"
#include "Engine/Engine.h"
#include "CompGeom/Delaunay2.h"
#include "BoxTypes.h"

AKKLayoutGraphActor::AKKLayoutGraphActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AKKLayoutGraphActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize layout manager
    LayoutManager.SetCanvasSize(CanvasWidth, CanvasHeight);
    
    // Set solver parameters
    FKKParams Params;
    Params.K = K;
    Params.IdealEdgeLength = IdealEdgeLength;
    Params.Tolerance = Tolerance;
    Params.EnergyTolerance = EnergyTolerance;
    Params.MaxIterations = MaxIterations;
    Params.MaxGlobalIterations = MaxGlobalIterations;
    LayoutManager.SetSolverParams(Params);
    
    // Load task system data and generate nodes
    LoadTaskSystemAndGenerateNodes();
    
    // Initialize solver
    LayoutManager.InitializeSolver();
}

void AKKLayoutGraphActor::LoadTaskSystemAndGenerateNodes()
{
    // 创建任务系统数据资产实例
    UTaskSystemDataAsset* TaskSystemData = NewObject<UTaskSystemDataAsset>(this, UTaskSystemDataAsset::StaticClass());
    if (TaskSystemData)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully created UTaskSystemDataAsset instance"));
        
        // 尝试从JSON目录加载数据 - 使用相对于项目的路径
        FString ProjectDir = FPaths::ProjectDir();
        FString JsonDirectoryPath = FPaths::ConvertRelativePathToFull(ProjectDir + TEXT("Source/ProjectMF/NMap/Json"));
        
        if (TaskSystemData->LoadFromJsonDirectory(JsonDirectoryPath))
        {
            UE_LOG(LogTemp, Log, TEXT("Successfully loaded task system data from JSON directory: %s"), *JsonDirectoryPath);
            
            // 输出加载的数据统计
            UE_LOG(LogTemp, Log, TEXT("Loaded %d tasks, %d rooms, %d tasksets"), 
                   TaskSystemData->Tasks.Num(), 
                   TaskSystemData->Rooms.Num(), 
                   TaskSystemData->TaskSets.Num());
            
            // 使用任务系统数据生成节点
            GenerateNodesFromTaskSystem(TaskSystemData);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load task system data from JSON directory: %s"), *JsonDirectoryPath);
            
            // Generate test data as fallback
            GenerateTestData();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UTaskSystemDataAsset instance"));
        
        // Generate test data as fallback
        GenerateTestData();
    }
}

void AKKLayoutGraphActor::GenerateNodesFromTaskSystem(UTaskSystemDataAsset* TaskSystemData)
{
    if (!TaskSystemData)
        return;
    
    // 选取default taskset
    FTaskSetConfig* DefaultTaskSet = TaskSystemData->GetTaskSetById("default");
    if (DefaultTaskSet)
    {
        UE_LOG(LogTemp, Log, TEXT("Found default taskset with %d tasks"), DefaultTaskSet->Tasks.Num());
        
        // Clear existing data
        LayoutManager.Clear();
        
        // Create groups based on tasks in the default taskset
        TArray<TArray<FString>> GroupNodeIds;
        
        // Process each task and create nodes
        CreateNodesFromTaskSet(DefaultTaskSet, TaskSystemData, GroupNodeIds);
        
        // Connect groups based on task dependencies or lock/key relationships
        if (GroupNodeIds.Num() > 1)
        {
            ConnectTaskGroups(LayoutManager, GroupNodeIds, TaskSystemData);
        }

        // Add background room nodes for each task (after creating all task nodes)
        AddBackgroundRoomNodes(DefaultTaskSet, TaskSystemData, GroupNodeIds);
        AddCoveRoomNodes(DefaultTaskSet, TaskSystemData, GroupNodeIds);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Default taskset not found, using test data"));
        // Generate test data as fallback
        GenerateTestData();
    }
}

void AKKLayoutGraphActor::AddBackgroundRoomNodes(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& GroupNodeIds)
{
    if (!TaskSet || !TaskSystemData)
        return;
    
    int bgNodeCount = 0;
    // Iterate through each task in the taskset
    for (int TaskIndex = 0; TaskIndex < TaskSet->Tasks.Num() && TaskIndex < GroupNodeIds.Num(); ++TaskIndex)
    {
        const FString& TaskId = TaskSet->Tasks[TaskIndex];
        FTaskDefinition* Task = TaskSystemData->GetTaskById(TaskId);
        
        if (!Task || Task->BackgroundRoom.IsEmpty())
            continue;
        
        // Check if the background room exists in the room definitions
        FRoomDefinition* BackgroundRoomDef = TaskSystemData->GetRoomByName(Task->BackgroundRoom);
        if (!BackgroundRoomDef)
            continue;
        
        // Get the group for this task
        TArray<FString>& TaskGroupNodes = GroupNodeIds[TaskIndex];
        
        // Store original node count before adding background nodes
        int OriginalNodeCount = TaskGroupNodes.Num();
        // re-allocate memory 
        TArray<FString> BackgroundNodes;

        // Iterate through each original node in this task's group
        for (int NodeIndex = 0; NodeIndex < OriginalNodeCount; ++NodeIndex)
        {
            const FString& OriginalNodeId = TaskGroupNodes[NodeIndex];
            
            // Randomly determine the number of background nodes for this original node (0 to 2)
            int BackgroundNodeCount = FMath::RandRange(0, 2);
            
            if (BackgroundNodeCount > 0)
            {
                UE_LOG(LogTemp, Log, TEXT("Adding %d background nodes for original node: %s (task: %s)"), 
                       BackgroundNodeCount, *OriginalNodeId, *TaskId);
                
                // Generate background nodes for this original node
                for (int i = 0; i < BackgroundNodeCount; ++i)
                {
                    // Create a unique background node id with BG_ prefix
                    FString BackgroundNodeId = FString::Printf(TEXT("BG_%s_%d"), *OriginalNodeId, bgNodeCount);
                    bgNodeCount++;
                    
                    // Generate a random position near the original node
                    FVector2D Position  = GenerateNodePosition(TaskIndex, GroupNodeIds.Num());
                    
                    // Add node with a slightly different color to distinguish it
                    LayoutManager.AddNode(BackgroundNodeId, BackgroundRoomDef->Colour, Position, 1.f); // Lower weight
                    
                    // Add to the task's group nodes
                    BackgroundNodes.Add(BackgroundNodeId);
                    
                    // Connect the background node to the original node
                    LayoutManager.AddEdge(OriginalNodeId, BackgroundNodeId); // Lower edge weight
                    
                    UE_LOG(LogTemp, Log, TEXT("Added background node: %s connected to: %s"), *BackgroundNodeId, *OriginalNodeId);
                }
            }
        }
        TaskGroupNodes.Append(BackgroundNodes);
    }
}

void AKKLayoutGraphActor::AddCoveRoomNodes(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& GroupNodeIds)
{
    if (!TaskSet || !TaskSystemData)
        return;
    
    // Iterate through each task in the taskset
    for (int TaskIndex = 0; TaskIndex < TaskSet->Tasks.Num() && TaskIndex < GroupNodeIds.Num(); ++TaskIndex)
    {
        const FString& TaskId = TaskSet->Tasks[TaskIndex];
        FTaskDefinition* Task = TaskSystemData->GetTaskById(TaskId);
        
        if (!Task)
            continue;
        
        // Get cove room name from task definition (default to "Blank" if not specified)
        FString CoveRoomName = Task->CoveRoomName.IsEmpty() ? TEXT("Blank") : Task->CoveRoomName;
        
        // Check if the cove room exists in the room definitions
        FRoomDefinition* CoveRoomDef = TaskSystemData->GetRoomByName(CoveRoomName);
        if (!CoveRoomDef)
            continue;
        
        // Get the group for this task
        TArray<FString>& TaskGroupNodes = GroupNodeIds[TaskIndex];
        
        // Store current node count before adding cove nodes
        int CurrentNodeCount = TaskGroupNodes.Num();
        
        // Iterate through each node in this task's group
        for (int NodeIndex = 0; NodeIndex < CurrentNodeCount; ++NodeIndex)
        {
            const FString& OriginalNodeId = TaskGroupNodes[NodeIndex];
            
            // Check the number of edges this node has
            int EdgeCount = LayoutManager.GetGraph()->GetEdgeNumOfNodeById(OriginalNodeId);
            
            // Only consider nodes with at most 1 edge
            if (EdgeCount <= 1)
            {
                // 35% chance to add a cove node
                if (FMath::FRand() <= 0.35f)
                {
                    UE_LOG(LogTemp, Log, TEXT("Adding cove node for node: %s (task: %s, edge count: %d)"), 
                           *OriginalNodeId, *TaskId, EdgeCount);
                    
                    // Create a unique cove node id with COVE_ prefix
                    FString CoveNodeId = FString::Printf(TEXT("COVE_%s"), *OriginalNodeId);
                    
                    // Generate a random position near the original node
                    FVector2D Position = GenerateNodePosition(TaskIndex, TaskSet->Tasks.Num());
                    
                    // Add node with a distinct color for cove nodes
                    LayoutManager.AddNode(CoveNodeId, CoveRoomDef->Colour, Position, 1.f); // Lower weight
                    
                    // Connect the cove node to the original node
                    LayoutManager.AddEdge(OriginalNodeId, CoveNodeId);

                    //  avoid OriginalNodeId reference issue
                    // Add to the task's group nodes
                    TaskGroupNodes.Add(CoveNodeId);
                    UE_LOG(LogTemp, Log, TEXT("Added cove node: %s connected to: %s"), *CoveNodeId, *OriginalNodeId);
                }
            }
        }
    }
}

void AKKLayoutGraphActor::CreateNodesFromTaskSet(FTaskSetConfig* TaskSet, UTaskSystemDataAsset* TaskSystemData, TArray<TArray<FString>>& OutGroupNodeIds)
{
    if (!TaskSet || !TaskSystemData)
        return;
    
    OutGroupNodeIds.Reserve(TaskSet->Tasks.Num());
    
    int GroupIndex = 0;
    
    // Process each task in the taskset
    for (const FString& TaskId : TaskSet->Tasks)
    {
        TArray<FString> NodeIds;
        
        FTaskDefinition* Task = TaskSystemData->GetTaskById(TaskId);
        if (Task)
        {
            UE_LOG(LogTemp, Log, TEXT("Processing task: %s"), *TaskId);
            
            // Create nodes for this task
            CreateNodesForTask(Task, TaskSystemData, GroupIndex, TaskSet->Tasks.Num(), NodeIds);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find task: %s"), *TaskId);
        }
        
        OutGroupNodeIds.Add(NodeIds);
        GroupIndex++;
    }
}
void AKKLayoutGraphActor::CreateNodesForTask(FTaskDefinition* Task, UTaskSystemDataAsset* TaskSystemData, int GroupIndex, int TotalGroups, TArray<FString>& OutNodeIds)
{
    if (!Task || !TaskSystemData)
        return;
    
    // Track the hub room node id (only record the first match)
    FString HubNodeId;
    int Index = 0;
    // Create nodes for this task based on its room choices
    for (const auto& RoomChoicePair : Task->RoomChoices)
    {
        FString RoomName = RoomChoicePair.Key;
        int32 RoomCount = static_cast<int32>(RoomChoicePair.Value); // Ensure at least 1
        
        // Find corresponding room definition
        FRoomDefinition* RoomDef = TaskSystemData->GetRoomByName(RoomName);
        if (RoomDef)
        {
            // Create multiple nodes for this room based on the count
            for (int i = 0; i < RoomCount; ++i)
            {
                // Create a unique node id with index to prevent conflicts
                FString NodeId = FString::Printf(TEXT("%s_%s_%d"), *Task->TaskId, *RoomName, Index);
                Index++;
                
                // Generate position for this node
                FVector2D Position = GenerateNodePosition(GroupIndex, TotalGroups);
                
                // Add node with color from task
                LayoutManager.AddNode(NodeId, Task->Colour, Position, 1.0f);
                OutNodeIds.Add(NodeId);
                
                // Check if this room is the HubRoom (only record the first match)
                if (HubNodeId.IsEmpty() && !Task->HubRoom.IsEmpty() && RoomName == Task->HubRoom)
                {
                    HubNodeId = NodeId;
                    UE_LOG(LogTemp, Log, TEXT("Found HubRoom node: %s"), *HubNodeId);
                }
                
                UE_LOG(LogTemp, Log, TEXT("Added node: %s"), *NodeId);
            }
        }
    }
    
    // Create nodes for special room choices as well
    for (const auto& SpecialRoomChoicePair : Task->RoomChoicesSpecial)
    {
        FString RoomName = SpecialRoomChoicePair.Key;
        int32 RoomCount = static_cast<int32>(SpecialRoomChoicePair.Value); // Ensure at least 1
        
        // Find corresponding room definition
        FRoomDefinition* RoomDef = TaskSystemData->GetRoomByName(RoomName);
        if (RoomDef)
        {
            // Create multiple nodes for this special room based on the count
            for (int i = 0; i < RoomCount; ++i)
            {
                // Create a unique node id with index to prevent conflicts
                FString NodeId = FString::Printf(TEXT("%s_%s_Special_%d"), *Task->TaskId, *RoomName, Index);
                Index++;
                
                // Generate position for this node
                FVector2D Position = GenerateNodePosition(GroupIndex, TotalGroups);
                
                // Add node with color from task
                LayoutManager.AddNode(NodeId, Task->Colour, Position, 1.0f);
                OutNodeIds.Add(NodeId);
                
                // Check if this room is the HubRoom (only record the first match)
                if (HubNodeId.IsEmpty() && !Task->HubRoom.IsEmpty() && RoomName == Task->HubRoom)
                {
                    HubNodeId = NodeId;
                    UE_LOG(LogTemp, Log, TEXT("Found HubRoom node in special rooms: %s"), *HubNodeId);
                }
                
                UE_LOG(LogTemp, Log, TEXT("Added special room node: %s"), *NodeId);
            }
        }
    }
    
    // If no rooms were added for this task, add at least one node representing the task
    if (OutNodeIds.Num() == 0)
    {
        FString NodeId = FString::Printf(TEXT("TASK_%s"), *Task->TaskId);
        
        // Generate position for this node
        FVector2D Position = GenerateNodePosition(GroupIndex, TotalGroups);
        
        LayoutManager.AddNode(NodeId, Task->Colour, Position, 1.0f);
        OutNodeIds.Add(NodeId);
        
        UE_LOG(LogTemp, Log, TEXT("Added task-only node: %s"), *NodeId);
    }
    
    // Now create edges within this task based on the determined hub node
    CreateEdgesForTask(Task, OutNodeIds, HubNodeId);
}
void AKKLayoutGraphActor::CreateEdgesForTask(FTaskDefinition* Task, const TArray<FString>& NodeIds, const FString& HubNodeId)
{
    if (!Task || NodeIds.Num() < 2)
        return;
    
    // Determine edge creation strategy based on task properties
    if (!HubNodeId.IsEmpty())
    {
        // HubRoom was found, create star edges with HubRoom as center
        CreateStarEdges(LayoutManager, NodeIds, HubNodeId);
        UE_LOG(LogTemp, Log, TEXT("Created star edges with hub: %s for task: %s"), *HubNodeId, *Task->TaskId);
    }
    else if (Task->bMakeLoop)
    {
        // No HubRoom but bMakeLoop is true, create cycle edges
        CreateCycleEdges(LayoutManager, NodeIds);
        UE_LOG(LogTemp, Log, TEXT("Created cycle edges for task: %s"), *Task->TaskId);
    }
    else
    {
        // Neither HubRoom nor bMakeLoop, create chain edges
        CreateChainEdges(LayoutManager, NodeIds);
        UE_LOG(LogTemp, Log, TEXT("Created chain edges for task: %s"), *Task->TaskId);
    }
}

void AKKLayoutGraphActor::CreateChainEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds)
{
    if (NodeIds.Num() < 2)
        return;
    
    // Create edges in a chain: Node[0] -> Node[1] -> Node[2] -> ... -> Node[N-1]
    for (int i = 0; i < NodeIds.Num() - 1; ++i)
    {
        InManager.AddEdge(NodeIds[i], NodeIds[i + 1]);
    }
}

void AKKLayoutGraphActor::CreateStarEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, const FString& CenterNodeId)
{
    if (NodeIds.Num() < 2 || CenterNodeId.IsEmpty())
        return;
    
    // Connect all other nodes to the center node
    for (const FString& NodeId : NodeIds)
    {
        if (NodeId != CenterNodeId)
        {
            InManager.AddEdge(CenterNodeId, NodeId);
        }
    }
}

FVector2D AKKLayoutGraphActor::GenerateNodePosition(int GroupIndex, int TotalGroups)
{
    float OffsetX = (CanvasWidth / TotalGroups) * GroupIndex + FMath::FRandRange(-50.f, 50.f);
    float OffsetY = CanvasHeight / 2 + FMath::FRandRange(-100.f, 100.f);
    
    return FVector2D(
        FMath::Clamp(OffsetX + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasWidth),
        FMath::Clamp(OffsetY + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasHeight)
    );
}

void AKKLayoutGraphActor::CreateEdgesWithinGroups(TArray<TArray<FString>>& GroupNodeIds)
{
    // Create edges within each group based on room connections in the task
    for (int g = 0; g < GroupNodeIds.Num(); ++g)
    {
        TArray<FString>& GroupNodes = GroupNodeIds[g];
        
        // Use different edge creation strategy based on task characteristics
        if (GroupNodes.Num() >= 2)
        {
            // Create cycle edges for this group
            CreateCycleEdges(LayoutManager, GroupNodes);
        }
    }
}

void AKKLayoutGraphActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bUpdateLayout)
    {
        // Update layout
        bool Converged = LayoutManager.UpdateLayout(DeltaTime);
        
        if (Converged)
        {
            bUpdateLayout = false;
        }
    }
    
    // Draw to render target if available
    if (RenderTarget)
    {
        DrawToRenderTarget();
    }
}

void AKKLayoutGraphActor::GenerateTestData()
{
    CreateTestData();
}

void AKKLayoutGraphActor::DrawToRenderTarget()
{
    if (!RenderTarget)
        return;
    
    // clear
    UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget);

    FDrawToRenderTargetContext Context;
    UCanvas* Canvas;
    FVector2D Size;

    // ��ȡCanvas����Ⱦ������
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
    if (!Canvas) return;

    // Draw the graph
    DrawGraph(Canvas);

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

void AKKLayoutGraphActor::SolveLayout()
{
    LayoutManager.SolveLayout();
}

void AKKLayoutGraphActor::CreateTestData()
{
    // Clear existing data
    LayoutManager.Clear();
    
    // Calculate nodes per group
    const int NodesPerGroup = FMath::Max(1, NodeCount / GroupCount);
    
    // Store node IDs for each group
    TArray<TArray<FString>> GroupNodeIds;
    GroupNodeIds.Reserve(GroupCount);
    
    // Create groups
    for (int g = 0; g < GroupCount; ++g)
    {
        TArray<FString> NodeIds;
        NodeIds.Reserve(NodesPerGroup);
        
        // Create nodes for this group
        for (int i = 0; i < NodesPerGroup; ++i)
        {
            FString NodeId = FString::Printf(TEXT("Node_%d_%d"), g, i);
            
            // Generate random position within a region for this group
            float OffsetX = (CanvasWidth / GroupCount) * g + FMath::FRandRange(-50.f, 50.f);
            float OffsetY = CanvasHeight / 2 + FMath::FRandRange(-100.f, 100.f);
            
            FVector2D Position(
                FMath::Clamp(OffsetX + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasWidth),
                FMath::Clamp(OffsetY + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasHeight)
            );
            
            LayoutManager.AddNode(NodeId,FLinearColor::White, Position, 1.0f);
            NodeIds.Add(NodeId);
        }
        
        // Randomly choose an edge generation function for this group
        int EdgeFunction = FMath::RandRange(0, 1);
        switch (EdgeFunction)
        {
        case 0:
            // Create cycle edges
            CreateCycleEdges(LayoutManager, NodeIds);
            break;
        case 1:
            // Create star edges
            CreateStarEdges(LayoutManager, NodeIds);
            break;
        }
        
        GroupNodeIds.Add(NodeIds);
    }
    
    // Add remaining nodes to last group
    int RemainingNodes = NodeCount - (GroupCount * NodesPerGroup);
    if (RemainingNodes > 0)
    {
        TArray<FString>& LastGroup = GroupNodeIds.Last();
        for (int i = NodesPerGroup; i < NodesPerGroup + RemainingNodes; ++i)
        {
            FString NodeId = FString::Printf(TEXT("Node_%d_%d"), GroupCount - 1, i);
            
            float OffsetX = (CanvasWidth / GroupCount) * (GroupCount - 1) + FMath::FRandRange(-50.f, 50.f);
            float OffsetY = CanvasHeight / 2 + FMath::FRandRange(-100.f, 100.f);
            
            FVector2D Position(
                FMath::Clamp(OffsetX + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasWidth),
                FMath::Clamp(OffsetY + FMath::FRandRange(-80.f, 80.f), 0.0f, CanvasHeight)
            );
            
            LayoutManager.AddNode(NodeId, FLinearColor::White, Position, 1.0f);
            LastGroup.Add(NodeId);
        }
        
        // Add edges for remaining nodes
        int EdgeFunction = FMath::RandRange(0, 1);
        switch (EdgeFunction)
        {
        case 0:
            CreateCycleEdges(LayoutManager, LastGroup);
            break;
        case 1:
            CreateStarEdges(LayoutManager, LastGroup);
            break;
        }
    }
    
    // Connect groups using tree structure
    if (GroupCount > 1)
    {
        ConnectGroups(LayoutManager, GroupNodeIds, NodesPerGroup);
    }
}

void AKKLayoutGraphActor::ConnectTaskGroups(FKKLayoutManager& InManager, const TArray<TArray<FString>>& GroupNodeIds, UTaskSystemDataAsset* TaskSystemData)
{
    const int GraphCount = GroupNodeIds.Num();
    
    if (GraphCount <= 1)
        return;
    
    // Create set A (unconnected groups) and set B (connected groups)
    TArray<int> SetA; // Unconnected groups
    TArray<int> SetB; // Connected groups
    
    // Initialize set A with all group indices
    for (int i = 0; i < GraphCount; i++)
    {
        SetA.Add(i);
    }
    
    // Randomly select an initial group for set B
    int InitialGroupIndex = FMath::RandRange(0, SetA.Num() - 1);
    SetB.Add(SetA[InitialGroupIndex]);
    SetA.RemoveAt(InitialGroupIndex);
    
    UE_LOG(LogTemp, Log, TEXT("Connecting %d task groups"), GraphCount);
    
    // Connect remaining groups in set A to set B
    while (SetA.Num() > 0)
    {
        // Randomly select a group from set A
        int AIndex = FMath::RandRange(0, SetA.Num() - 1);
        int GroupA = SetA[AIndex];
        
        // Randomly select a group from set B
        int BIndex = FMath::RandRange(0, SetB.Num() - 1);
        int GroupB = SetB[BIndex];
        
        // Select random nodes from each group
        const TArray<FString>& GroupANodes = GroupNodeIds[GroupA];
        const TArray<FString>& GroupBNodes = GroupNodeIds[GroupB];
        
        // Skip if either group has no nodes
        if (GroupANodes.Num() == 0 || GroupBNodes.Num() == 0)
        {
            SetB.Add(GroupA);
            SetA.RemoveAt(AIndex);
            continue;
        }
        
        int NodeAIndex = FMath::RandRange(0, GroupANodes.Num() - 1);
        int NodeBIndex = FMath::RandRange(0, GroupBNodes.Num() - 1);
        
        FString NodeAId = GroupANodes[NodeAIndex];
        FString NodeBId = GroupBNodes[NodeBIndex];
        
        InManager.AddEdge(NodeAId, NodeBId);
        
        UE_LOG(LogTemp, Log, TEXT("Connected task group %d to group %d: %s <-> %s"), 
               GroupA, GroupB, *NodeAId, *NodeBId);
        
        // Move the selected group from set A to set B
        SetB.Add(GroupA);
        SetA.RemoveAt(AIndex);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Finished connecting task groups"));
}

void AKKLayoutGraphActor::ConnectGroups(FKKLayoutManager& InManager, const TArray<TArray<FString>>& GroupNodeIds, int NodesPerGroup)
{
    const int GraphCount = GroupNodeIds.Num();
    
    // Create set A (unconnected groups) and set B (connected groups)
    TArray<int> SetA; // Unconnected groups
    TArray<int> SetB; // Connected groups
    
    // Initialize set A with all group indices
    for (int i = 0; i < GraphCount; i++)
    {
        SetA.Add(i);
    }
    
    // Randomly select an initial group for set B
    int InitialGroupIndex = FMath::RandRange(0, SetA.Num() - 1);
    SetB.Add(SetA[InitialGroupIndex]);
    SetA.RemoveAt(InitialGroupIndex);
    
    // Connect remaining groups in set A to set B
    while (SetA.Num() > 0)
    {
        // Randomly select a group from set A
        int AIndex = FMath::RandRange(0, SetA.Num() - 1);
        int GroupA = SetA[AIndex];
        
        // Randomly select a group from set B
        int BIndex = FMath::RandRange(0, SetB.Num() - 1);
        int GroupB = SetB[BIndex];
        
        // Select random nodes from each group
        const TArray<FString>& GroupANodes = GroupNodeIds[GroupA];
        const TArray<FString>& GroupBNodes = GroupNodeIds[GroupB];
        
        int NodeAIndex = FMath::RandRange(0, GroupANodes.Num() - 1);
        int NodeBIndex = FMath::RandRange(0, GroupBNodes.Num() - 1);
        
        FString NodeAId = GroupANodes[NodeAIndex];
        FString NodeBId = GroupBNodes[NodeBIndex];
        
        InManager.AddEdge(NodeAId, NodeBId);
        
        // Move the selected group from set A to set B
        SetB.Add(GroupA);
        SetA.RemoveAt(AIndex);
    }
}

void AKKLayoutGraphActor::CreateRandomEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, int EdgeCount)
{
    int CreatedEdges = 0;
    int MaxAttempts = EdgeCount * 10;
    
    for (int i = 0; i < MaxAttempts && CreatedEdges < EdgeCount; ++i)
    {
        int Index1 = FMath::RandRange(0, NodeIds.Num() - 1);
        int Index2 = FMath::RandRange(0, NodeIds.Num() - 1);
        
        if (Index1 != Index2)
        {
            if (InManager.AddEdge(NodeIds[Index1], NodeIds[Index2]))
            {
                CreatedEdges++;
            }
        }
    }
}

void AKKLayoutGraphActor::CreateCycleEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds)
{
    for (int i = 0; i < NodeIds.Num(); ++i)
    {
        int NextIndex = (i + 1) % NodeIds.Num();
        InManager.AddEdge(NodeIds[i], NodeIds[NextIndex]);
    }
}

void AKKLayoutGraphActor::CreateStarEdges(FKKLayoutManager& InManager, const TArray<FString>& NodeIds, int CenterNodeIndex)
{
    if (CenterNodeIndex < 0 || CenterNodeIndex >= NodeIds.Num())
        return;
    
    FString CenterNodeId = NodeIds[CenterNodeIndex];
    
    for (int i = 0; i < NodeIds.Num(); ++i)
    {
        if (i != CenterNodeIndex)
        {
            InManager.AddEdge(CenterNodeId, NodeIds[i]);
        }
    }
}

void AKKLayoutGraphActor::DrawGraph(UCanvas* Canvas)
{
    if (!Canvas)
        return;
    
    FKKGraph* Graph = LayoutManager.GetGraph();
    if (!Graph)
        return;
    // Draw edges
    Graph->ForEachKKEdge([&](FKKGraph*, FKKNode& Source, FKKNode& Target, float) {
        Canvas->K2_DrawLine(Source.Position, Target.Position, 1.0f, Source.Color);
    });
    
    FLinearColor NodeColor = { 0.0f, 0.7f, 1.0f, 1.0f };
    // Draw nodes
    Graph->ForEachKKNode([&](FKKGraph*, FKKNode& Node) {
        FVector2D Position(Node.Position.X * Scale, Node.Position.Y * Scale);
        
        float Radius = 10.0f * Node.Weight ;
        Canvas->K2_DrawBox(Node.Position - Radius / 2, FVector2D{ Radius }, Radius / 2, Node.Color);
        
        // Draw node index
        Canvas->K2_DrawText(GEngine->GetSmallFont(), Node.Id, Node.Position, FVector2D{ 1.f }, Node.Color);
    });
}




// Voronoi diagram

void AKKLayoutGraphActor::BakeAndDrawVoronoiDiagram()
{
    CreateVNGraphFromLayoutManager();
    DrawVoronoiDiagram();
}


void AKKLayoutGraphActor::CreateVNGraphFromLayoutManager()
{
    // Get the graph from LayoutManager
    FKKGraph* Graph = LayoutManager.GetGraph();
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateVNGraphFromLayoutManager: No graph found in LayoutManager"));
        return;
    }

    int NodeCount_ = Graph->GetNodeNum();
    if (NodeCount_ <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateVNGraphFromLayoutManager: No nodes in graph"));
        return;
    }

    // Step 1: Collect all node positions from LayoutManager
    TArray<FVector2D> Points;
    Points.Reserve(NodeCount);

    Graph->ForEachKKNode([&](FKKGraph*, FKKNode& Node) {
        Points.Add(FVector2D(Node.Position.X, Node.Position.Y));
        return true;
    });

    UE_LOG(LogTemp, Log, TEXT("CreateVNGraphFromLayoutManager: Collected %d node positions"), Points.Num());

    // Step 2: Generate Voronoi corners using Delaunay triangulation
    float MapWidth = CanvasWidth;
    float MapHeight = CanvasHeight;
    
    TArray<TArray<FVector2D>> Corners = GenerateCornerPoints(Points, MapWidth, MapHeight, 0);

    UE_LOG(LogTemp, Log, TEXT("CreateVNGraphFromLayoutManager: Generated %d corner sets"), Corners.Num());

    // Step 3: Build VNGraph from points and corners
    VNGraph = FVNGraph::BuildFromPointsAndCorners(Points, Corners);

    if (VNGraph.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("CreateVNGraphFromLayoutManager: Successfully created VNGraph with %d cells, %d corners, %d edges"),
               VNGraph->Cells.Num(), VNGraph->Corners.Num(), VNGraph->Edges.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CreateVNGraphFromLayoutManager: Failed to create VNGraph"));
    }
}

TArray<TArray<FVector2D>> AKKLayoutGraphActor::GenerateCornerPoints(TArray<FVector2D>& InOutPoints, float Width, float Height, int LloydRelaxations)
{
    using namespace UE::Geometry;
    FAxisAlignedBox2d Box = { FVector2D(0), FVector2D(Width, Height) };
    FDelaunay2 Delaunay;
    bool bSuccess = Delaunay.Triangulate(InOutPoints);
    
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateCornerPoints: Delaunay triangulation failed"));
        return TArray<TArray<FVector2D>>();
    }

    TArray<TArray<FVector2D>> OutCornerPoints = Delaunay.GetVoronoiCells(InOutPoints, true, Box);

    // Apply Lloyd relaxation if requested
    for (int32 i = 0; i < LloydRelaxations; ++i)
    {
        // Implementation of Lloyd relaxation would go here
        // For simplicity, we'll skip it for now
    }

    return OutCornerPoints;
}

void AKKLayoutGraphActor::DrawVoronoiDiagram()
{
    if (!VNGraph.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("DrawVoronoiDiagram: No VNGraph to draw"));
        return;
    }

    if(!RTBioronoi)
    {
        UE_LOG(LogTemp, Error, TEXT("DrawVoronoiDiagram: No RTBioronoi"));
        return;
    }

	{
		FDrawToRenderTargetContext Context;
		UCanvas* Canvas;
		FVector2D Size;
		
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RTBioronoi, Canvas, Size, Context);
		if (!Canvas) return;
		

        LayoutManager.GetGraph()->ForEachKKNode([&](FKKGraph*, FKKNode& Node) {
            const FVNCell& cen = VNGraph->Cells[Node.Index];
            FCanvasUVTri tri{};
			tri.V0_Pos = cen.Position;
			tri.V0_Color = tri.V1_Color = tri.V2_Color = Node.Color;

			TArray<FCanvasUVTri> CanvasUVTri;

			for (int k = 0; k < cen.Corners.Num(); ++k)
			{
				int c1 = cen.Corners[k];
				int c2 = cen.Corners[(k + 1) % cen.Corners.Num()];

				tri.V1_Pos = VNGraph->Corners[c1].Position;
				tri.V2_Pos = VNGraph->Corners[c2].Position;
				CanvasUVTri.Add(tri);

			}
			Canvas->K2_DrawTriangle(nullptr, CanvasUVTri);
            Canvas->K2_DrawText(GEngine->GetSmallFont(), Node.Id, cen.Position, FVector2D{ 1.f }, Node.Color);
        });

        for (auto& e :VNGraph-> Edges)
        {
            Canvas->K2_DrawLine(VNGraph->Corners[e.StartCornerId].Position, VNGraph->Corners[e.EndCornerId].Position, 1, FLinearColor::Black);
        }
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
	}
}
