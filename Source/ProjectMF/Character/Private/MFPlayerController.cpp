// Copyright ProjectMF. All Rights Reserved.

#include "MFPlayerController.h"
#include "MFPlayerConfig.h"
#include "MFMainHUDWidget.h"
#include "MFCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"

AMFPlayerController::AMFPlayerController()
{
}

void AMFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	AddInputMappingContext();

	if (PlayerConfig && PlayerConfig->MainHUDClass)
	{
		MainHUDInstance = CreateWidget<UMFMainHUDWidget>(this, PlayerConfig->MainHUDClass);
		if (MainHUDInstance)
		{
			MainHUDInstance->AddToViewport();
			MainHUDInstance->InitPlayerHUD(Cast<AMFCharacter>(GetPawn()));
		}
	}
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
	if (!PlayerConfig || !PlayerConfig->DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(PlayerConfig->DefaultMappingContext, PlayerConfig->DefaultMappingPriority);
	}
}

void AMFPlayerController::RemoveInputMappingContext()
{
	if (!PlayerConfig || !PlayerConfig->DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(PlayerConfig->DefaultMappingContext);
	}
}
