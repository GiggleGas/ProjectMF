// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

AMFCharacter::AMFCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(RootComponent);
}

void AMFCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IdleFlipbook)
	{
		SetFlipbook(IdleFlipbook);
	}
}

void AMFCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMFCharacter::HandleMove);
		}

		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AMFCharacter::HandleJumpStarted);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMFCharacter::HandleJumpCompleted);
		}
	}
}

void AMFCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (MoveInput.X != 0.f)
	{
		AddMovementInput(FVector(1.f, 0.f, 0.f), MoveInput.X);
	}

	if (MoveInput.Y != 0.f)
	{
		AddMovementInput(FVector(0.f, 1.f, 0.f), MoveInput.Y);
	}
}

void AMFCharacter::HandleJumpStarted()
{
	Jump();
}

void AMFCharacter::HandleJumpCompleted()
{
	StopJumping();
}

void AMFCharacter::SetFlipbook(UPaperFlipbook* NewFlipbook)
{
	if (FlipbookComponent && NewFlipbook)
	{
		FlipbookComponent->SetFlipbook(NewFlipbook);
	}
}

UPaperFlipbook* AMFCharacter::GetCurrentFlipbook() const
{
	if (FlipbookComponent)
	{
		return FlipbookComponent->GetFlipbook();
	}
	return nullptr;
}
