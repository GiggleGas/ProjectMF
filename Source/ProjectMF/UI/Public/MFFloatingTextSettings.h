// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFFloatingTextSettings.generated.h"

class UMFDamageNumberWidget;

/**
 * 战斗飘字（伤害/治疗数字）的全局配置。
 * 项目设置 → Game → MF Floating Text。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MF Floating Text"))
class PROJECTMF_API UMFFloatingTextSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	/** 飘字 widget 类（继承 UMFDamageNumberWidget，内含一个名为 AmountText 的 TextBlock）。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	TSoftClassPtr<UMFDamageNumberWidget> WidgetClass;

	/** 总开关。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	bool bEnabled = true;

	/** 飘字锚点相对角色的头顶世界偏移（cm）。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText", meta = (ClampMin = "0.0"))
	float HeadZOffset = 80.f;

	/** 存活时长（秒）。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText", meta = (ClampMin = "0.1"))
	float Lifetime = 0.9f;

	// --- 跳出（缩放过冲）---
	/** 起始缩放。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Pop", meta = (ClampMin = "0.0"))
	float StartScale = 0.4f;
	/** 过冲峰值缩放。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Pop", meta = (ClampMin = "0.0"))
	float OvershootScale = 1.25f;
	/** 缩放过冲达成时长（秒），之后稳定在 SettleScale。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Pop", meta = (ClampMin = "0.01"))
	float PopTime = 0.15f;

	/** 稳定缩放随机量（±）：每个数字最终大小在 1.0±此值 间随机，避免大小一致。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Pop", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float ScaleJitter = 0.18f;

	// --- 上抛弧线（屏幕像素）---
	/** 向上初速基准（px/s）。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Motion", meta = (ClampMin = "0.0"))
	float LaunchUpSpeed = 220.f;
	/** 初速随机量（±px/s）：每个数字飞出速度不同。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Motion", meta = (ClampMin = "0.0"))
	float LaunchSpeedJitter = 70.f;
	/** 发射角随机量（±度，相对竖直向上）：每个数字飞出方向不同。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Motion", meta = (ClampMin = "0.0", ClampMax = "85.0"))
	float LaunchAngleJitter = 30.f;
	/** 重力（px/s²），让上抛减速形成短弧。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText|Motion", meta = (ClampMin = "0.0"))
	float Gravity = 480.f;

	/** 末段淡出时长（秒）。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText", meta = (ClampMin = "0.0"))
	float FadeOutTime = 0.35f;

	/** 伤害颜色。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	FLinearColor DamageColor = FLinearColor(1.0f, 0.25f, 0.2f);

	/** 治疗颜色。 */
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	FLinearColor HealColor = FLinearColor(0.35f, 1.0f, 0.4f);
};
