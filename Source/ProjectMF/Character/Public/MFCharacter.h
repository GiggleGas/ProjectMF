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

protected:
	virtual void BeginPlay() override;

	// -----------------------------------------------------------------------
	// Camera accessors (AMFCharacterBase interface)
	// -----------------------------------------------------------------------

	virtual bool  GetBillboardCameraForward(FVector& OutForward) const override;
	virtual float GetCameraYawForDirectionality() const override;

	// -----------------------------------------------------------------------
	// Input Actions
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PickAction;

	/** 1D axis: +1 = snap CW (E key), -1 = snap CCW (Q key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RotateCameraAction;

	/**
	 * 抓宠键（建议绑定 F 或 E）。
	 * 触发器类型：Released（按下松开后激活技能，避免与移动键冲突）。
	 * 在 GAS 中激活 MF.Ability.CatchPet。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CatchPetAction;

	/**
	 * DEMO: 召唤宠物 slot 1-5 的临时按键绑定。
	 * 元素 0 = slot 1，元素 4 = slot 5。
	 * 每个按键触发时向 GAS 发送 MF.Ability.SummonPet 事件（Magnitude = slot 序号）。
	 * 待 GA_PetWheel（轮盘选择）实现后，此数组及绑定代码可整体删除。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Demo")
	TArray<TObjectPtr<UInputAction>> SummonSlotActions;

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
};
