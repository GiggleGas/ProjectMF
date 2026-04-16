// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacterBase.h"
#include "MFAnimInstanceBase.h"
#include "MFAttributeSetBase.h"
#include "MFCombatAttributeSet.h"
#include "MFGameplayAbilityBase.h"
#include "MFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "PaperFlipbookComponent.h"
#include "PaperZDAnimationComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

// ---------------------------------------------------------------------------
// CVars
// 控制台输入示例:
//   MF.Char.CharacterBaseDebug 1   → 开启箭头/碰撞球
//   MF.Char.AttributeDebug 1       → 开启角色头顶属性面板
// ---------------------------------------------------------------------------
static int32 GCharacterBaseDebug = 0;
static FAutoConsoleVariableRef CVarCharacterBaseDebug(
	TEXT("MF.Char.CharacterBaseDebug"),
	GCharacterBaseDebug,
	TEXT("Enable MFCharacterBase debug visualization (arrows, collision sphere). 1 = on, 0 = off."),
	ECVF_Default
);

static int32 GAttributeDebug = 0;
static FAutoConsoleVariableRef CVarAttributeDebug(
	TEXT("MF.Char.AttributeDebug"),
	GAttributeDebug,
	TEXT("Render GAS attribute values as world-space text above each character. 1 = on, 0 = off."),
	ECVF_Default
);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AMFCharacterBase::AMFCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- GAS ---
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet           = CreateDefaultSubobject<UMFAttributeSetBase>(TEXT("AttributeSet"));
	CombatAttributeSet     = CreateDefaultSubobject<UMFCombatAttributeSet>(TEXT("CombatAttributeSet"));

	// --- Flipbook (render target driven by PaperZD) ---
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(RootComponent);

	// --- PaperZD Animation ---
	// Set the AnimBP class in the derived Blueprint.
	AnimationComponent = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("AnimationComponent"));
	AnimationComponent->InitRenderComponent(FlipbookComponent);
}

void AMFCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystemComponent();

	// 绑定死亡委托：AttributeSet 的 OnDeath → HandleDeath（虚函数，派生类可覆盖）
	if (AttributeSet)
	{
		AttributeSet->OnDeath.AddUObject(this, &AMFCharacterBase::HandleDeath);
	}

	if (bAutoUpdateCollisionFromFlipbook)
	{
		UpdateCollisionFromFlipbook();
	}
}

// ---------------------------------------------------------------------------
// IAbilitySystemInterface
// ---------------------------------------------------------------------------

UAbilitySystemComponent* AMFCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ---------------------------------------------------------------------------
// GAS initialization
// ---------------------------------------------------------------------------

