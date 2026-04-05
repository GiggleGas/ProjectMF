// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "MFCamera.h"
#include "MFCharacterData.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

AMFCharacter::AMFCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Flipbook ---
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(RootComponent);

	// --- Camera Spring Arm ---
	// Fixed isometric angle; orbit yaw is controlled by UMFCameraController.
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength = 1200.f;
	CameraSpringArm->bDoCollisionTest = false;
	CameraSpringArm->bUsePawnControlRotation = false;
	CameraSpringArm->bInheritPitch = false;
	CameraSpringArm->bInheritYaw = false;
	CameraSpringArm->bInheritRoll = false;
	CameraSpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

	// --- Camera ---
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);

	// --- Camera Controller ---
	CameraController = CreateDefaultSubobject<UMFCameraController>(TEXT("CameraController"));
	CameraController->Initialize(CameraSpringArm, CameraComponent);
}

void AMFCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMFCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCharacterAction();
	UpdateAnimation();
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

		if (PickAction)
		{
			EnhancedInput->BindAction(PickAction, ETriggerEvent::Started, this, &AMFCharacter::HandlePickStarted);
			EnhancedInput->BindAction(PickAction, ETriggerEvent::Completed, this, &AMFCharacter::HandlePickCompleted);
		}

		if (RotateCameraAction)
		{
			EnhancedInput->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AMFCharacter::HandleCameraRotate);
		}
	}
}

void AMFCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	// Camera-relative movement: derive axes from spring arm's current yaw.
	const FRotator YawRotation(0.f, CameraSpringArm->GetRelativeRotation().Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(RightDir, MoveInput.X);
	}

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(ForwardDir, MoveInput.Y);
	}
}

void AMFCharacter::HandlePickStarted()
{
	CharacterState.bIsPicking = true;
}

void AMFCharacter::HandlePickCompleted()
{
	CharacterState.bIsPicking = false;
}

void AMFCharacter::HandleCameraRotate(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->AddOrbitYaw(Value.Get<float>());
	}
}

void AMFCharacter::UpdateCharacterAction()
{
	if (CharacterState.bIsPicking)
	{
		CharacterState.CurrentAction = EMFCharacterAction::Pick;
		return;
	}

	const FVector Velocity = GetVelocity();
	const FVector2D Vel2D(Velocity.X, Velocity.Y);

	if (Vel2D.SizeSquared() > SMALL_NUMBER)
	{
		CharacterState.CurrentAction = EMFCharacterAction::Walk;

		// Dominant axis determines facing direction
		if (FMath::Abs(Vel2D.X) >= FMath::Abs(Vel2D.Y))
		{
			CharacterState.FacingDirection = Vel2D.X > 0.f ? EMFFacingDirection::Right : EMFFacingDirection::Left;
		}
		else
		{
			CharacterState.FacingDirection = Vel2D.Y > 0.f ? EMFFacingDirection::Up : EMFFacingDirection::Down;
		}
	}
	else
	{
		CharacterState.CurrentAction = EMFCharacterAction::Idle;
	}
}

void AMFCharacter::UpdateAnimation()
{
	if (!AnimationConfig)
	{
		return;
	}

	UPaperFlipbook* TargetFlipbook = nullptr;

	switch (CharacterState.CurrentAction)
	{
	case EMFCharacterAction::Walk:
		TargetFlipbook = GetFlipbookForDirection(AnimationConfig->Walk, CharacterState.FacingDirection);
		break;
	case EMFCharacterAction::Pick:
		TargetFlipbook = GetFlipbookForDirection(AnimationConfig->Pick, CharacterState.FacingDirection);
		break;
	case EMFCharacterAction::Idle:
	default:
		TargetFlipbook = AnimationConfig->Idle;
		break;
	}

	if (TargetFlipbook && TargetFlipbook != GetCurrentFlipbook())
	{
		SetFlipbook(TargetFlipbook);
	}
}

UPaperFlipbook* AMFCharacter::GetFlipbookForDirection(const FMFDirectionalFlipbooks& Set, EMFFacingDirection Direction)
{
	switch (Direction)
	{
	case EMFFacingDirection::Right: return Set.Right;
	case EMFFacingDirection::Left:  return Set.Left;
	case EMFFacingDirection::Up:    return Set.Up;
	case EMFFacingDirection::Down:  return Set.Down;
	default:                        return nullptr;
	}
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
