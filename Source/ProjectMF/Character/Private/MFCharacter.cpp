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
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

// ---------------------------------------------------------------------------
// CVar: CharacterBaseDebug  ——  运行时开关 debug 可视化
// 控制台输入: CharacterBaseDebug 1  开启 / CharacterBaseDebug 0  关闭
// ---------------------------------------------------------------------------
static int32 GCharacterBaseDebug = 0;
static FAutoConsoleVariableRef CVarCharacterBaseDebug(
	TEXT("MF.Char.CharacterBaseDebug"),
	GCharacterBaseDebug,
	TEXT("Enable MFCharacter debug visualization. 1 = on, 0 = off."),
	ECVF_Default
);

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
	UpdateBillboard();
	DrawDebug();
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

	const FVector Vel = GetVelocity();
	const FVector2D Vel2D(Vel.X, Vel.Y);

	if (Vel2D.SizeSquared() > SMALL_NUMBER)
	{
		CharacterState.CurrentAction   = EMFCharacterAction::Walk;
		CharacterState.LastVelocityDir = Vel2D.GetSafeNormal();
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

	AI->Speed           = GetVelocity().Size2D();
	AI->bIsPicking      = CharacterState.bIsPicking;
	AI->DirectionalInput = GetDirectionalInput();
}

void AMFCharacter::UpdateBillboard()
{
	if (!FlipbookComponent || !CameraComponent) return;

	// Full 3D vector from sprite to camera.
	FVector ToCam = CameraComponent->GetComponentLocation() - GetActorLocation();
	if (ToCam.IsNearlyZero()) return;
	ToCam.Normalize();

	// Build a rotation matrix where local +Y (Paper2D sprite normal) points exactly
	// toward the camera — i.e., the sprite plane is perpendicular to the camera ray.
	// World +Z is used as the "up" reference so the sprite doesn't spin unexpectedly.
	const FRotator BillRot = FRotationMatrix::MakeFromYZ(ToCam, FVector::UpVector).Rotator();
	FlipbookComponent->SetWorldRotation(BillRot);
}

// ---------------------------------------------------------------------------
// Directional input for PaperZD SetDirectionality node
// ---------------------------------------------------------------------------

FVector2D AMFCharacter::GetDirectionalInput() const
{
	// Use current velocity direction; fall back to last known when idle.
	const FVector2D FacingDir = CharacterState.LastVelocityDir;
	const FVector   FacingWorld(FacingDir.X, FacingDir.Y, 0.f);

	// Unrotate world-space facing by the camera's sprite orientation yaw so the
	// result is expressed in camera space.  SpriteOrientationYaw only advances at
	// canonical 90° snaps, keeping the sprite stable at 45° intermediate positions.
	const float CameraYaw = CameraController ? CameraController->GetSpriteOrientationYaw() : 0.f;
	const FVector RelFacing = FRotator(0.f, -CameraYaw, 0.f).RotateVector(FacingWorld);

	// Map to SetDirectionality's 2D convention:
	//   RelFacing.X  = camera-forward component  →  DirectionalInput.Y  (0° axis = away from camera)
	//   RelFacing.Y  = camera-right  component   →  DirectionalInput.X  (90° axis)
	return FVector2D(RelFacing.Y, RelFacing.X);
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void AMFCharacter::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (!GCharacterBaseDebug) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();
	constexpr float ArrowLen   = 80.f;
	constexpr float ArrowSize  = 10.f;
	constexpr float Thickness  = 2.f;
	constexpr float LifeTime   = -1.f; // 单帧

	// --- 最后朝向箭头（黄色） ---
	const FVector2D LastDir = CharacterState.LastVelocityDir;
	const FVector FacingDir(LastDir.X, LastDir.Y, 0.f);
	DrawDebugDirectionalArrow(
		World,
		Origin,
		Origin + FacingDir * ArrowLen,
		ArrowSize, FColor::Yellow, false, LifeTime, 0, Thickness);

	// --- 当前速度箭头（青色） ---
	const FVector Vel3D = GetVelocity();
	const FVector Vel2D(Vel3D.X, Vel3D.Y, 0.f);
	if (!Vel2D.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Origin,
			Origin + Vel2D.GetSafeNormal() * ArrowLen,
			ArrowSize, FColor::Cyan, false, LifeTime, 0, Thickness);
	}

	// --- 屏幕左上角：DirectionalInput & 速度分量 ---
	if (GEngine)
	{
		const FVector2D DI = GetDirectionalInput();
		GEngine->AddOnScreenDebugMessage(
			43, 0.f, FColor::Yellow,
			FString::Printf(TEXT("DirInput: (%.2f, %.2f)"), DI.X, DI.Y));

		GEngine->AddOnScreenDebugMessage(
			41, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Vel Y  : %.1f"), Vel3D.Y));

		GEngine->AddOnScreenDebugMessage(
			40, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Vel X  : %.1f"), Vel3D.X));
	}
#endif // ENABLE_DRAW_DEBUG
}
