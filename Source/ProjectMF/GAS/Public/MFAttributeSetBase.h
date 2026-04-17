// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "MFAttributeSetBase.generated.h"

// Boilerplate macro: generates Property getter, Value getter/setter/initter
// for a FGameplayAttributeData member. Must be called inside the class body.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)            \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)  \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// -----------------------------------------------------------------------
// Damage event delegates (broadcast from PostGameplayEffectExecute)
// -----------------------------------------------------------------------

/** 角色血量变化时广播（NewHealth 已夹紧到 [0, MaxHealth]）。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FMFOnHealthChanged, float /*NewHealth*/);

/** 角色 HP 归零时广播。只广播一次（由 PostGE 保证）。 */
DECLARE_MULTICAST_DELEGATE(FMFOnDeath);

/**
 * 角色 HP 低于 FleeThreshold 时广播。
 * 每次受伤后如果仍低于阈值都会广播，由监听方自行去重。
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FMFOnLowHealth, float /*NewHealth*/);

/**
 * Base attribute set shared by all MF characters (player and AI).
 *
 * Defines the minimal stats every character owns:
 *   - Health / MaxHealth
 *   - MoveSpeed
 *   - Damage (meta attribute, consumed in PostGameplayEffectExecute)
 *
 * Combat stats (Attack, Defense, FleeThreshold) live in UMFCombatAttributeSet,
 * which is added to every character on top of this set.
 *
 * Damage pipeline:
 *   GA_Attack → GE_DamageBase (SetByCaller "Data.Damage") → Damage meta attr
 *   → PostGameplayEffectExecute: reads Defense from UMFCombatAttributeSet,
 *     computes FinalDamage = max(Damage - Defense, 1), subtracts from Health,
 *     then broadcasts OnHealthChanged / OnDeath / OnLowHealth.
 */
UCLASS()
class PROJECTMF_API UMFAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMFAttributeSetBase();

	// -----------------------------------------------------------------------
	// Health
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMFAttributeSetBase, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMFAttributeSetBase, MaxHealth)

	// -----------------------------------------------------------------------
	// Movement
	// -----------------------------------------------------------------------

	/** Base movement speed (units/s). Applied to CharacterMovementComponent via GE. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UMFAttributeSetBase, MoveSpeed)

	// -----------------------------------------------------------------------
	// Damage (meta attribute — not persisted, consumed in PostGE)
	// -----------------------------------------------------------------------

	/**
	 * Transient incoming damage value. Set by GE_DamageBase → consumed and reset
	 * to 0 in PostGameplayEffectExecute. Never query this from outside the PostGE.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UMFAttributeSetBase, Damage)

	// -----------------------------------------------------------------------
	// Damage pipeline events
	// -----------------------------------------------------------------------

	FMFOnHealthChanged OnHealthChanged;
	FMFOnDeath         OnDeath;
	FMFOnLowHealth     OnLowHealth;

	// -----------------------------------------------------------------------
	// UAttributeSet interface
	// -----------------------------------------------------------------------

	/** Clamp Health to [0, MaxHealth] before the attribute is committed. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/**
	 * Consume the Damage meta attribute, apply Defense reduction, subtract from
	 * Health, then broadcast OnHealthChanged / OnDeath / OnLowHealth.
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
