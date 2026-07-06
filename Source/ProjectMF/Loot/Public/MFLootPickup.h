// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFSceneActorBase.h"
#include "MFLootPickup.generated.h"

class UPaperSpriteComponent;

/**
 * AMFLootPickup — 地面掉落物（一个实例 = 一件物品）。
 *
 * 渲染沿用 AMFSceneActorBase 的 2D 管线；场景外观由物品总表的 WorldSprite 决定
 * （InitLoot 时按 ItemID 填入），一个类通吃所有物品。
 *
 * 生命周期（由 UMFLootSubsystem 生成，勿直接摆放）：
 *   Scatter（出生小抛物线散开）→ Idle（静止待拾取）→ PickUp()（玩家主动拾取后销毁）。
 *
 * 拾取 = 玩家主动交互（空格键，见 AMFCharacter::HandleCarryOrRevive 里的就近拾取）；
 * 当前无背包，PickUp() 虚空销毁，将来接背包时在此 AddResource。
 */
UCLASS(Blueprintable)
class PROJECTMF_API AMFLootPickup : public AMFSceneActorBase
{
	GENERATED_BODY()

public:
	AMFLootPickup();

	/**
	 * 初始化掉落内容并启动出生散开。由 UMFLootSubsystem 在 SpawnActor 后立即调用。
	 * @param LandLocation  散开落点（已对齐地面）。与当前位置相同则跳过散开直接 Idle。
	 */
	void InitLoot(int32 InItemID, int32 InCount, const FVector& LandLocation);

	/** 玩家主动拾取：当前无背包 → 虚空销毁。将来接背包时在此 AddResource(ItemID, Count)。 */
	void PickUp();

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetItemID() const { return ItemID; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetCount() const { return Count; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** BP 外观钩子：InitLoot 后调用，可按 ItemID/Count 切换外观或缩放。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLootInitialized(int32 InItemID, int32 InCount);

	/** 场景 2D 外观：显示物品的 WorldSprite（InitLoot 时按 ItemID 从总表取）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	/** 出生散开飞行时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ScatterDuration = 0.35f;

	/** 出生散开抛物线顶高（cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float ScatterArcHeight = 60.f;

	/** 存在时长（秒），超时自动消失。0 = 永久。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float Lifetime = 300.f;

	/** Sprite 面向相机的 yaw 偏移（度）。Paper2D XZ 平面 sprite 通常 -90。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float BillboardYawOffset = -90.f;

private:
	enum class EPickupState : uint8 { Scatter, Idle };

	EPickupState State = EPickupState::Idle;

	int32 ItemID = 0;
	int32 Count = 0;

	/** 散开插值：起点 / 落点 / 已飞时间。 */
	FVector ScatterStart = FVector::ZeroVector;
	FVector ScatterEnd   = FVector::ZeroVector;
	float   ScatterTime  = 0.f;

	void TickScatter(float DeltaTime);

	/** 让 Sprite 面向玩家相机（简易 billboard），任何相机角度都可见。 */
	void UpdateBillboard();
};
