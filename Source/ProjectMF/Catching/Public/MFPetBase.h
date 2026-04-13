// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAICharacter.h"
#include "MFCatchable.h"
#include "MFPetBase.generated.h"

// MFItemTypes.h 不在此处 include（会循环），用前向声明 + 引用参数规避。
// MFPetBase.cpp 中 include MFItemTypes.h 获取完整定义。
struct FMFPetInstance;

/**
 * AMFPetBase — 可被玩家抓取的宠物 AI 基类。
 *
 * 继承关系：
 *   AMFPetBase → AMFAICharacter → AMFCharacterBase → ACharacter
 *                              └→ IMFMassControllable
 *             → IMFCatchable
 *
 * 职责：
 *   - 提供 IMFCatchable 的默认 C++ 实现，子类可重写以添加游戏逻辑。
 *   - 维护 bIsCaught 状态，防止已收服宠物被重复抓取。
 *   - 继承 AMFAICharacter 的全套 GAS / PaperZD / Mass 支持。
 *
 * 子类 (Blueprint 或 C++) 使用方式：
 *   1. 在 Blueprint Class Settings → Parent Class 设为 BP_MFPetBase（或直接继承本类）。
 *   2. 重写 CanBeCaught / OnCaught / OnCatchFailed 添加宠物特定逻辑。
 *   3. 配置 DefaultAbilities（继承自 AMFCharacterBase）为宠物专属技能。
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API AMFPetBase : public AMFAICharacter, public IMFCatchable
{
	GENERATED_BODY()

public:
	AMFPetBase();

	// -----------------------------------------------------------------------
	// IMFCatchable 默认实现
	// -----------------------------------------------------------------------

	/**
	 * 默认：未收服（!bIsCaught）时可抓。
	 * 子类可重写以添加等级差、特殊道具、状态限制等条件。
	 *
	 * @param Catcher  尝试抓取的玩家角色。
	 * @return true = 可抓（白色描边），false = 不可抓（红色描边）。
	 */
	virtual bool CanBeCaught_Implementation(const AActor* Catcher) const override;

	/**
	 * 默认：设置 bIsCaught = true 并记录日志。
	 * 子类在 Super::OnCaught_Implementation 之后添加动画、粒子、队伍注册等逻辑。
	 *
	 * @param Catcher  成功收服的玩家角色。
	 */
	virtual void OnCaught_Implementation(AActor* Catcher) override;

	/**
	 * 默认：记录失败日志，留有逃跑 AI 占位注释供子类扩展。
	 *
	 * @param Catcher  抓取失败的玩家角色。
	 */
	virtual void OnCatchFailed_Implementation(AActor* Catcher) override;

	// -----------------------------------------------------------------------
	// 状态查询
	// -----------------------------------------------------------------------

	/** 返回宠物是否已被收服（收服后 CanBeCaught 返回 false）。 */
	UFUNCTION(BlueprintPure, Category = "Pet|Catch")
	bool IsCaught() const { return bIsCaught; }

	// -----------------------------------------------------------------------
	// 数据绑定
	// -----------------------------------------------------------------------

	/**
	 * 对应 UMFItemDatabase 中的物品 ID（如 "Item.Pet.SlimeCat"）。
	 * 在 Blueprint CDO 中赋值，捕获成功后 InventoryComponent 用此 ID 注册宠物实例。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pet|Data")
	FName PetItemID;

	// -----------------------------------------------------------------------
	// 序列化接口
	// -----------------------------------------------------------------------

	/**
	 * 将当前 Actor 的关键状态写入 InOutInstance。
	 * - 捕获时：填充 PetItemID + AttributeSnapshot（由 GA_CatchPet 调用）
	 * - 召回时：仅刷新 AttributeSnapshot（由 InventoryComponent::RecallPet 调用）
	 * 子类可 Super:: 后追加自定义字段。
	 */
	virtual void SerializeToInstance(FMFPetInstance& InOutInstance) const;

	/**
	 * 从 Instance 恢复 Actor 运行时状态（由 InventoryComponent::SummonPet 在 BeginPlay 后调用）。
	 * 子类可 Super:: 后追加自定义恢复逻辑。
	 */
	virtual void RestoreFromInstance(const FMFPetInstance& Instance);

protected:
	// -----------------------------------------------------------------------
	// 收服状态
	// -----------------------------------------------------------------------

	/**
	 * 是否已被收服。
	 * 置为 true 后 CanBeCaught 始终返回 false，防止重复抓取。
	 * 子类可在特殊规则下重置（例如宠物从队伍中逃脱）。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet|Catch")
	bool bIsCaught = false;
};
