// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFOverheadWidget.generated.h"

class UAbilitySystemComponent;
class UWidget;
struct FOnAttributeChangeData;
struct FGameplayTag;

/**
 * Base class for the widget that floats above each character's head (Screen Space).
 *
 * C++ handles ASC binding and attribute change callbacks.
 * Blueprint subclass implements BP_OnHealthChanged to drive the ProgressBar visuals.
 *
 * Usage:
 *   1. Create WBP_OverheadHP inheriting from UMFOverheadWidget in the Editor.
 *   2. Add a ProgressBar named "HealthBar" and implement BP_OnHealthChanged to set its percent.
 *   3. Assign WBP_OverheadHP to OverheadWidgetClass on the character Blueprint.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by AMFCharacterBase::BeginPlay after GAS is initialized. */
	void InitWithASC(UAbilitySystemComponent* InASC);

protected:
	/**
	 * Implement in Blueprint to update the health bar visuals.
	 * Called immediately on init (with current values) and on every health/MaxHealth change.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Overhead")
	void BP_OnHealthChanged(float CurrentHP, float MaxHP);

	virtual void NativeDestruct() override;

private:
	/** 头顶眩晕图标。WBP 里放一个名为 StunIcon 的控件即可，显隐由 C++ 控制（无需 BP 逻辑）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StunIcon;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle StunnedTagHandle;

	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);

	/** State.Stunned 标签变化 → 切换 StunIcon 显隐。 */
	void OnStunnedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
};
