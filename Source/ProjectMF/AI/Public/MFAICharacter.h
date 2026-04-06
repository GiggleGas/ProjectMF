// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFCharacterBase.h"
#include "MFMassInterface.h"
#include "MFAICharacter.generated.h"

/**
 * Base class for all AI-controlled characters in ProjectMF.
 *
 * Inherits the 2D PaperZD rendering pipeline from AMFCharacterBase and adds:
 *  - Billboard alignment toward the player camera (no own camera rig needed).
 *  - IMFMassControllable implementation: Mass Processors can push move targets
 *    and action overrides each tick via ApplyMassCommand().
 *
 * Blueprint subclass (e.g., BP_MFEnemy) should:
 *  - Set the AnimBP class to an AnimBP that references UMFAIAnimInstance.
 *  - Configure the CharacterMovementComponent (speed, nav walking, etc.).
 *  - Optionally assign an AIController Blueprint for behavior trees.
 *
 * Mass integration notes (see MFMassInterface.h for the roadmap):
 *  Phase 1: Processor fetches the Actor, calls Execute_ApplyMassCommand().
 *  Phase 2: Use FMassEntityHandle fragments and MassRepresentationProcessor.
 */
UCLASS()
class PROJECTMF_API AMFAICharacter : public AMFCharacterBase, public IMFMassControllable
{
	GENERATED_BODY()

public:
	AMFAICharacter();

	// -----------------------------------------------------------------------
	// IMFMassControllable
	// -----------------------------------------------------------------------

	/** Push a movement/action command from a Mass Processor. */
	virtual void ApplyMassCommand_Implementation(const FMFAICommand& Command) override;

	/** Notify that a Mass Entity has claimed this Actor as its visual representation. */
	virtual void OnMassEntityLinked_Implementation(int32 EntityIndex) override;

	/** Notify that the Mass Entity has released this Actor (LOD swap / pooling). */
	virtual void OnMassEntityUnlinked_Implementation() override;

	// -----------------------------------------------------------------------
	// State accessors (read-only from Blueprints / other systems)
	// -----------------------------------------------------------------------

	/** Mass Entity index this Actor is currently representing. -1 = unlinked. */
	UFUNCTION(BlueprintPure, Category = "Mass")
	int32 GetLinkedMassEntityIndex() const { return LinkedMassEntityIndex; }

	/** Returns true if this Actor is currently driven by a Mass Entity. */
	UFUNCTION(BlueprintPure, Category = "Mass")
	bool IsLinkedToMassEntity() const { return LinkedMassEntityIndex != INDEX_NONE; }

protected:
	virtual void BeginPlay() override;

	// -----------------------------------------------------------------------
	// AMFCharacterBase camera interface
	// -----------------------------------------------------------------------

	/**
	 * Return the player camera forward vector so all AI sprites share the same
	 * uniform tilt as the player sprite (parallel billboard, no per-object deviation).
	 */
	virtual bool  GetBillboardCameraForward(FVector& OutForward) const override;

	/**
	 * Use the player camera yaw so AI sprite directions match the player's view.
	 * Keeps directional sprites consistent across player and AI characters.
	 */
	virtual float GetCameraYawForDirectionality() const override;

	// -----------------------------------------------------------------------
	// Mass-driven state (protected so Blueprint subclasses can inspect it)
	// -----------------------------------------------------------------------

	/** Opaque index of the linked Mass Entity. INDEX_NONE (-1) when unlinked. */
	UPROPERTY(BlueprintReadOnly, Category = "Mass")
	int32 LinkedMassEntityIndex = INDEX_NONE;

	/** Cached move target from the last Mass command (world space). */
	UPROPERTY(BlueprintReadOnly, Category = "Mass")
	FVector MassMoveTarget = FVector::ZeroVector;

private:
	/** Apply movement toward MassMoveTarget if the command carries a valid target. */
	void ApplyMovementFromCommand(const FMFAICommand& Command);
};
