// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MFAttributeInitData.h"
#include "MFPlayerConfig.generated.h"

class UInputMappingContext;
class UInputAction;
class UMFGameplayAbilityBase;
class UGameplayEffect;
class UMFMainHUDWidget;
class UMFItemDatabase;
class UDataTable;

/**
 * UMFPlayerConfig — 玩家专属配置（DataAsset）。
 *
 * 汇总所有需要在 Blueprint 中配置的玩家属性：输入绑定、UI 类、GAS 技能与属性初始化、战斗参数。
 * 在 BP_MFCharacter 和 BP_PlayerController 的 Details 面板中赋值同一个资产实例即可。
 *
 * 设计原则：基类 AMFCharacterBase 的属性保持不变以支持 AI 角色配置；
 * AMFCharacter 在 BeginPlay 前将 Config 值复制到基类属性，后续流程无感知。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFPlayerConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// Input — 输入映射
	// -----------------------------------------------------------------------

	/** Enhanced Input 的映射上下文资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** 映射上下文优先级（数值越大越优先）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;

	/** 移动输入（2D 轴）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** 拾取输入（长按 / 松开）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PickAction;

	/** 摄像机旋转（1D 轴，+1 = 顺时针，-1 = 逆时针）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RotateCameraAction;

	/** 抓宠键（松开触发）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CatchPetAction;

	/**
	 * 召唤宠物 slot 1-5 的临时按键。
	 * 元素 0 = slot 1，元素 4 = slot 5。
	 * GA_PetWheel 实现后可整体移除。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Demo")
	TArray<TObjectPtr<UInputAction>> SummonSlotActions;

	/** 手动触发 Boss 战（捕宠阶段且宠物数量足够时生效）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> StartBossBattleAction;

	/** 切换命令模式（指令系统）。M3 仅切换交互；M4 叠加林克时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CommandModeAction;

	/** 命令模式内的左键：点选 / 拖拽移动 / 双击放技能。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CommandClickAction;

	/** 抱起/移动宠物（GA_CarryPet）。切换式：按一次抱起、再按一次放下。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CarryPetAction;

	// -----------------------------------------------------------------------
	// 指令系统 / 林克时间
	// -----------------------------------------------------------------------

	/** 命令模式（林克时间）下的全局时间倍率：越小越慢。经 UMFTimeControlSubsystem 应用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float CommandModeTimeDilation = 0.2f;

	/** 命令模式倒计时时长（秒，真实时间）。到点自动退出并进入冷却。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command", meta = (ClampMin = "0.1"))
	float CommandModeDuration = 5.f;

	/** 命令模式冷却时长（秒）。退出后这段时间内无法再次进入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command", meta = (ClampMin = "0.0"))
	float CommandModeCooldown = 8.f;

	// -----------------------------------------------------------------------
	// 抱起/移动宠物（GA_CarryPet）
	// -----------------------------------------------------------------------

	/** 抱宠时玩家移速倍率（负重减速）。0.5 = 半速。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CarryPet",
		meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float CarrySpeedMultiplier = 0.5f;

	/** 可抱起宠物的最大距离（cm）。超出则抱不到。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CarryPet", meta = (ClampMin = "0.0"))
	float CarryReach = 200.f;

	/** 抱起时宠物相对玩家的挂载偏移（本地空间，默认头顶上方）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CarryPet")
	FVector CarryHoldOffset = FVector(0.f, 0.f, 80.f);

	// -----------------------------------------------------------------------
	// UI — 界面配置
	// -----------------------------------------------------------------------

	/**
	 * 主 HUD Widget 类（必须继承 UMFMainHUDWidget）。
	 * 由 AMFPlayerController::BeginPlay 创建并 AddToViewport。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMFMainHUDWidget> MainHUDClass;

	// -----------------------------------------------------------------------
	// GAS — 技能系统初始化
	// -----------------------------------------------------------------------

	/** BeginPlay 时授予的初始技能列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UMFGameplayAbilityBase>> DefaultAbilities;

	/** 初始 Loose GameplayTag（阵营声明，无需 GE）。通常包含 MF.Team.Player。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTagContainer DefaultOwnedTags;

	/** 初始属性值（MaxHealth/MoveSpeed/Attack/Defense/FleeThreshold）。BeginPlay 前复制到角色基类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FMFAttributeInitData InitAttributes;

	// -----------------------------------------------------------------------
	// 召唤宠物阵营 — 召唤时写入，覆盖宠物自带的中立/野生配置
	// -----------------------------------------------------------------------

	/** 召唤宠物的出生阵营标签。通常为 MF.Team.Player。SummonPet 时通过 SetFaction 写入宠物 ASC。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|SummonedPet")
	FGameplayTagContainer SummonedPetTeamTags;
	// 索敌方向由阵营自动判定（faction-auto），不再需要配置 SummonedPetTargetTags。

	// -----------------------------------------------------------------------
	// Combat — 战斗参数
	// -----------------------------------------------------------------------

	/** 被击闪红持续时间（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat",
		meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float HitFlashDuration = 0.25f;

	// -----------------------------------------------------------------------
	// Inventory — 背包配置
	// -----------------------------------------------------------------------

	/** 全局资源物品数据库（资源类物品的 MaxStackSize / 校验用）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UMFItemDatabase> ItemDatabase;

	/**
	 * AI 全局注册表 DataTable（行结构 FMFAIRegistryRow）。
	 * RowKey = AIConfigID（如 "Pet_SlimeCat"），值 = TSoftObjectPtr<UMFPetConfig>。
	 * 赋值 DT_AIRegistry。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UDataTable> AIRegistry;

	/** 资源格子上限（0 = 不限制）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0))
	int32 MaxResourceSlots = 0;

	/** 宠物携带上限（0 = 不限制）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0))
	int32 MaxPetSlots = 0;
};
