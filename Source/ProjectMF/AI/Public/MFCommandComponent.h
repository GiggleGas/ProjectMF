// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MFCommandComponent.generated.h"

class AActor;
class AMFPetBase;
class APlayerController;

/** 命令模式开关变化（true=进入）。HUD / BP 绑此驱动后处理（去色/暗角）+ 提示，表现不写死在 C++。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMFOnCommandModeChanged, bool, bActive);

/**
 * UMFCommandComponent — 玩家侧指令组件，挂在 AMFPlayerController 上。
 *
 * 命令模式（M3 引入这个壳；M4 在进入/退出处叠加林克时间 SetGlobalTimeDilation + 视觉）：
 *   - 进入：显示鼠标光标，开始响应点选 / 拖拽 / 双击；退出：恢复光标、清选中。
 *   - 点选召唤宠物 → 高亮；按住拖动 → 抬手处下达移动指令；双击 → 释放该宠"已转好的手动技能"。
 * 指令经宠物的 UMFPetCommandComponent::IssueCommand 下达，由 StateTree 打断执行（M1）。
 */
UCLASS(ClassGroup = (MF), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UMFCommandComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 命令模式开关变化时广播（true=进入）。供 HUD/BP 驱动后处理与提示。 */
	UPROPERTY(BlueprintAssignable, Category = "MF|Command")
	FMFOnCommandModeChanged OnCommandModeChanged;

	// -----------------------------------------------------------------------
	// 命令模式（绑定到输入）
	// -----------------------------------------------------------------------

	/** 切换命令模式（绑定 CommandModeAction.Started）。 */
	void ToggleCommandMode();
	void EnterCommandMode();
	void ExitCommandMode();

	UFUNCTION(BlueprintPure, Category = "MF|Command")
	bool IsCommandModeActive() const { return bCommandModeActive; }

	// -----------------------------------------------------------------------
	// 点击处理（绑定 CommandClickAction 的 Started / Completed）
	// -----------------------------------------------------------------------

	/** 左键按下：点选 / 双击放技能。 */
	void OnCommandClickStarted();
	/** 左键抬起：拖拽到地面点 → 移动指令。 */
	void OnCommandClickCompleted();

	// -----------------------------------------------------------------------
	// 调试入口（控制台 Exec 在 AMFPlayerController 上转调）
	// -----------------------------------------------------------------------

	/** 调试：给指定宠物下移动指令。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugIssuePetMove(AActor* Pet, const FVector& Location);

	/** 调试：给指定宠物下释放技能指令。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugIssuePetSkill(AActor* Pet, FGameplayTag SkillTag);

	/** 让最近的宠物移动到 (X,Y,Z)。由 PC 的 Exec 命令转调。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugMoveNearestPet(float X, float Y, float Z);

	/** 让最近的宠物释放指定 tag 的技能。由 PC 的 Exec 命令转调。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Command|Debug")
	void DebugSkillNearestPet(const FString& SkillTag);

private:

	// 拾取 / 指令
	AMFPetBase* FindNearestPet() const;
	AMFPetBase* GetPetUnderCursor() const;            // 光标下的召唤宠物（Pet_Summoned）
	bool        GetGroundPointUnderCursor(FVector& OutLocation) const;
	void        SelectPet(AMFPetBase* Pet);
	void        ClearSelection();
	void        IssuePetMove(AMFPetBase* Pet, const FVector& Location);
	void        IssuePetSkill(AMFPetBase* Pet, FGameplayTag SkillTag);
	void        IssuePetReadyManualSkill(AMFPetBase* Pet);

	APlayerController* GetPC() const;

	/** 从 PlayerConfig 读命令模式时间倍率（配置只在 Config，不在蓝图）；取不到用 0.2 兜底。 */
	float GetCommandModeDilation() const;

	/** 命令模式后处理：开启=玩家相机去色 + 暗角；关闭=清除。代码驱动，不依赖 PP 材质/蓝图。 */
	void ApplyCommandModePostProcess(bool bEnable);

	// 状态
	bool bCommandModeActive   = false;
	bool bSavedShowMouseCursor = false;

	TWeakObjectPtr<AMFPetBase> SelectedPet;

	// 点击 / 拖拽 / 双击判定
	FVector2D                  PressScreenPos = FVector2D::ZeroVector;
	bool                       bPressedOnPet  = false;
	double                     LastClickRealTime = 0.0;   // 用真实时间，林克时间膨胀下双击仍准
	TWeakObjectPtr<AMFPetBase> LastClickedPet;

	static constexpr float DoubleClickThreshold = 0.3f;   // 秒
	static constexpr float DragThreshold        = 10.f;   // 屏幕像素
};
