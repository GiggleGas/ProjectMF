// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MFCombatAttributeSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)            \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)  \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * Combat attribute set for all MF characters (player and AI/pet/boss).
 *
 * Mounted alongside UMFAttributeSetBase on every AMFCharacterBase.
 * Default values (Attack=0, Defense=0) are neutral for the player;
 * pets and bosses receive real values via GE_CharacterInit at BeginPlay.
 *
 * Attributes:
 *   Attack        — base attack power, carried via SetByCaller into GE_DamageBase.
 *   Defense       — flat damage reduction applied in PostGameplayEffectExecute.
 *   FleeThreshold — HP ratio [0,1] below which OnLowHealth fires. Default 0.3.
 */
UCLASS()
class PROJECTMF_API UMFCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMFCombatAttributeSet();

	// -----------------------------------------------------------------------
	// Combat stats
	// -----------------------------------------------------------------------

	/** Base attack power. Passed via SetByCaller("Data.Damage") into GE_DamageBase. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(UMFCombatAttributeSet, Attack)

	/**
	 * Flat damage reduction (not percentage).
	 * FinalDamage = max(IncomingDamage - Defense, 1).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UMFCombatAttributeSet, Defense)

	/**
	 * HP ratio threshold for the "low health / flee" state [0, 1].
	 * When Health < MaxHealth * FleeThreshold, UMFAttributeSetBase broadcasts
	 * OnLowHealth and StateTree can switch to the Flee state.
	 * Default: 0.3 (30 % HP).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData FleeThreshold;
	ATTRIBUTE_ACCESSORS(UMFCombatAttributeSet, FleeThreshold)

	// -----------------------------------------------------------------------
	// UAttributeSet interface
	// -----------------------------------------------------------------------

	/** Clamp Attack and Defense to [0, +inf]; FleeThreshold to [0, 1]. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
