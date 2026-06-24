// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFTelegraphSettings.generated.h"

class UMaterialInterface;

/**
 * UMFTelegraphSettings — 预警系统的项目配置（Project Settings → Game → MF Telegraph，写入 DefaultGame.ini）。
 *
 * 配置进 Config、非蓝图。DecalMaterial 留空时子系统兜底用引擎默认 Deferred Decal 材质
 * （能渲染出一块投影，但非纯色/非圆形）；做一个 Deferred Decal 域、带 Color 向量参数的材质
 * 指定到这里即可得到纯色圆/矩形预警。
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "MF Telegraph"))
class PROJECTMF_API UMFTelegraphSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 预警 decal 材质（Deferred Decal 域，最好带 Color/DecalColor 向量参数）。留空走引擎默认兜底。 */
	UPROPERTY(EditAnywhere, config, Category = "Telegraph")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	/** decal 投影深度（cm，沿地面法线方向的投影厚度）。 */
	UPROPERTY(EditAnywhere, config, Category = "Telegraph", meta = (ClampMin = "10.0"))
	float ProjectionDepth = 256.f;

	virtual FName GetCategoryName() const override { return FName("Game"); }
};
