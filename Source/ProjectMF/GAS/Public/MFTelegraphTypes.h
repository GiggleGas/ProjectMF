// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFTelegraphTypes.generated.h"

/** 预警形状。 */
UENUM(BlueprintType)
enum class EMFTelegraphShape : uint8
{
	Circle  UMETA(DisplayName = "圆形"),
	Rect    UMETA(DisplayName = "矩形"),
};

/** 一次预警请求的参数（位置/形状/尺寸/颜色）。 */
USTRUCT(BlueprintType)
struct FMFTelegraphRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	EMFTelegraphShape Shape = EMFTelegraphShape::Circle;

	/** 世界位置（地面点）。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	FVector Location = FVector::ZeroVector;

	/** 朝向（仅 Rect 用其 Yaw）。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	FRotator Rotation = FRotator::ZeroRotator;

	/** 半径（cm）。Circle 用。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	float Radius = 100.f;

	/** 矩形尺寸（长 × 宽，cm）。Rect 用。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	FVector2D BoxSize = FVector2D(200.f, 100.f);

	/** 颜色（材质需有 Color / DecalColor 向量参数才生效）。 */
	UPROPERTY(BlueprintReadWrite, Category = "MF|Telegraph")
	FLinearColor Color = FLinearColor(1.f, 0.1f, 0.1f, 0.5f);
};

/** 预警句柄（用于更新 / 隐藏）。 */
USTRUCT(BlueprintType)
struct FMFTelegraphHandle
{
	GENERATED_BODY()

	uint32 Id = 0;

	bool IsValid() const { return Id != 0; }
	void Invalidate() { Id = 0; }
};
