// Copyright ProjectMF. All Rights Reserved.

#include "MFAICharacter.h"
#include "MFAIConfig.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFCombatAttributeSet.h"
#include "MFGameplayAbilityBase.h"
#include "MFOverheadWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Components/CapsuleComponent.h"

// Defined in MFCharacterBase.cpp — controls the MF.Char.AttributeDebug CVar.
extern int32 GAttributeDebug;

AMFAICharacter::AMFAICharacter()
{
	// AI characters auto-possess an AIController when placed or spawned.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// --- GAS: combat attributes (Attack, Defense, FleeThreshold) ---
	CombatAttributeSet = CreateDefaultSubobject<UMFCombatAttributeSet>(TEXT("CombatAttributeSet"));

	// --- Overhead Widget (Screen Space) ---
	OverheadWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComp->SetupAttachment(RootComponent);
	OverheadWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComp->SetDrawAtDesiredSize(true);
	OverheadWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 雷达感知组件：默认感知半径和扫描频率由组件 CDO 提供，
	// 子类或编辑器中可按需覆盖（如 BP_MFPet 配置不同的感知半径）。
	RadarSensingComp = CreateDefaultSubobject<UMFRadarSensingComponent>(TEXT("RadarSensingComp"));

	// 索敌组件：依赖 RadarSensingComp，BeginPlay 中自动绑定其事件。
	ThreatComp = CreateDefaultSubobject<UMFThreatComponent>(TEXT("ThreatComp"));
}

void AMFAICharacter::BeginPlay()
{
	// 关卡直接摆放的 AI 可在 Blueprint 中设置 AIConfig，由此处在 InitASC 之前注入。
	// Manager 生成的 AI（AIConfig = null）在 Spawn 后由 ApplyAIConfig() 注入，此处跳过。
	if (AIConfig)
	{
		DefaultAbilities  = AIConfig->DefaultAbilities;
		DefaultOwnedTags  = AIConfig->DefaultOwnedTags;
		InitAttributes    = AIConfig->InitAttributes;
		HitFlashDuration  = AIConfig->HitFlashDuration;
		OverheadWidgetClass   = AIConfig->OverheadWidgetClass;
		OverheadWidgetZOffset = AIConfig->OverheadWidgetZOffset;
	}

	Super::BeginPlay();

	// 关卡直接摆放路径：AIConfig 已在上方将 OverheadWidgetClass 写入成员，此处初始化 Widget。
	// Manager 生成路径：OverheadWidgetClass 此时为 null，Widget 由 ApplyAIConfig() 初始化。
	OverheadWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, OverheadWidgetZOffset));
	if (OverheadWidgetClass)
	{
		OverheadWidgetComp->SetWidgetClass(OverheadWidgetClass);
		OverheadWidgetComp->InitWidget();
		if (UMFOverheadWidget* W = Cast<UMFOverheadWidget>(OverheadWidgetComp->GetUserWidgetObject()))
		{
			W->InitWithASC(AbilitySystemComponent);
		}
	}
}

// ---------------------------------------------------------------------------
// Runtime config injection
// ---------------------------------------------------------------------------

void AMFAICharacter::ApplyAIConfig(const UMFAIConfig* Config)
{
	if (!Config) return;

	// --- GAS (additive, safe post-BeginPlay) ---
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TSubclassOf<UMFGameplayAbilityBase>& AbilityClass : Config->DefaultAbilities)
		{
			if (AbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
			}
		}

		ApplyAttributeInitData(Config->InitAttributes);

		if (!Config->DefaultOwnedTags.IsEmpty())
		{
			ASC->AddLooseGameplayTags(Config->DefaultOwnedTags);
		}
	}

	// --- Combat ---
	HitFlashDuration = Config->HitFlashDuration;

	// --- Overhead Widget ---
	OverheadWidgetZOffset = Config->OverheadWidgetZOffset;
	OverheadWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, OverheadWidgetZOffset));
	if (Config->OverheadWidgetClass)
	{
		OverheadWidgetComp->SetWidgetClass(Config->OverheadWidgetClass);
		OverheadWidgetComp->InitWidget();
		if (UMFOverheadWidget* W = Cast<UMFOverheadWidget>(OverheadWidgetComp->GetUserWidgetObject()))
		{
			W->InitWithASC(AbilitySystemComponent);
		}
	}
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

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void AMFAICharacter::DrawDebug() const
{
	Super::DrawDebug();

#if ENABLE_DRAW_DEBUG
	if (!GAttributeDebug || !CombatAttributeSet) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const float CapsuleTop = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 50.f;
	// Offset below the base HP/SPD lines (base draws 2–3 lines; start after them).
	const FVector Base = GetActorLocation() + FVector(0.f, 0.f, CapsuleTop + 15.f + 22.f * 3.f);

	constexpr float LineHeight = 22.f;
	constexpr float FontScale  = 1.2f;
	constexpr float Duration   = 0.f;

	int32 Line = 0;
	auto DrawLine = [&](const FString& Text, const FColor& Color)
	{
		DrawDebugString(World,
			Base + FVector(0.f, 0.f, LineHeight * Line),
			Text, nullptr, Color, Duration, /*bDrawShadow=*/true, FontScale);
		++Line;
	};

	const FColor OrangeColor(255, 165, 0);
	DrawLine(FString::Printf(TEXT("ATK %.0f  DEF %.0f"),
		CombatAttributeSet->GetAttack(),
		CombatAttributeSet->GetDefense()), OrangeColor);

	DrawLine(FString::Printf(TEXT("Flee %.0f%%"),
		CombatAttributeSet->GetFleeThreshold() * 100.f), FColor::White);
#endif
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
