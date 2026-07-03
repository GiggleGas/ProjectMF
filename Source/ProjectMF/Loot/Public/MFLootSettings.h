// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFLootSettings.generated.h"

class AMFLootPickup;

/**
 * UMFLootSettings — 掉落物系统的项目配置（Project Settings → Game → MF Loot，写入 DefaultGame.ini）。
 *
 * 生成参数在这里（Subsystem 读取）；单个掉落物的手感参数（吸附速度/寿命等）在
 * AMFLootPickup 类默认值上（BP 子类调）——每项配置只有一个归属地。
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "MF Loot"))
class PROJECTMF_API UMFLootSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * 掉落物 Actor 类。建议配置成 AMFLootPickup 的 BP 子类（在 BP 里设 Flipbook 外观）。
	 * 留空回退 C++ AMFLootPickup——逻辑可用但没有外观（隐形），会打警告日志。
	 */
	UPROPERTY(EditAnywhere, config, Category = "Loot")
	TSoftClassPtr<AMFLootPickup> PickupClass;

	/** 多个掉落物围绕掉落点散开的半径（cm）。 */
	UPROPERTY(EditAnywhere, config, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float ScatterRadius = 100.f;

	virtual FName GetCategoryName() const override { return FName("Game"); }
};
