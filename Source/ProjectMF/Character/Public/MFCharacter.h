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
class UMFPlayerConfig;
class UMFPlayerAttributeSet;
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

	/** Player-only attribute set. Extension point for player-specific GAS attributes. */
	UPROPERTY()
	TObjectPtr<UMFPlayerAttributeSet> PlayerAttributeSet;

public:
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UMFInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

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
};
