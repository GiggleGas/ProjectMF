// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "MFCamera.h"
#include "MFPlayerAnimInstance.h"
#include "PaperFlipbookComponent.h"
#include "PaperZDAnimationComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

AMFCharacter::AMFCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Flipbook (render target driven by PaperZD) ---
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(RootComponent);

	// --- PaperZD Animation ---
	// Set the AnimBP class to ABP_Player in the derived Blueprint (B_MFCharacter_Player).
	AnimationComponent = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("AnimationComponent"));
	AnimationComponent->InitRenderComponent(FlipbookComponent);

	// --- Spring Arm ---
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength         = 1200.f;
	CameraSpringArm->bDoCollisionTest        = false;
	CameraSpringArm->bUsePawnControlRotation = false;
	CameraSpringArm->bInheritPitch           = false;
	CameraSpringArm->bInheritYaw             = false;
	CameraSpringArm->bInheritRoll            = false;
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
	// UpdateBillboard();
}

void AMFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EI->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMFCharacter::HandleMove);
		}
		if (PickAction)
		{
			EI->BindAction(PickAction, ETriggerEvent::Started,   this, &AMFCharacter::HandlePickStarted);
			EI->BindAction(PickAction, ETriggerEvent::Completed, this, &AMFCharacter::HandlePickCompleted);
		}
		if (RotateCameraAction)
		{
			EI->BindAction(RotateCameraAction, ETriggerEvent::Started, this, &AMFCharacter::HandleCameraRotate);
		}
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void AMFCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, CameraSpringArm->GetRelativeRotation().Yaw, 0.f);
	const FVector  ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector  RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (!FMath::IsNearlyZero(MoveInput.X)) AddMovementInput(RightDir,   MoveInput.X);
	if (!FMath::IsNearlyZero(MoveInput.Y)) AddMovementInput(ForwardDir,  MoveInput.Y);
}

void AMFCharacter::HandlePickStarted()  { CharacterState.bIsPicking = true;  }
void AMFCharacter::HandlePickCompleted(){ CharacterState.bIsPicking = false; }

void AMFCharacter::HandleCameraRotate(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->SnapCamera(Value.Get<float>() >= 0.f ? 1 : -1);
	}
}

// ---------------------------------------------------------------------------
// Per-frame updates
// ---------------------------------------------------------------------------

void AMFCharacter::UpdateCharacterAction()
{
	if (CharacterState.bIsPicking)
	{
		CharacterState.CurrentAction = EMFCharacterAction::Pick;
		return;
	}

	const FVector2D Vel2D(GetVelocity().X, GetVelocity().Y);

	if (Vel2D.SizeSquared() > SMALL_NUMBER)
	{
		CharacterState.CurrentAction = EMFCharacterAction::Walk;

		if (FMath::Abs(Vel2D.X) >= FMath::Abs(Vel2D.Y))
			CharacterState.FacingDirection = Vel2D.X > 0.f ? EMFFacingDirection::Right : EMFFacingDirection::Left;
		else
			CharacterState.FacingDirection = Vel2D.Y > 0.f ? EMFFacingDirection::Up : EMFFacingDirection::Down;
	}
	else
	{
		CharacterState.CurrentAction = EMFCharacterAction::Idle;
	}
}

void AMFCharacter::UpdateAnimation()
{
	if (!AnimationComponent) return;

	UMFPlayerAnimInstance* AI = Cast<UMFPlayerAnimInstance>(AnimationComponent->GetAnimInstance());
	if (!AI) return;

	AI->Speed            = GetVelocity().Size2D();
	AI->bIsPicking       = CharacterState.bIsPicking;
	AI->CameraRelativeDir = GetCameraRelativeDir();
}

void AMFCharacter::UpdateBillboard()
{
	FVector ToCam = CameraComponent->GetComponentLocation() - GetActorLocation();
	ToCam.Z = 0.f;
	if (ToCam.IsNearlyZero()) return;
	ToCam.Normalize();

	FRotator BillRot = ToCam.ToOrientationRotator();
	BillRot.Pitch = 0.f;
	BillRot.Roll  = 0.f;
	BillRot.Yaw  += BillboardYawOffset;
}

// ---------------------------------------------------------------------------
// Camera-relative direction helpers
// ---------------------------------------------------------------------------

EMFCameraRelativeDir AMFCharacter::GetCameraRelativeDir() const
{
	if (!CameraController) return EMFCameraRelativeDir::Front;

	const float Rel = FMath::UnwindDegrees(GetFacingYaw(CharacterState.FacingDirection)
	                                        - CameraController->GetSpriteOrientationYaw());

	if (Rel > -45.f  && Rel <=  45.f) return EMFCameraRelativeDir::Front;
	if (Rel >  45.f  && Rel <= 135.f) return EMFCameraRelativeDir::Left;
	if (Rel > -135.f && Rel <= -45.f) return EMFCameraRelativeDir::Right;
	return EMFCameraRelativeDir::Back;
}

float AMFCharacter::GetFacingYaw(EMFFacingDirection Dir)
{
	switch (Dir)
	{
	case EMFFacingDirection::Right: return   0.f;
	case EMFFacingDirection::Up:    return  90.f;
	case EMFFacingDirection::Left:  return 180.f;
	case EMFFacingDirection::Down:  return 270.f;
	default:                        return   0.f;
	}
}
