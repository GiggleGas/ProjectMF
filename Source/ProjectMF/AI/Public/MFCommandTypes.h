// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MFCommandTypes.generated.h"

/**
 * 玩家对宠物下达的指令类型。
 * 指令系统 M0：仅定义数据；执行路由在 M1（StateTree 顶层事件打断）。
 */
UENUM(BlueprintType)
enum class EMFCommandType : uint8
{
	None       UMETA(DisplayName = "无"),
	MoveTo     UMETA(DisplayName = "移动到点"),
	CastSkill  UMETA(DisplayName = "释放技能"),
};

/**
 * 一条玩家指令的完整数据。由玩家侧 UMFCommandComponent 构造，
 * 经宠物的 UMFPetCommandComponent::IssueCommand 下达，供 StateTree 执行状态读取。
 */
USTRUCT(BlueprintType)
struct FMFPetCommand
{
	GENERATED_BODY()

	/** 指令类型（决定 StateTree 走哪条执行分支）。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Command")
	EMFCommandType Type = EMFCommandType::None;

	/** 移动目标点（世界坐标）。Type == MoveTo 时有效。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Command")
	FVector TargetLocation = FVector::ZeroVector;

	/** 技能目标 Actor（可空，为空时技能按自身逻辑取目标）。Type == CastSkill 时有效。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Command")
	TWeakObjectPtr<AActor> TargetActor;

	/** 要释放的技能 AbilityTag（如 MF.Ability.Pet.Move.Jump）。Type == CastSkill 时有效。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Command")
	FGameplayTag SkillTag;
};