void AMFCharacterBase::InitAbilitySystemComponent()
{
	if (!AbilitySystemComponent) return;

	// Owner and Avatar are both this actor (no separate PlayerState for now).
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Apply default init effect (sets MaxHealth, Attack, Defense, etc.)
	// Must come before granting abilities so attribute values are ready.
	if (DefaultInitEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec =
			AbilitySystemComponent->MakeOutgoingSpec(DefaultInitEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}

	// Grant every ability listed in DefaultAbilities at level 1.
	for (const TSubclassOf<UMFGameplayAbilityBase>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

void AMFCharacterBase::HandleDeath()
{
	if (!AbilitySystemComponent) return;

	// 防止重复触发（已有 State.Dead 时直接返回）
	if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
	{
		return;
	}

	// 打上死亡 Tag（阻断后续能力激活，StateTree 可监听此 Tag 切换死亡状态）
	AbilitySystemComponent->AddLooseGameplayTag(MFGameplayTags::State_Dead);

	// 取消所有进行中的能力
	AbilitySystemComponent->CancelAllAbilities();

	// 停止移动
	GetCharacterMovement()->DisableMovement();
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AMFCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCharacterAction();
	UpdateAnimation();
	UpdateBillboard();
	DrawDebug();
}

// ---------------------------------------------------------------------------
// Shared per-frame logic
// ---------------------------------------------------------------------------

void AMFCharacterBase::UpdateCharacterAction()
{
	// Derive bIsPicking from the live GAS tag rather than a raw flag.
	// GA_Pick sets MF.Character.State.Picking via ActivationOwnedTags while active.
	CharacterState.bIsPicking = AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_Picking);

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

void AMFCharacterBase::UpdateAnimation()
{
	if (!AnimationComponent) return;

	UMFAnimInstanceBase* AI = Cast<UMFAnimInstanceBase>(AnimationComponent->GetAnimInstance());
	if (!AI) return;

	AI->Speed            = GetVelocity().Size2D();
	AI->bIsPicking       = CharacterState.bIsPicking;
	AI->DirectionalInput = GetDirectionalInput();
}

void AMFCharacterBase::UpdateBillboard()
{
	if (!FlipbookComponent) return;

	FVector CamForward;
	if (!GetBillboardCameraForward(CamForward)) return;

	// All sprites use the same direction: opposite of where the camera is looking.
	// This gives a uniform tilt across the entire scene (no per-object angle variation).
	const FVector ToCam = -CamForward;

	// Build a rotation where local +Y (Paper2D sprite normal) points toward the camera.
	const FRotator BillRot = FRotationMatrix::MakeFromYZ(ToCam, FVector::UpVector).Rotator();
	FlipbookComponent->SetWorldRotation(BillRot);
}

// ---------------------------------------------------------------------------
// Directional input for PaperZD SetDirectionality node
// ---------------------------------------------------------------------------

FVector2D AMFCharacterBase::GetDirectionalInput() const
{
	// Use current velocity direction; fall back to last known when idle.
	const FVector2D FacingDir = CharacterState.LastVelocityDir;
	const FVector   FacingWorld(FacingDir.X, FacingDir.Y, 0.f);

	// Unrotate world-space facing by the camera's sprite orientation yaw.
	const float  CameraYaw = GetCameraYawForDirectionality();
	const FVector RelFacing = FRotator(0.f, -CameraYaw, 0.f).RotateVector(FacingWorld);

	// Map to SetDirectionality's 2D convention:
	//   RelFacing.X  = camera-forward component  →  DirectionalInput.Y
	//   RelFacing.Y  = camera-right  component   →  DirectionalInput.X
	return FVector2D(RelFacing.Y, RelFacing.X);
}

// ---------------------------------------------------------------------------
// Collision fitting
// ---------------------------------------------------------------------------

void AMFCharacterBase::UpdateCollisionFromFlipbook()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule || !FlipbookComponent) return;

	const UPaperFlipbook* Flipbook = FlipbookComponent->GetFlipbook();
	if (!Flipbook || Flipbook->GetNumKeyFrames() == 0) return;

	const UPaperSprite* Sprite = Flipbook->GetKeyFrameChecked(0).Sprite;
	if (!Sprite) return;

	const float PixelsPerUnit = Sprite->GetPixelsPerUnrealUnit();
	if (PixelsPerUnit <= KINDA_SMALL_NUMBER) return;

	// SpriteWidth / PPU = world-space width; half = sphere radius.
	// Width (X) represents the character's horizontal extent — the footprint for a
	// billboard sprite in a top-down 3D world.
	const FVector2D SourceSize = Sprite->GetSourceSize();
	const float SpriteWidthUnits = SourceSize.X / PixelsPerUnit;
	const float NewRadius = FMath::Max(1.f, SpriteWidthUnits * 0.5f * CollisionRadiusScale);

	// Setting HalfHeight == Radius makes the capsule geometrically identical to a sphere.
	// The bUpdateOverlaps flag triggers an immediate overlap recheck.
	Capsule->SetCapsuleSize(NewRadius, NewRadius, /*bUpdateOverlaps=*/true);

	// Shift the sprite so its local origin sits at the bottom of the collision sphere.
	// The capsule center is at the actor origin (Z=0); bottom of sphere is at Z=-Radius.
	// This aligns the visual footprint with the physics footprint.
	FlipbookComponent->SetRelativeLocation(FVector(0.f, 0.f, -NewRadius));

	UE_LOG(LogTemp, Log,
		TEXT("[%s] Collision sphere: Radius=%.1f  (sprite %.0f x %.0f px, PPU=%.2f, scale=%.2f)"),
		*GetName(), NewRadius, SourceSize.X, SourceSize.Y, PixelsPerUnit, CollisionRadiusScale);
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void AMFCharacterBase::DrawAttributeDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (!AttributeSet) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	// 文本锚点：胶囊顶部再往上一点
	const float CapsuleTop = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 50.f;
	const FVector Base = GetActorLocation() + FVector(0.f, 0.f, CapsuleTop + 15.f);

	constexpr float LineHeight = 22.f;  // 世界空间行间距（单位：cm）
	constexpr float FontScale  = 1.2f;
	constexpr float Duration   = 0.f;  // 单帧，每 Tick 刷新

	int32 Line = 0;
	auto DrawLine = [&](const FString& Text, const FColor& Color)
	{
		DrawDebugString(World,
			Base + FVector(0.f, 0.f, LineHeight * Line),
			Text, nullptr, Color, Duration, /*bDrawShadow=*/true, FontScale);
		++Line;
	};

	// --- 角色名 ---
	DrawLine(FString::Printf(TEXT("[%s]"), *GetName()), FColor::White);

	// --- Health（按血量比例变色：绿/黄/红）---
	const float HP    = AttributeSet->GetHealth();
	const float MaxHP = AttributeSet->GetMaxHealth();
	const float HPRatio = (MaxHP > 0.f) ? HP / MaxHP : 0.f;
	const FColor HPColor = (HPRatio > 0.6f) ? FColor::Green
	                     : (HPRatio > 0.3f) ? FColor::Yellow
	                                        : FColor::Red;
	DrawLine(FString::Printf(TEXT("HP  %.0f / %.0f"), HP, MaxHP), HPColor);

	// --- MoveSpeed ---
	DrawLine(FString::Printf(TEXT("SPD %.0f"), AttributeSet->GetMoveSpeed()), FColor::White);

	// --- Combat 属性（CombatAttributeSet 挂载时显示）---
	if (CombatAttributeSet)
	{
		const FColor OrangeColor(255, 165, 0);
		DrawLine(FString::Printf(TEXT("ATK %.0f  DEF %.0f"),
			CombatAttributeSet->GetAttack(),
			CombatAttributeSet->GetDefense()), OrangeColor);

		DrawLine(FString::Printf(TEXT("Flee %.0f%%"),
			CombatAttributeSet->GetFleeThreshold() * 100.f), FColor::White);
	}

	// --- 状态 Tag ---
	if (AbilitySystemComponent)
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
		{
			DrawLine(TEXT("[DEAD]"), FColor::Red);
		}
		else if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_InCombat))
		{
			DrawLine(TEXT("[IN COMBAT]"), FColor(255, 80, 80));
		}
	}
