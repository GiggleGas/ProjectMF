// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MFCommandComponent.generated.h"

class AActor;

/**
 * UMFCommandComponent — 玩家侧的指令组件，挂在 AMFPlayerController 上。
 *
 * 职责（最终）：管理林克时间(M4)、宠物选中态、把鼠标输入翻译成 FMFPetCommand 下达给宠物(M3)。
 *
 * M0 阶段：仅骨架 + 两个调试入口，用于验证「IssueCommand → 存数据 → 发 StateTree 事件」链路，
 * 以及给 M1 验证「打断 → 路由 → 回落」提供下令手段。
 */
UCLASS(ClassGroup = (MF), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UMFCommandComponent();

	/** 调试：给指定宠物下达移动到点的指令（精确控制，供 BP / 调试 widget 调用）。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugIssuePetMove(AActor* Pet, const FVector& Location);

	/** 调试：给指定宠物下达释放技能指令（SkillTag 为该技能的 AbilityTag，目标由技能自取）。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugIssuePetSkill(AActor* Pet, FGameplayTag SkillTag);

	/**
	 * 让离玩家最近的宠物移动到 (X,Y,Z)。由 AMFPlayerController 的同名 Exec 命令转调。
	 * （Exec 放在 PlayerController 上，组件 exec 不保证被控制台路由命中。）
	 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugMoveNearestPet(float X, float Y, float Z);

	/** 让离玩家最近的宠物释放指定 tag 的技能。由 AMFPlayerController 的 Exec 命令转调。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugSkillNearestPet(const FString& SkillTag);

private:
	/** 返回离玩家最近的宠物（AMFPetBase），无则 nullptr。调试命令共用。 */
	class AMFPetBase* FindNearestPet() const;
};
