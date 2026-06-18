// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFCommandTypes.h"
#include "MFPetCommandComponent.generated.h"

/**
 * UMFPetCommandComponent — 宠物侧的玩家指令载体。
 *
 * 挂在可被指挥的宠物（AMFPetBase）上。玩家侧（UMFCommandComponent）调用 IssueCommand
 * 存入待执行指令，并向本宠 StateTree 发 MF.AI.Event.PlayerCommand 事件触发打断。
 * StateTree 的执行状态（M1）通过 HasCommand / GetCommand 读取指令，执行完调 ConsumeCommand。
 *
 * M0 阶段：数据载体 + 发事件管线就位；事件在 M1 给 StateTree 加顶层转换后才被消费。
 */
UCLASS(ClassGroup = (MF), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFPetCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UMFPetCommandComponent();

	/** 存入指令并向本宠 StateTree 发 Event_PlayerCommand。控制器/StateTree 缺失时只存数据。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command")
	void IssueCommand(const FMFPetCommand& Command);

	/** 是否有待执行指令。 */
	UFUNCTION(BlueprintPure, Category = "MF|Command")
	bool HasCommand() const { return bHasCommand; }

	/** 读取当前待执行指令（配合 HasCommand 使用）。 */
	const FMFPetCommand& GetCommand() const { return PendingCommand; }

	/** 指令执行完毕后清空待执行标记（由 StateTree 执行状态在 M1 调用）。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command")
	void ConsumeCommand();

private:

	UPROPERTY()
	FMFPetCommand PendingCommand;

	bool bHasCommand = false;
};
