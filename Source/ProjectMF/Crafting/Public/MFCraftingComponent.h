// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFCraftingComponent.generated.h"

class UMFInventoryComponent;
struct FMFRecipeDef;

/** 背包/可合成状态变化时广播（UI 刷新可合成高亮）。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftableChanged);

/**
 * UMFCraftingComponent — 打造合成组件（挂玩家）。
 *
 * 配方来自全局 UMFCraftingSettings 配方库；扣料/产出走 owner 的 UMFInventoryComponent。
 * 合成逻辑极轻：CanCraft 查料够，Craft 扣 Inputs + 产出 Output 入包。
 */
UCLASS(ClassGroup = (MF), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFCraftingComponent();

	/** 背包料是否够合成该配方。 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraft(FName RecipeID) const;

	/** 配方库全部配方 ID。 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FName> GetAllRecipes() const;

	/** 合成：校验 → 扣料 → 产出入包。成功返回 true。 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool Craft(FName RecipeID);

	/** 配方显示名（DisplayName 非空 → 用它；否则回退产出物名 / RecipeID）。 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	FText GetRecipeDisplayName(FName RecipeID) const;

	UPROPERTY(BlueprintAssignable, Category = "Crafting|Events")
	FOnCraftableChanged OnCraftableChanged;

protected:
	virtual void BeginPlay() override;

private:
	/** owner 上的背包组件。 */
	UMFInventoryComponent* GetInventory() const;

	/** 从全局配方库按 RowName 查配方；无则 nullptr。 */
	const FMFRecipeDef* FindRecipe(FName RecipeID) const;

	UFUNCTION() void HandleInventoryChanged();
};
