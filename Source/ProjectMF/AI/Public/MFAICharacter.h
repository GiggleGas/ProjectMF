// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFCharacterBase.h"
#include "MFMassInterface.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFAICharacter.generated.h"

class UWidgetComponent;
class UMFOverheadWidget;
class UMFCombatAttributeSet;
class UMFAIConfig;

/**
 * Base class for all AI-controlled characters in ProjectMF.
 *
 * Inherits the 2D PaperZD rendering pipeline from AMFCharacterBase and adds:
 *  - Billboard alignment toward the player camera (no own camera rig needed).
 *  - IMFMassControllable implementation: Mass Processors can push move targets
 *    and action overrides each tick via ApplyMassCommand().
 *
 * Blueprint subclass (e.g., BP_MFEnemy) should:
 *  - Set the AnimBP class to an AnimBP that references UMFAIAnimInstance.
 *  - Configure the CharacterMovementComponent (speed, nav walking, etc.).
 *  - Optionally assign an AIController Blueprint for behavior trees.
 *
 * Mass integration notes (see MFMassInterface.h for the roadmap):
 *  Phase 1: Processor fetches the Actor, calls Execute_ApplyMassCommand().
 *  Phase 2: Use FMassEntityHandle fragments and MassRepresentationProcessor.
 */
UCLASS()
class PROJECTMF_API AMFAICharacter : public AMFCharacterBase, public IMFMassControllable
{
	GENERATED_BODY()

public:
	AMFAICharacter();

	// -----------------------------------------------------------------------
	// IMFMassControllable
	// -----------------------------------------------------------------------

	/** Push a movement/action command from a Mass Processor. */
	virtual void ApplyMassCommand_Implementation(const FMFAICommand& Command) override;

	/** Notify that a Mass Entity has claimed this Actor as its visual representation. */
	virtual void OnMassEntityLinked_Implementation(int32 EntityIndex) override;

	/** Notify that the Mass Entity has released this Actor (LOD swap / pooling). */
	virtual void OnMassEntityUnlinked_Implementation() override;

	// -----------------------------------------------------------------------
	// Runtime config injection (called post-spawn by manager or subclasses)
	// -----------------------------------------------------------------------

	/**
	 * 将 UMFAIConfig 中的配置写入本角色，post-BeginPlay 安全调用。
	 *
	 * 写入内容：
	 *   - GAS：追加授予 DefaultAbilities，应用 DefaultInitEffect，添加 DefaultOwnedTags
	 *   - HitFlashDuration
	 *   - OverheadWidget：设置 WidgetClass 并初始化
	 *
	 * 由 AMFSpawnAIManager 通过 ApplyPetConfig（→ 内部调用本函数）驱动。
	 * 关卡直接摆放的 AI 也可在 Blueprint AIConfig 属性中配置，由 BeginPlay 处理。
	 * Config 为 nullptr 时安全跳过。
	 */
	void ApplyAIConfig(const UMFAIConfig* Config);

	// -----------------------------------------------------------------------
	// State accessors (read-only from Blueprints / other systems)
	// -----------------------------------------------------------------------

	/** Mass Entity index this Actor is currently representing. -1 = unlinked. */
	UFUNCTION(BlueprintPure, Category = "Mass")
	int32 GetLinkedMassEntityIndex() const { return LinkedMassEntityIndex; }

	/** Returns true if this Actor is currently driven by a Mass Entity. */
	UFUNCTION(BlueprintPure, Category = "Mass")
	bool IsLinkedToMassEntity() const { return LinkedMassEntityIndex != INDEX_NONE; }

protected:
	virtual void BeginPlay() override;

	// -----------------------------------------------------------------------
	// AMFCharacterBase camera interface
	// -----------------------------------------------------------------------

	/**
	 * Return the player camera forward vector so all AI sprites share the same
	 * uniform tilt as the player sprite (parallel billboard, no per-object deviation).
	 */
	virtual bool  GetBillboardCameraForward(FVector& OutForward) const override;

	/**
	 * Use the player camera yaw so AI sprite directions match the player's view.
	 * Keeps directional sprites consistent across player and AI characters.
	 */
	virtual float GetCameraYawForDirectionality() const override;

	// -----------------------------------------------------------------------
	// AI Config
	// -----------------------------------------------------------------------

	/**
	 * AI 通用配置资产（DataAsset）。
	 * 汇总 GAS 初始化、头顶 Widget 和战斗参数。
	 * BeginPlay 会将 Config 值复制到对应成员属性，后续流程无感知。
	 * 留空则使用各属性的编辑器默认值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UMFAIConfig> AIConfig;

	// -----------------------------------------------------------------------
	// GAS — Combat attributes (AI-only)
	// -----------------------------------------------------------------------

	/** Combat attribute set: Attack, Defense, FleeThreshold. AI characters only. */
	UPROPERTY()
	TObjectPtr<UMFCombatAttributeSet> CombatAttributeSet;

	// -----------------------------------------------------------------------
	// Overhead UI (Screen Space Widget)
	// -----------------------------------------------------------------------

	/** Screen-space widget projected from this component's world position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComp;

	/** Blueprint widget class (must inherit UMFOverheadWidget). Leave null to disable. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMFOverheadWidget> OverheadWidgetClass;

	/** Z offset (relative to capsule center) for the overhead widget anchor. */
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float OverheadWidgetZOffset = 120.f;

	// -----------------------------------------------------------------------
	// Debug
	// -----------------------------------------------------------------------

	/** Extends base DrawDebug with combat attribute display (ATK/DEF/Flee). */
	virtual void DrawDebug() const override;

	// -----------------------------------------------------------------------
	// Radar Sensing
	// -----------------------------------------------------------------------

	/**
	 * 雷达感知组件：球形范围内检测指定 GameplayTag 的目标。
	 * 在编辑器中通过 RadarSensingComp 配置感知半径和目标 Tag。
	 * StateTree / 威胁系统通过 OnTargetDetected 委托或 GetPerceivedActors() 获取结果。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Radar")
	TObjectPtr<UMFRadarSensingComponent> RadarSensingComp;

	/**
	 * 索敌组件：消费 RadarSensingComp 的感知列表，进行打分并维护当前目标。
	 * 通过 GetCurrentTarget() 获取目标，供 StateTree / 攻击系统调用。
	 * 目标变化时广播 OnTargetChanged；ASC 上的 MF.AI.Perception.HasTarget 标签联动更新。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Threat")
	TObjectPtr<UMFThreatComponent> ThreatComp;

	// -----------------------------------------------------------------------
	// Mass-driven state (protected so Blueprint subclasses can inspect it)
	// -----------------------------------------------------------------------

	/** Opaque index of the linked Mass Entity. INDEX_NONE (-1) when unlinked. */
	UPROPERTY(BlueprintReadOnly, Category = "Mass")
	int32 LinkedMassEntityIndex = INDEX_NONE;

	/** Cached move target from the last Mass command (world space). */
	UPROPERTY(BlueprintReadOnly, Category = "Mass")
	FVector MassMoveTarget = FVector::ZeroVector;

private:
	/** Apply movement toward MassMoveTarget if the command carries a valid target. */
	void ApplyMovementFromCommand(const FMFAICommand& Command);
};
