// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "MFFactionStatics.generated.h"

class UAbilitySystemComponent;

/**
 * UMFFactionStatics — 阵营工具静态库。
 *
 * 阵营用 ASC 上的 Loose GameplayTag (MF.Team.*) 表示：
 *   - AI 出生默认中立（无 MF.Team.* 标签）。
 *   - 召唤宠物 / Boss 在生成时通过 SetFaction 写入各自阵营标签。
 *
 * 伤害过滤（GA_AIAttackBase / GA_AIRangedAttackBase 的 FilterTarget）调用 AreSameTeam
 * 判定双方是否同阵营：共享任意 MF.Team.* 标签即同队；中立与所有人都不同队。
 */
UCLASS()
class PROJECTMF_API UMFFactionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 设置 ASC 的阵营：先移除已有的 MF.Team.* Loose 标签，再写入 NewTeamTags。
	 * 传入空容器即清为中立。
	 */
	UFUNCTION(BlueprintCallable, Category = "MF|Faction")
	static void SetFaction(UAbilitySystemComponent* ASC, const FGameplayTagContainer& NewTeamTags);

	/**
	 * 是否同阵营：A、B 共享任意 MF.Team.* 标签则为 true。
	 * 任一方中立（无队伍标签）或任一 ASC 为空 → false。
	 */
	UFUNCTION(BlueprintPure, Category = "MF|Faction")
	static bool AreSameTeam(const UAbilitySystemComponent* ASCA, const UAbilitySystemComponent* ASCB);

	/**
	 * 是否敌对：双方都有非空 Team 且不共享任何 Team 标签 → true。
	 * 任一方中立（无队）或 ASC 为空 → false（中立既不索敌也不被索）。
	 *
	 * 全工程"敌方"判定（索敌 / 伤害 EnemyOnly / 投射物）统一走此函数。
	 * 与 AreSameTeam 互补：同队、敌对、（双方都中立时）皆非——三态而非二元取反，
	 * 故 !AreSameTeam ≠ AreHostile（前者会把中立算成敌人）。
	 */
	UFUNCTION(BlueprintPure, Category = "MF|Faction")
	static bool AreHostile(const UAbilitySystemComponent* ASCA, const UAbilitySystemComponent* ASCB);

	/** 返回 ASC 当前持有的所有 MF.Team.* 标签。 */
	static FGameplayTagContainer GetTeamTags(const UAbilitySystemComponent* ASC);
};
