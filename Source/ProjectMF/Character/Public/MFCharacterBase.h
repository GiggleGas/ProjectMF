// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "MFCharacterTypes.h"
#include "MFHitReactInterface.h"
#include "MFAttributeInitData.h"
#include "MFCharacterBase.generated.h"

class UPaperFlipbookComponent;
class UPaperZDAnimationComponent;
class UAbilitySystemComponent;
class UMFAttributeSetBase;
class UMFGameplayAbilityBase;
class UGameplayEffect;
struct FOnAttributeChangeData;

/**
 * Abstract base class for all MF characters (player and AI).
 *
 * Owns the 2D rendering components (Flipbook + PaperZD) and the shared per-frame
 * logic: character action state machine, animation driving, and billboard alignment.
 *
 * GAS integration:
 *   - Implements IAbilitySystemInterface so the GAS subsystem can locate the ASC.
 *   - ASC and the base AttributeSet live here so every character (player and AI/pet)
 *     automatically has Health, MaxHealth, and MoveSpeed.
 *   - DefaultAbilities is configured per Blueprint; InitAbilitySystemComponent()
 *     grants them at BeginPlay.
 *   - UpdateCharacterAction() reads MF.Character.State.Picking from the ASC to set
 *     CharacterState.bIsPicking, keeping the existing PaperZD animation pipeline intact.
 *
 * Mass integration stubs live in IMFMassControllable (AI/Public/MFMassInterface.h).
 * Only AMFAICharacter implements that interface; this base stays input-agnostic.
 */
