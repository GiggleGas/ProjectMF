// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "MFCharacterTypes.h"
#include "MFCharacterBase.generated.h"

class UPaperFlipbookComponent;
class UPaperZDAnimationComponent;
class UAbilitySystemComponent;
class UMFAttributeSetBase;
class UMFCombatAttributeSet;
class UMFGameplayAbilityBase;
class UGameplayEffect;

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
class PROJECTMF_API AMFCharacterBase : public ACharacter, public IAbilitySystemInterface
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

	/** Combat attribute set: Attack, Defense, FleeThreshold. Owned by this actor. */
	UPROPERTY()
	TObjectPtr<UMFCombatAttributeSet> CombatAttributeSet;

	/**
	 * Abilities granted at BeginPlay.
	 * Configure in the Blueprint default for each character type.
	 * Example: BP_MFCharacter adds GA_Pick here.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UMFGameplayAbilityBase>> DefaultAbilities;

	/**
	 * Loose GameplayTags added to the ASC at BeginPlay.
	 * 用于声明该角色的阵营或固有属性标签，无需 GameplayEffect 即可持有。
	 *
	 * 典型配置（在各子类 Blueprint 的 Defaults 中设置）：
	 *   BP_MFCharacter (玩家)  → MF.Team.Player
	 *   BP_MFPet / BP_MFEnemy → MF.Team.Enemy
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTagContainer DefaultOwnedTags;

	/**
	 * Instant GameplayEffect applied at BeginPlay to set initial attribute values
	 * (MaxHealth, Health, MoveSpeed, Attack, Defense, FleeThreshold).
	 *
	 * Create one Blueprint GE per character type (e.g. GE_Init_Cat, GE_Init_Boss)
	 * and assign it here in the Blueprint defaults. If null, constructor defaults are used.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultInitEffect;

	/**
	 * Initialize the ASC actor info, grant DefaultAbilities, and apply DefaultInitEffect.
	 * Called from BeginPlay() — safe to call on both server and standalone.
	 */
	void InitAbilitySystemComponent();

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
	 * Render all GAS attribute values as world-space text above this character.
	 * Called from DrawDebug() when MF.Char.AttributeDebug is non-zero.
	 */
	void DrawAttributeDebug() const;

	/**
	 * Compute a 2D camera-relative facing vector for PaperZD SetDirectionality.
	 * Uses CharacterState.LastVelocityDir and GetCameraYawForDirectionality().
	 */
	FVector2D GetDirectionalInput() const;
};
