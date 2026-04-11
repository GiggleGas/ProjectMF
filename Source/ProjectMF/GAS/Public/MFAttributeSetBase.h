// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MFAttributeSetBase.generated.h"

// Boilerplate macro: generates Property getter, Value getter/setter/initter
// for a FGameplayAttributeData member. Must be called inside the class body.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)            \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)  \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Base attribute set shared by all MF characters (player and AI).
 *
 * Defines the minimal stats every character owns:
 *   - Health / MaxHealth
 *   - MoveSpeed
 *
 * Pet-specific stats (Attack, Defense, MP, Luck, etc.) live in
 * UMFPetAttributeSet, which will be added to pet actors on top of this set.
 *
 * Attribute initialization:
 *   Default values are set in the constructor via InitXxx().
 *   Later, a startup GameplayEffect (GE_CharacterInit) can override these
 *   per character type once the GE system is set up.
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
	// UAttributeSet interface
	// -----------------------------------------------------------------------

	/** Clamp Health to [0, MaxHealth] before the attribute is committed. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
