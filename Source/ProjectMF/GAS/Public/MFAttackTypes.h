// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackTypes.generated.h"

/**
 * Shape of the hit detection volume used by AI attack abilities.
 * Configure via UMFAttackAbilityData.
 */
UENUM(BlueprintType)
enum class EAttackShapeType : uint8
{
	Sphere  UMETA(DisplayName = "球形"),
	Sector  UMETA(DisplayName = "扇形"),
	Box     UMETA(DisplayName = "矩形 (AABB)"),
};

/**
 * Which faction of actors the attack should affect.
 * Resolved by comparing MF.Team.* GameplayTags on caster vs. candidate.
 */
UENUM(BlueprintType)
enum class EAttackTargetFilter : uint8
{
	EnemyOnly  UMETA(DisplayName = "仅敌方"),
	AllyOnly   UMETA(DisplayName = "仅友方"),
	All        UMETA(DisplayName = "全部"),
};
