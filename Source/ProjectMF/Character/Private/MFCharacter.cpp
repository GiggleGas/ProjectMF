// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "MFCamera.h"
#include "MFGameplayTags.h"
#include "MFInventoryComponent.h"
#include "MFGameMode.h"
#include "MFLog.h"
#include "MFPlayerConfig.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
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
	// 在 Super::BeginPlay 之前将 Config 值写入基类属性，
	// 这样 InitAbilitySystemComponent() 和头顶 Widget 初始化能正确读取。
	if (PlayerConfig)
	{
		DefaultAbilities      = PlayerConfig->DefaultAbilities;
		DefaultOwnedTags      = PlayerConfig->DefaultOwnedTags;
		DefaultInitEffect     = PlayerConfig->DefaultInitEffect;
		HitFlashDuration      = PlayerConfig->HitFlashDuration;
		OverheadWidgetClass   = PlayerConfig->OverheadWidgetClass;
		OverheadWidgetZOffset = PlayerConfig->OverheadWidgetZOffset;
	}

	Super::BeginPlay();
}

void AMFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerConfig)
	{
		MF_LOG_WARNING(LogMFCharacter, TEXT("AMFCharacter: PlayerConfig is not set — no input bindings applied."));
		return;
	}

	if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (PlayerConfig->MoveAction)
		{
			EI->BindAction(PlayerConfig->MoveAction, ETriggerEvent::Triggered, this, &AMFCharacter::HandleMove);
		}
		if (PlayerConfig->PickAction)
		{
			EI->BindAction(PlayerConfig->PickAction, ETriggerEvent::Started,   this, &AMFCharacter::HandlePickStarted);
			EI->BindAction(PlayerConfig->PickAction, ETriggerEvent::Completed, this, &AMFCharacter::HandlePickCompleted);
		}
		if (PlayerConfig->RotateCameraAction)
		{
			EI->BindAction(PlayerConfig->RotateCameraAction, ETriggerEvent::Started, this, &AMFCharacter::HandleCameraRotate);
		}
		if (PlayerConfig->CatchPetAction)
		{
			EI->BindAction(PlayerConfig->CatchPetAction, ETriggerEvent::Completed, this, &AMFCharacter::HandleCatchPet);
		}
		else
		{
			MF_LOG_WARNING(LogMFCharacter, TEXT("AMFCharacter: PlayerConfig->CatchPetAction is not set — catch ability cannot be activated via input."));
		}

		// DEMO BEGIN — 召唤按键临时绑定（1-5 对应 slot 1-5）
		// TODO: GA_PetWheel 完成后，删除此段并改为激活轮盘 Ability
		for (int32 i = 0; i < PlayerConfig->SummonSlotActions.Num() && i < 5; ++i)
		{
			if (PlayerConfig->SummonSlotActions[i])
			{
				EI->BindAction(PlayerConfig->SummonSlotActions[i], ETriggerEvent::Started,
					this, &AMFCharacter::HandleSummonSlot, i + 1);
			}
		}
		// DEMO END

		if (PlayerConfig->StartBossBattleAction)
		{
			EI->BindAction(PlayerConfig->StartBossBattleAction, ETriggerEvent::Started,
				this, &AMFCharacter::HandleStartBossBattle);
		}
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

void AMFCharacter::HandleStartBossBattle()
{
	if (AMFGameMode* GM = Cast<AMFGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RequestBossPhase();
	}
}
