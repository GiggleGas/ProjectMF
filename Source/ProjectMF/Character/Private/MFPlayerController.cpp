// Copyright ProjectMF. All Rights Reserved.

#include "MFPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AMFPlayerController::AMFPlayerController()
{
}

void AMFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	AddInputMappingContext();
}

void AMFPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AddInputMappingContext();
}

void AMFPlayerController::OnUnPossess()
{
	RemoveInputMappingContext();

	Super::OnUnPossess();
}

void AMFPlayerController::AddInputMappingContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
	}
}

void AMFPlayerController::RemoveInputMappingContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(DefaultMappingContext);
	}
}