UCLASS(Abstract)
class PROJECTMF_API AMFCharacterBase : public ACharacter,
	public IAbilitySystemInterface,
	public IMFHitReactInterface
{
	GENERATED_BODY()

public:
	AMFCharacterBase();

	virtual void Tick(float DeltaTime) override;

	// -----------------------------------------------------------------------
	// IAbilitySystemInterface
	// -----------------------------------------------------------------------

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

	// -----------------------------------------------------------------------
	// GAS Components
	// -----------------------------------------------------------------------

	/** Ability System Component — owns active abilities, tags, and attribute sets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Base attribute set: Health, MaxHealth, MoveSpeed, Damage. Owned by this actor. */
	UPROPERTY()
	TObjectPtr<UMFAttributeSetBase> AttributeSet;

	/**
	 * 应用 GAS 配置（初始属性 / 授予技能 / 阵营标签 / 受击闪光时长）。
	 * 在 InitAbilitySystemComponent 内调用；基类默认空实现，各子类覆写为"从自己的 Config 应用"
	 * （玩家 → UMFPlayerConfig，AI → UMFAIConfig）。
	 * 设计：基类不再存 DefaultAbilities/OwnedTags/InitAttributes——这些一律走 Config（单一数据源）。
	 */
	virtual void ApplyGASConfig() {}

	/**
	 * Initialize the ASC actor info + 属性变化/眩晕回调，然后调用 ApplyGASConfig()。
	 * Called from BeginPlay() — safe to call on both server and standalone.
	 */
	void InitAbilitySystemComponent();

	/**
	 * 把 InitData 写入 ASC（基础集恒写；战斗集 Attack/Defense/FleeThreshold 仅在挂载时写）
	 * 并同步一次 MaxWalkSpeed。供子类 ApplyGASConfig / ApplyAIConfig 调用。
	 */
	void ApplyAttributeInitData(const FMFAttributeInitData& Data);

	/**
	 * 授予一个技能。bAutoRelease=true 时给 ability spec 打动态标签 MF.SkillMode.Auto
	 * （供 STCond_CanAutoUseSkill 判定；默认手动）。供子类 ApplyGASConfig / ApplyAIConfig 调用。
	 */
	void GrantAbility(TSubclassOf<UMFGameplayAbilityBase> AbilityClass, bool bAutoRelease = false);

	/**
	 * Called when Health reaches 0. Grants State.Dead tag, cancels abilities,
	 * and disables movement.
	 *
	 * Override in C++ subclasses (e.g. AMFPetBase sets bIsDead on FMFPetInstance).
	 * Also callable from Blueprint via BlueprintCallable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void HandleDeath();

	// -----------------------------------------------------------------------
	// Hit React (IMFHitReactInterface)
	// -----------------------------------------------------------------------

	/** Flipbook 闪红持续时间（秒）。运行时由各子类从 Config 写入（不再在基类 BP 编辑）。 */
	float HitFlashDuration = 0.25f;

	virtual void ReactToHit_Implementation() override;

	/** 受到治疗时的视觉反馈：Flipbook 闪绿。由 OnHealthChangedCallback 在回血时调用。 */
	void ReactToHeal();

	// -----------------------------------------------------------------------
	// 2D Rendering Components
	// -----------------------------------------------------------------------

	/** Flipbook render component driven by PaperZD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	/** PaperZD animation component: owns the AnimInstance and drives FlipbookComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperZDAnimationComponent> AnimationComponent;

	// -----------------------------------------------------------------------
	// Character State
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FMFCharacterState CharacterState;

	// -----------------------------------------------------------------------
	// Collision fitting
	// -----------------------------------------------------------------------

	/**
	 * When true, UpdateCollisionFromFlipbook() is called automatically on BeginPlay.
	 * Disable if you prefer to set collision size manually in the Blueprint defaults.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision|Flipbook")
	bool bAutoUpdateCollisionFromFlipbook = true;

	/**
	 * Multiplier applied to (SpriteWidth / 2) to produce the collision sphere radius.
	 * Tune this per Blueprint to get the desired fit.
	 *   1.0 → radius = half the sprite width (exact fit)
	 *   0.5 → quarter of sprite width (tighter, avoids edge gaps)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision|Flipbook",
		meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float CollisionRadiusScale = 1.0f;

	/**
	 * Fit the root CapsuleComponent to a sphere based on the first frame of the
	 * current flipbook asset.
	 *
	 * Sets CapsuleRadius = CapsuleHalfHeight = (SpriteWidth / 2) * CollisionRadiusScale.
	 * When HalfHeight == Radius the capsule is geometrically identical to a sphere.
	 *
	 * Called automatically on BeginPlay when bAutoUpdateCollisionFromFlipbook is true.
	 * Can also be called from Blueprint at runtime after swapping the flipbook asset.
	 */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	virtual void UpdateCollisionFromFlipbook();

	// -----------------------------------------------------------------------
	// Billboard
	// -----------------------------------------------------------------------

	/**
	 * Yaw offset added after aligning the sprite plane to the camera.
	 *   0   = sprite local +X faces camera
	 *  -90  = sprite local +Y faces camera  (typical for Paper2D XZ-plane sprites)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard")
	float BillboardYawOffset = -90.f;

	// -----------------------------------------------------------------------
	// Camera abstraction — override in subclasses
	// -----------------------------------------------------------------------

	/**
	 * Return the camera's world-space forward vector (the direction the camera is looking).
	 * UpdateBillboard() negates this to get the "toward-camera" direction and applies it
	 * uniformly to all sprites — so every sprite in the scene shares the same tilt
	 * regardless of its world position (parallel / isometric-style billboard).
	 *
	 * Player: returns CameraComponent->GetForwardVector().
	 * AI:     returns PlayerCameraManager rotation vector.
	 */
	virtual bool GetBillboardCameraForward(FVector& OutForward) const { return false; }

	/**
	 * Return the yaw (degrees) of the in-game camera for directional animation mapping.
	 * This is fed into ComputeDirectionalInput() to produce camera-relative facing.
	 *
	 * Player: returns CameraController->GetSpriteOrientationYaw().
	 * AI:     returns the player camera manager yaw.
	 */
	virtual float GetCameraYawForDirectionality() const { return 0.f; }

	// -----------------------------------------------------------------------
	// Shared per-frame logic (called from Tick)
	// -----------------------------------------------------------------------

	/** Update CharacterState.CurrentAction based on velocity and bIsPicking. */
	void UpdateCharacterAction();

	/**
	 * Drive the AnimInstance (UMFAnimInstanceBase) with Speed, bIsPicking,
	 * and DirectionalInput. Override if you need character-specific anim logic.
	 */
	virtual void UpdateAnimation();

	/** Rotate FlipbookComponent to always face the billboard camera. */
	void UpdateBillboard();

	/** Console-var-gated debug visualization (arrows + screen text). */
	virtual void DrawDebug() const;

	/**
	 * Debug：友方（持 MF.Team.Player）头顶画一个绿色圆点，便于在怪堆里区分敌我。
	 * 由 MF.Char.FriendlyMarker 控制（默认开）。仅 ENABLE_DRAW_DEBUG 构建有效。
	 */
	void DrawFriendlyMarker() const;

	/**
	 * Render all GAS attribute values as world-space text above this character.
	 * Called from DrawDebug() when MF.Char.AttributeDebug is non-zero.
	 */
	void DrawAttributeDebug() const;

	/**
	 * Compute a 2D camera-relative facing vector for PaperZD SetDirectionality.
	 * Uses CharacterState.LastVelocityDir and GetCameraYawForDirectionality().
	 */
	FVector2D GetDirectionalInput() const;

private:
	FTimerHandle HitFlashTimerHandle;

	void OnHealthChangedCallback(float OldHealth, float NewHealth);

	/** MoveSpeed 属性变化 → 写入 CharacterMovement->MaxWalkSpeed。 */
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	/** State.Stunned 标签变化 → 进入眩晕禁动+打断 / 解除恢复行走。 */
	void OnStunnedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** 将 Flipbook 染成指定颜色并启动复位定时器（受击/治疗闪光共用）。 */
	void FlashSpriteColor(const FLinearColor& Color);
	void ResetHitFlashColor();
};
