// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacterBase.h"
#include "MFHitReactInterface.h"
#include "MFAnimInstanceBase.h"
#include "MFAttributeSetBase.h"
#include "MFCombatAttributeSet.h"
#include "MFGameplayAbilityBase.h"
#include "MFGameplayTags.h"
#include "MFDamageNumberWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "MFSpriteVisualComponent.h"
#include "MFLog.h"
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

// Non-static so AMFAICharacter can extern-reference it for combat attribute debug rendering.
int32 GAttributeDebug = 0;
static FAutoConsoleVariableRef CVarAttributeDebug(
	TEXT("MF.Char.AttributeDebug"),
	GAttributeDebug,
	TEXT("Render GAS attribute values as world-space text above each character. 1 = on, 0 = off."),
	ECVF_Default
);

// 友方头顶绿点（区分敌我），默认开启。
static int32 GFriendlyMarker = 1;
static FAutoConsoleVariableRef CVarFriendlyMarker(
	TEXT("MF.Char.FriendlyMarker"),
	GFriendlyMarker,
	TEXT("Draw a green dot above friendly (MF.Team.Player) characters' heads. 1 = on (default), 0 = off."),
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

	// --- Flipbook (render target driven by PaperZD) ---
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(RootComponent);

	// --- PaperZD Animation ---
	// Set the AnimBP class in the derived Blueprint.
	AnimationComponent = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("AnimationComponent"));
	AnimationComponent->InitRenderComponent(FlipbookComponent);

	// --- 2D 表现能力组件（S1 引入，S2 起接管 billboard/闪光/碰撞） ---
	SpriteVisual = CreateDefaultSubobject<UMFSpriteVisualComponent>(TEXT("SpriteVisual"));
}

void AMFCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystemComponent();

	// 绑定死亡委托：AttributeSet 的 OnDeath → HandleDeath（虚函数，派生类可覆盖）
	if (AttributeSet)
	{
		AttributeSet->OnDeath.AddUObject(this, &AMFCharacterBase::HandleDeath);
		AttributeSet->OnHealthChanged.AddUObject(this, &AMFCharacterBase::OnHealthChangedCallback);
	}

	// 2D 表现组件绑定（须在碰撞拟合前——UpdateCollisionFromFlipbook 已转发给组件）：
	// 目标 Flipbook + 碰撞胶囊；相机源复用子类已实现的 GetBillboardCameraForward
	// （玩家=相机前向，AI=PlayerCameraManager）；组件接管每帧 billboard。
	if (SpriteVisual)
	{
		SpriteVisual->InitVisual(FlipbookComponent, GetCapsuleComponent());
		SpriteVisual->bDriveBillboardOnTick = true;

		TWeakObjectPtr<AMFCharacterBase> WeakThis(this);
		SpriteVisual->SetCameraForwardProvider([WeakThis](FVector& OutForward) -> bool
		{
			return WeakThis.IsValid() ? WeakThis->GetBillboardCameraForward(OutForward) : false;
		});
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

	// MoveSpeed → MaxWalkSpeed：先注册属性变化回调（覆盖后续 slow/缠绕/buff 对移速的改动）。
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UMFAttributeSetBase::GetMoveSpeedAttribute())
		.AddUObject(this, &AMFCharacterBase::OnMoveSpeedChanged);

	// 眩晕标签变化 → 禁动 / 打断 / 恢复。
	AbilitySystemComponent->RegisterGameplayTagEvent(
		MFGameplayTags::State_Stunned, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AMFCharacterBase::OnStunnedTagChanged);

	// 应用 GAS 配置（初始属性 / 授予技能 / 阵营标签）——由子类从各自 Config 实现。
	// 在属性变化/眩晕回调注册之后、OnDeath/OnHealthChanged 绑定之前（见 BeginPlay）执行，
	// 避免初始化血量时误触发受伤 / 治疗回调。
	ApplyGASConfig();
}

void AMFCharacterBase::ApplyAttributeInitData(const FMFAttributeInitData& Data)
{
	if (!AbilitySystemComponent) return;

	// 基础集（每个角色都挂载 UMFAttributeSetBase）。
	// MaxHealth 须先于 Health 写入：PreAttributeChange 会把 Health 夹紧到 [0, MaxHealth]。
	AbilitySystemComponent->SetNumericAttributeBase(UMFAttributeSetBase::GetMaxHealthAttribute(), Data.MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UMFAttributeSetBase::GetHealthAttribute(),    Data.MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UMFAttributeSetBase::GetMoveSpeedAttribute(), Data.MoveSpeed);

	// 战斗集仅 AI/宠物/Boss 挂载；玩家无此集，跳过攻防/逃跑阈值。
	if (AbilitySystemComponent->GetSet<UMFCombatAttributeSet>())
	{
		AbilitySystemComponent->SetNumericAttributeBase(UMFCombatAttributeSet::GetAttackAttribute(),        Data.Attack);
		AbilitySystemComponent->SetNumericAttributeBase(UMFCombatAttributeSet::GetDefenseAttribute(),       Data.Defense);
		AbilitySystemComponent->SetNumericAttributeBase(UMFCombatAttributeSet::GetFleeThresholdAttribute(), Data.FleeThreshold);
	}

	// 同步一次 MaxWalkSpeed，确保初始移速即时生效（移速属性回调通常也会同步）。
	if (AttributeSet)
	{
		if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		{
			CMC->MaxWalkSpeed = AttributeSet->GetMoveSpeed() * MoveSpeedMultiplier;
		}
	}
}

