// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFCharacterBase.h"
#include "MFCharacter.generated.h"

class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UMFCameraController;
class UMFInventoryComponent;
class UMFCraftingComponent;
class UMFPlayerConfig;
class UMFPlayerAttributeSet;
class AMFPetBase;
class AMFLootPickup;
class UMFThrowableData;
struct FInputActionValue;

/**
 * Player-controlled character.
 * Extends AMFCharacterBase with a camera rig, enhanced input bindings,
 * and camera-relative directional movement.
 */
UCLASS()
class PROJECTMF_API AMFCharacter : public AMFCharacterBase
{
	GENERATED_BODY()

public:
	AMFCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Camera")
	UMFCameraController* GetCameraController() const { return CameraController; }

	/** 玩家相机组件访问器（供命令模式驱动后处理：去色 / 暗角）。 */
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

protected:
	virtual void BeginPlay() override;

	/** 从 PlayerConfig 应用 GAS（初始属性 / 授予技能 / 阵营标签 / 受击闪光）。InitASC 内调用。 */
	virtual void ApplyGASConfig() override;

	// -----------------------------------------------------------------------
	// Camera accessors (AMFCharacterBase interface)
	// -----------------------------------------------------------------------

	virtual bool  GetBillboardCameraForward(FVector& OutForward) const override;
	virtual float GetCameraYawForDirectionality() const override;

	// -----------------------------------------------------------------------
	// Player Config
	// -----------------------------------------------------------------------

	/**
	 * 玩家专属配置资产（DataAsset）。
	 * 汇总输入绑定、UI 类、GAS 初始化和战斗参数。
	 * BP_MFCharacter 和 BP_PlayerController 引用同一个资产实例。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UMFPlayerConfig> PlayerConfig;

	// -----------------------------------------------------------------------
	// Camera Components
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMFCameraController> CameraController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMFInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMFCraftingComponent> CraftingComponent;

	/** Player-only attribute set. Extension point for player-specific GAS attributes. */
	UPROPERTY()
	TObjectPtr<UMFPlayerAttributeSet> PlayerAttributeSet;

public:
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UMFInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	UMFCraftingComponent* GetCraftingComponent() const { return CraftingComponent; }

	/** 使用消耗品：配了 ThrowableData → 进投掷瞄准抛物线扔出；否则直接对宠 UseConsumable。 */
	void TryUseConsumable(int32 ItemID);

	virtual void Tick(float DeltaTime) override;

	/** GM/调试：按序击杀一只自己的出战宠（每次一只，走完整死亡→濒死流程）。控制台输入 MFKillNextPet。 */
	UFUNCTION(Exec)
	void MFKillNextPet();

	/** GM/调试：在脚下生成掉落物。控制台输入 MFSpawnLoot Item.Resource.Meat 3。 */
	UFUNCTION(Exec)
	void MFSpawnLoot(int32 ItemID, int32 Count = 1);

	/** GM/调试：在脚下按掉落表资产名 roll 一次（验概率/分布）。控制台输入 MFDropTable LT_TestPet。 */
	UFUNCTION(Exec)
	void MFDropTable(const FString& TableAssetName);

	/** GM/调试：合成一个配方。控制台输入 MFCraft Recipe_HealSnack。 */
	UFUNCTION(Exec)
	void MFCraft(const FString& RecipeID);

	/** GM/调试：使用一个消耗品（喂全体召唤宠）。控制台输入 MFUseItem 2001。 */
	UFUNCTION(Exec)
	void MFUseItem(int32 ItemID);

private:
	void HandleMove(const FInputActionValue& Value);
	void HandlePickStarted();
	void HandlePickCompleted();
	void HandleCameraRotate(const FInputActionValue& Value);

	/**
	 * 抓宠键松开时触发，激活 GA_CatchPet（MF.Ability.CatchPet）。
	 * 技能内部通过 AT_WaitPetTarget Task 处理后续的鼠标瞄准和确认逻辑。
	 */
	void HandleCatchPet();

	// DEMO: 临时召唤绑定，由 GA_PetWheel 替换后删除
	void HandleSummonSlot(int32 SlotIndex);

	void HandleStartBossBattle();

	/** 命令模式键：已激活则取消（提前退出），否则按 tag 激活 GA_CommandMode（冷却中会被自动挡）。 */
	void HandleCommandMode();

	/**
	 * 抱宠/复活统一键（切换式）：抱/复活中则取消；否则就近友方宠——濒死→激活 GA_RevivePet，存活→激活 GA_CarryPet。
	 */
	void HandleCarryOrRevive();

	/** 就近找友方宠（Team.Player + 在 CarryReach 内 + 未被抱）；供 HandleCarryOrRevive 判断死活。 */
	AMFPetBase* FindNearestFriendlyPetInReach() const;

	/** 就近找掉落物（CarryReach 内最近）；供 HandleCarryOrRevive 优先拾取。 */
	AMFLootPickup* FindNearestLootPickupInReach() const;

	// -----------------------------------------------------------------------
	// 消耗品投掷瞄准（配了 ThrowableData 的消耗品）
	// -----------------------------------------------------------------------

	/** 进入投掷瞄准态。 */
	void BeginThrowConsumable(int32 ItemID, const UMFThrowableData* ThrowableData);

	/** 投掷瞄准态每帧：左键确认落点 → 抛物线扔出投射物，右键取消。 */
	void TickThrowAiming();

	/** 反算抛物线初速度 + 重力（由 Speed/ArcHeight），Launch 一个 Flipbook 投射物；落地由回调生成 ImpactArea。 */
	void LaunchThrowable(const FVector& InTarget);

	bool bThrowAiming = false;
	int32 PendingThrowItemID = 0;
	const UMFThrowableData* PendingThrowable = nullptr;
};
