// Copyright ProjectMF. All Rights Reserved.

#include "MFAICharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

AMFAICharacter::AMFAICharacter()
{
	// AI characters auto-possess an AIController when placed or spawned.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMFAICharacter::BeginPlay()
{
	Super::BeginPlay();
}

// ---------------------------------------------------------------------------
// Camera interface (AMFCharacterBase)
// ---------------------------------------------------------------------------

bool AMFAICharacter::GetBillboardCameraForward(FVector& OutForward) const
{
	// Use the player camera forward vector — same as the player sprite's billboard axis.
	if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		OutForward = PCM->GetCameraRotation().Vector();
		return true;
	}
	return false;
}

float AMFAICharacter::GetCameraYawForDirectionality() const
{
	// Match the player camera yaw so AI sprite facing is consistent with player sprites.
	if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		return PCM->GetCameraRotation().Yaw;
	}
	return 0.f;
}

// ---------------------------------------------------------------------------
// IMFMassControllable
// ---------------------------------------------------------------------------

void AMFAICharacter::ApplyMassCommand_Implementation(const FMFAICommand& Command)
{
	// 1. Movement
	ApplyMovementFromCommand(Command);

	// 2. Action override (legacy path — kept for non-GAS callers)
	if (Command.bOverrideAction)
	{
		CharacterState.bIsPicking = (Command.DesiredAction == EMFCharacterAction::Pick);
	}

	// 3. GAS ability activation (preferred path — drives state via tag)
	if (Command.bActivateAbility && Command.AbilityTagToActivate.IsValid() && AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(Command.AbilityTagToActivate));
	}
}

void AMFAICharacter::OnMassEntityLinked_Implementation(int32 EntityIndex)
{
	LinkedMassEntityIndex = EntityIndex;
}

void AMFAICharacter::OnMassEntityUnlinked_Implementation()
{
	LinkedMassEntityIndex = INDEX_NONE;
	CharacterState.bIsPicking = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AMFAICharacter::ApplyMovementFromCommand(const FMFAICommand& Command)
{
	if (!Command.bHasMoveTarget) return;

	MassMoveTarget = Command.MoveTarget;

	// Flat (XY) direction toward target.
	const FVector2D ToTarget2D(
		MassMoveTarget.X - GetActorLocation().X,
		MassMoveTarget.Y - GetActorLocation().Y);

	// Dead-zone: stop jittering when already close.
	constexpr float ArrivalRadiusSq = 10.f * 10.f;
	if (ToTarget2D.SizeSquared() < ArrivalRadiusSq) return;

	const FVector Direction(ToTarget2D.GetSafeNormal(), 0.f);
	AddMovementInput(Direction, 1.f);

	// TODO(Mass Phase 2): Replace with UPathFollowingComponent or UCrowdFollowingComponent
	//   for proper nav-mesh avoidance when crowds are large.
}
