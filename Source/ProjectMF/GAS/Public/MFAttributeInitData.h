// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttributeInitData.generated.h"

/**
 * 角色属性初始值（扁平配置）。
 *
 * 取代以往的 init GameplayEffect（GE_Init_*）：初始数值就地明文配置，
 * 由 AMFCharacterBase::ApplyAttributeInitData 在初始化时用 SetNumericAttributeBase 直接写入。
 *
 * 字段说明：
 *   - Health 初始 = MaxHealth（不单列）。
 *   - Attack / Defense / FleeThreshold 仅对挂载了 UMFCombatAttributeSet 的角色（AI/宠物/Boss）生效；
 *     玩家无战斗集，这三项会被忽略。
 *   - 增减伤系数（Incoming/OutgoingDamageMultiplier）恒为 1，由 AttributeSet 构造负责，不在此配置。
 *
 * 当前为扁平值，无等级缩放；等级成长见设计文档「养成 / 等级系统」规划。
 */
USTRUCT(BlueprintType)
struct FMFAttributeInitData
{
	GENERATED_BODY()

	/** 最大生命（初始生命同步设为此值）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	/** 移动速度（cm/s），由 MoveSpeed 属性驱动 MaxWalkSpeed。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.f;

	/** 攻击力（仅 AI/宠物/Boss 生效）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Attack = 0.f;

	/** 防御力，平砍减免（仅 AI/宠物/Boss 生效）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Defense = 0.f;

	/** 低血逃跑阈值 [0,1]（仅 AI/宠物/Boss 生效）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeThreshold = 0.3f;
};