void AMFCharacterBase::GrantAbility(TSubclassOf<UMFGameplayAbilityBase> AbilityClass, bool bAutoRelease)
{
	if (!AbilityClass || !AbilitySystemComponent) return;

	FGameplayAbilitySpec Spec(AbilityClass, 1);
	if (bAutoRelease)
	{
		// 动态 spec 标签标记"自动释放"；无此标签即手动。STCond_CanAutoUseSkill 据此判定。
		Spec.GetDynamicSpecSourceTags().AddTag(MFGameplayTags::SkillMode_Auto);
	}
	AbilitySystemComponent->GiveAbility(Spec);
}

void AMFCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = Data.NewValue * MoveSpeedMultiplier;
	}
}

void AMFCharacterBase::SetMoveSpeedMultiplier(float InMultiplier)
{
	MoveSpeedMultiplier = FMath::Max(InMultiplier, 0.f);
	if (AttributeSet)
	{
		if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		{
			CMC->MaxWalkSpeed = AttributeSet->GetMoveSpeed() * MoveSpeedMultiplier;
		}
	}
}

void AMFCharacterBase::OnStunnedTagChanged(const FGameplayTag /*CallbackTag*/, int32 NewCount)
{
	if (!AbilitySystemComponent) return;

	if (NewCount > 0)
	{
		// 进入眩晕：禁止移动 + 打断当前技能（技能再激活由 ActivationBlockedTags 拦截）。
		GetCharacterMovement()->DisableMovement();
		AbilitySystemComponent->CancelAllAbilities();
	}
	else if (!AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
	{
		// 解除眩晕且未死亡：恢复行走（MaxWalkSpeed 由 MoveSpeed 同步维持）。
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
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
	// billboard 由 UMFSpriteVisualComponent 接管（BeginPlay 已 bDriveBillboardOnTick=true）。
	DrawDebug();
	DrawFriendlyMarker();
}

// ---------------------------------------------------------------------------
// Shared per-frame logic
// ---------------------------------------------------------------------------

void AMFCharacterBase::UpdateCharacterAction()
{
	// Derive bIsPicking from the live GAS tag rather than a raw flag.
	// GA_Pick sets MF.GameplayState.Picking via ActivationOwnedTags while active.
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

// ---------------------------------------------------------------------------
// 表现能力 exec —— 手动触发组件三块能力（S1 起验证 / 常态调试）
// ---------------------------------------------------------------------------

void AMFCharacterBase::MFSVBillboard()
{
#if !UE_BUILD_SHIPPING
	if (SpriteVisual)
	{
		SpriteVisual->TickBillboard();
		MF_LOG(LogMFVisual, TEXT("[%s] MFSVBillboard：已触发组件 billboard（开 MF.SpriteVisual.Debug 1 看朝向箭头/日志）。"), *GetName());
	}
#endif
}

void AMFCharacterBase::MFSVFlash()
{
#if !UE_BUILD_SHIPPING
	if (SpriteVisual)
	{
		SpriteVisual->FlashColor(FLinearColor::Blue, 0.5f);
		MF_LOG(LogMFVisual, TEXT("[%s] MFSVFlash：闪蓝 0.5s（应自动复位白色）。"), *GetName());
	}
#endif
}

void AMFCharacterBase::MFSVCollision()
{
#if !UE_BUILD_SHIPPING
	if (SpriteVisual)
	{
		SpriteVisual->FitCollisionToFlipbook(CollisionRadiusScale);
		MF_LOG(LogMFVisual, TEXT("[%s] MFSVCollision：已触发组件碰撞拟合（比对 Radius 日志与现有一致）。"), *GetName());
	}
#endif
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
	// 实现已抽入 UMFSpriteVisualComponent（S2）；保留本虚函数签名，转发给组件。
	if (SpriteVisual)
	{
		SpriteVisual->FitCollisionToFlipbook(CollisionRadiusScale);
	}
}

// ---------------------------------------------------------------------------
// Hit React
// ---------------------------------------------------------------------------

void AMFCharacterBase::OnHealthChangedCallback(float OldHealth, float NewHealth)
{
	const float Delta = NewHealth - OldHealth;

	if (Delta < 0.f)
	{
		ReactToHit_Implementation();   // 掉血 → 闪红
	}
	else if (Delta > 0.f)
	{
		ReactToHeal();                 // 回血 → 闪绿
	}

	// 战斗飘字：伤害红字 / 治疗绿字（从角色头顶弹出）
	if (!FMath::IsNearlyZero(Delta))
	{
		UMFDamageNumberWidget::ShowDamageNumber(this, GetActorLocation(), FMath::Abs(Delta), Delta > 0.f);
	}
}

void AMFCharacterBase::ReactToHit_Implementation()
{
	// 闪光实现已抽入 UMFSpriteVisualComponent（S2）；时长仍由各 Config 写入的 HitFlashDuration 控制。
	if (SpriteVisual)
	{
		SpriteVisual->FlashColor(FLinearColor::Red, HitFlashDuration);
	}
}

void AMFCharacterBase::ReactToHeal()
{
	if (SpriteVisual)
	{
		SpriteVisual->FlashColor(FLinearColor::Green, HitFlashDuration);
	}
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

void AMFCharacterBase::DrawFriendlyMarker() const
{
#if ENABLE_DRAW_DEBUG
	if (!GFriendlyMarker) return;

	// 仅友方（持 Team.Player）：召唤宠 + 玩家。敌方/中立不画。
	if (!AbilitySystemComponent ||
		!AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::Team_Player))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World) return;

	const float HeadZ = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 80.f;
	const FVector DotLoc = GetActorLocation() + FVector(0.f, 0.f, HeadZ + 30.f);

	// 单帧（每 Tick 重画）的绿色实心点。
	DrawDebugPoint(World, DotLoc, /*Size=*/18.f, FColor::Green, /*bPersistent=*/false, /*LifeTime=*/-1.f);
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