#endif
}

void AMFCharacterBase::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (GAttributeDebug)
	{
		DrawAttributeDebug();
	}

	if (!GCharacterBaseDebug) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();
	constexpr float ArrowLen  = 80.f;
	constexpr float ArrowSize = 10.f;
	constexpr float Thickness = 2.f;
	constexpr float LifeTime  = -1.f; // 单帧

	// 碰撞球（绿色）
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		DrawDebugSphere(World, Origin, Capsule->GetScaledCapsuleRadius(),
			16, FColor::Green, false, LifeTime, 0, Thickness);
	}

	// 最后朝向（黄色）
	const FVector2D LastDir = CharacterState.LastVelocityDir;
	const FVector FacingDir(LastDir.X, LastDir.Y, 0.f);
	DrawDebugDirectionalArrow(
		World, Origin, Origin + FacingDir * ArrowLen,
		ArrowSize, FColor::Yellow, false, LifeTime, 0, Thickness);

	// 当前速度（青色）
	const FVector Vel3D = GetVelocity();
	const FVector Vel2D(Vel3D.X, Vel3D.Y, 0.f);
	if (!Vel2D.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World, Origin, Origin + Vel2D.GetSafeNormal() * ArrowLen,
			ArrowSize, FColor::Cyan, false, LifeTime, 0, Thickness);
	}

	if (GEngine)
	{
		const FVector2D DI = GetDirectionalInput();
		GEngine->AddOnScreenDebugMessage(43, 0.f, FColor::Yellow,
			FString::Printf(TEXT("[%s] DirInput: (%.2f, %.2f)"), *GetName(), DI.X, DI.Y));
		GEngine->AddOnScreenDebugMessage(41, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Vel Y: %.1f"), Vel3D.Y));
		GEngine->AddOnScreenDebugMessage(40, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Vel X: %.1f"), Vel3D.X));
	}
#endif
}
