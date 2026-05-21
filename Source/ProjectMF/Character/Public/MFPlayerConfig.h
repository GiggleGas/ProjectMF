// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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

	/**
	 * 初始化属性的 Instant GameplayEffect（MaxHealth、MoveSpeed、Attack 等）。
	 * 留空则使用 AttributeSet 构造函数默认值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultInitEffect;

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
