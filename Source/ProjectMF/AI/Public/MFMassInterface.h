// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MFCharacterTypes.h"
#include "MFMassInterface.generated.h"

// ---------------------------------------------------------------------------
// Command packet sent from a Mass Processor to an AI Character Actor.
// ---------------------------------------------------------------------------

/**
 * Encapsulates everything a Mass Processor needs to tell an AI Character for one tick.
 *
 * Usage pattern (Mass Processor side):
 *   FMFAICommand Cmd;
 *   Cmd.bHasMoveTarget  = true;
 *   Cmd.MoveTarget      = TargetLocation;
 *   IMFMassControllable::Execute_ApplyMassCommand(CharacterActor, Cmd);
 *
 * TODO(Mass): When MassEntity module is added, consider storing FMassEntityHandle here
 *             instead of routing commands through the Actor interface directly.
 *             Preferred Mass pattern is Processor → Fragment → Actor::NativeEventDelegate.
 */
USTRUCT(BlueprintType)
struct FMFAICommand
{
	GENERATED_BODY()

	/** World-space destination the AI should move toward. */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Command")
	FVector MoveTarget = FVector::ZeroVector;

	/** Whether MoveTarget is valid and should be applied this tick. */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Command")
	bool bHasMoveTarget = false;

	/**
	 * Desired action state override from the Processor.
	 * Ignored unless bOverrideAction is true.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Command")
	EMFCharacterAction DesiredAction = EMFCharacterAction::Idle;

	/**
	 * When true, DesiredAction overrides the character's own state machine.
	 * When false, the character decides its own action from velocity/pick state.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Command")
	bool bOverrideAction = false;
};

// ---------------------------------------------------------------------------
// IMFMassControllable — implement on characters driven by Mass
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, BlueprintType)
class UMFMassControllable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for AI Characters that can be driven by a Mass Processor.
 *
 * Integration roadmap:
 *  Phase 1 (current) — Actor-based: Processor fetches the Actor pointer from a
 *    FMassEntityHandle fragment and calls ApplyMassCommand() directly.
 *
 *  Phase 2 — Full Mass: Replace Actor pointer lookups with a dedicated
 *    FMFActorRepresentationFragment; use MassSignalSubsystem or
 *    UMassRepresentationProcessor patterns to drive character actors.
 *
 * Only AMFAICharacter implements this interface; AMFCharacter (player) does not.
 */
class PROJECTMF_API IMFMassControllable
{
	GENERATED_BODY()

public:
	/**
	 * Called by the Mass Processor each tick to push movement and action commands.
	 * Implementations should apply movement immediately via AddMovementInput.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass")
	void ApplyMassCommand(const FMFAICommand& Command);

	/**
	 * Called once when a Mass Entity is assigned to this Actor for representation.
	 *
	 * @param EntityIndex  Opaque index identifying the Mass Entity.
	 *   TODO(Mass): Replace with FMassEntityHandle when MassEntity module is available.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass")
	void OnMassEntityLinked(int32 EntityIndex);

	/**
	 * Called when Mass releases this Actor (e.g., LOD swap, object pooling).
	 * Implementations should stop movement and reset any command-driven state.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass")
	void OnMassEntityUnlinked();
};
