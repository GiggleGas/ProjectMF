// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "MFAttackTypes.h"
#include "MFCombatEffectSettings.generated.h"

class UGameplayEffect;

/**
 * 一种命中附加效果（按 EMFOnHitEffectKind 索引）的定义：
 * 施加哪个 GE，以及把 OnHitEffects 里的 Duration / Magnitude 写入哪些 SetByCaller 键。
 *
 * 配好这一行即可知道该效果需要哪些配置值：
 *   - DurationTag  非空 → 该效果吃「持续时间」（写入此键）
 *   - MagnitudeTag 非空 → 该效果吃一个「主数值」（写入此键），含义见 MagnitudeLabel
 */
USTRUCT(BlueprintType)
struct FMFEffectKindDef
{
	GENERATED_BODY()

	/** 此类型施加的 GE 模板（数值由下面的键经 SetByCaller 注入）。 */
	UPROPERTY(EditAnywhere, Config, Category = "Effect")
	TSubclassOf<UGameplayEffect> Effect;

	/** 持续时间写入的 SetByCaller 键（一般 MF.Data.Duration；瞬发类留空）。 */
	UPROPERTY(EditAnywhere, Config, Category = "Effect")
	FGameplayTag DurationTag;

	/** 主数值写入的 SetByCaller 键（如减速→MF.Data.MoveSpeedMult；无主数值则留空）。 */
	UPROPERTY(EditAnywhere, Config, Category = "Effect")
	FGameplayTag MagnitudeTag;

	/** 主数值的含义说明（仅文档用，如"减速到移速×"）。 */
	UPROPERTY(EditAnywhere, Config, Category = "Effect")
	FText MagnitudeLabel;
};

/**
 * 战斗效果映射表：EMFOnHitEffectKind → FMFEffectKindDef。
 * 项目设置 → Game → MF Combat Effects。
 *
 * OnHitEffects 只选 Kind + 填 Duration/Magnitude 值；GE 与「值写哪个键」由本表解析。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MF Combat Effects"))
class PROJECTMF_API UMFCombatEffectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	/** 命中附加效果映射表（每种 Kind 对应的 GE 与 SetByCaller 键）。 */
	UPROPERTY(EditAnywhere, Config, Category = "OnHitEffects")
	TMap<EMFOnHitEffectKind, FMFEffectKindDef> EffectMap;
};
