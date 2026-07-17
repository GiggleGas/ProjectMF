// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFCraftRowWidget.generated.h"

class UTextBlock;

/** 点击某配方（可合成时），参数 = 配方 ID。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMFCraftClicked, FName, RecipeID);

/**
 * 合成页里的一行配方。仿 UMFItemSlotWidget：逻辑全 C++，蓝图只摆 BindWidget。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFCraftRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 填充：显示名 + 可否合成。可合成才响应点击。 */
	void SetRecipe(FName InRecipeID, const FText& InDisplayName, bool bInCanCraft);

	FName GetRecipeID() const { return RecipeID; }

	/** 点击（可合成时）触发。 */
	FOnMFCraftClicked OnCraftClicked;

	/** 配方名。Designer 放 TextBlock 命名 "RecipeName"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RecipeName;

protected:
	/** 可/不可合成外观钩子——蓝图定制高亮/置灰。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void OnCraftableStateChanged(bool bInCanCraft);

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	FName RecipeID;
	bool  bCanCraft = false;
};
