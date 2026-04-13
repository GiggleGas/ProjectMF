// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "MFCamera.h"
#include "MFGameplayTags.h"
#include "MFInventoryComponent.h"
#include "MFLog.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

AMFCharacter::AMFCharacter()
{
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

	// --- Inventory ---
	InventoryComponent = CreateDefaultSubobject<UMFInventoryComponent>(TEXT("InventoryComponent"));
}

void AMFCharacter::BeginPlay()
{
	Super::BeginPlay();
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
		if (CatchPetAction)
		{
			// Completed = 按键松开后触发，确保不与长按走路等操作冲突
			EI->BindAction(CatchPetAction, ETriggerEvent::Completed, this, &AMFCharacter::HandleCatchPet);
		}
		else
		{
			MF_LOG_WARNING(LogMFCharacter, TEXT("AMFCharacter: CatchPetAction is not set — catch ability cannot be activated via input."));
		}

		// DEMO BEGIN — 召唤按键临时绑定（1-5 对应 slot 1-5）
		// TODO: GA_PetWheel 完成后，删除此段并改为激活轮盘 Ability
		for (int32 i = 0; i < SummonSlotActions.Num() && i < 5; ++i)
		{
			if (SummonSlotActions[i])
			{
				EI->BindAction(SummonSlotActions[i], ETriggerEvent::Started,
					this, &AMFCharacter::HandleSummonSlot, i + 1);
			}
		}
		// DEMO END
	}
}

// ---------------------------------------------------------------------------
// Camera accessors
// ---------------------------------------------------------------------------

bool AMFCharacter::GetBillboardCameraForward(FVector& OutForward) const
{
	if (!CameraComponent) return false;
	OutForward = CameraComponent->GetForwardVector();
	return true;
}

float AMFCharacter::GetCameraYawForDirectionality() const
{
	return CameraController ? CameraController->GetSpriteOrientationYaw() : 0.f;
}

// ---------------------------------------------------------------------------
// Input handlers
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

void AMFCharacter::HandlePickStarted()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(MFGameplayTags::Ability_Pick));
	}
}

void AMFCharacter::HandlePickCompleted()
{
	if (AbilitySystemComponent)
	{
		const FGameplayTagContainer PickTag(MFGameplayTags::Ability_Pick);
		AbilitySystemComponent->CancelAbilities(&PickTag);
	}
}

void AMFCharacter::HandleCameraRotate(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->SnapCamera(Value.Get<float>() >= 0.f ? 1 : -1);
	}
}

void AMFCharacter::HandleCatchPet()
{
	if (!AbilitySystemComponent)
	{
		MF_LOG_ERROR(LogMFCharacter, TEXT("AMFCharacter::HandleCatchPet — AbilitySystemComponent is null!"));
		return;
	}

	MF_LOG(LogMFCharacter, TEXT("AMFCharacter: CatchPet key released — trying to activate MF.Ability.CatchPet."));

	const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(
		FGameplayTagContainer(MFGameplayTags::Ability_CatchPet));

	if (!bActivated)
	{
		MF_LOG_WARNING(LogMFCharacter,
			TEXT("AMFCharacter: TryActivateAbilitiesByTag(CatchPet) returned false. "
			     "Check that GA_CatchPet is in DefaultAbilities and CatchConfig is assigned."));
	}
}

void AMFCharacter::HandleSummonSlot(int32 SlotIndex)
{
	if (!AbilitySystemComponent) return;

	FGameplayEventData EventData;
	EventData.EventMagnitude = static_cast<float>(SlotIndex);
	AbilitySystemComponent->HandleGameplayEvent(MFGameplayTags::Ability_SummonPet, &EventData);
}
