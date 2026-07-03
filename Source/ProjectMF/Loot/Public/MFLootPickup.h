// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFSceneActorBase.h"
#include "MFLootPickup.generated.h"

class USphereComponent;
class AMFCharacter;

/**
 * AMFLootPickup — 地面掉落物（一个实例 = 一种 ItemID × Count）。
 *
 * 渲染沿用 AMFSceneActorBase 的 2D 管线（Flipbook/PaperZD），外观在 BP 子类配置；
 * 也可在 BP 实现 OnLootInitialized 按 ItemID 切换外观。
 *
 * 生命周期（由 UMFLootSubsystem 生成，勿直接摆放）：
 *   Scatter（出生小抛物线散开）→ Idle（待拾取）→ Magnet（玩家进入感应半径，加速飞向玩家）
 *   → 到达 → AddResource：全部入包销毁；背包满则扣除已入包部分、原地回落冷却后重试。
 *
 * 拾取触发者：MVP 仅玩家 Pawn（AMFCharacter）。宠物/劳作拾取留扩展。
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
	void InitLoot(FName InItemID, int32 InCount, const FVector& LandLocation);

	UFUNCTION(BlueprintPure, Category = "Loot")
	FName GetItemID() const { return ItemID; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetCount() const { return Count; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** BP 外观钩子：InitLoot 后调用，可按 ItemID/Count 切换 Flipbook 或缩放。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLootInitialized(FName InItemID, int32 InCount);

	// -----------------------------------------------------------------------
	// 组件
	// -----------------------------------------------------------------------

	/** 拾取感应球（overlap Pawn）。半径由 MagnetRadius 写入。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> MagnetSphere;

	// -----------------------------------------------------------------------
	// 手感配置（BP 子类调）
	// -----------------------------------------------------------------------

	/** 吸附感应半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "10.0"))
	float MagnetRadius = 120.f;

	/** 吸附初速（cm/s）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float MagnetInitialSpeed = 350.f;

	/** 吸附加速度（cm/s²）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float MagnetAcceleration = 3000.f;

	/** 判定入包的距离（到玩家中心，cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "5.0"))
	float AbsorbDistance = 40.f;

	/** 出生散开飞行时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ScatterDuration = 0.35f;

	/** 出生散开抛物线顶高（cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float ScatterArcHeight = 60.f;

	/** 存在时长（秒），超时自动消失。0 = 永久。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float Lifetime = 300.f;

	/** 背包满回落后，再次尝试吸附的冷却（秒）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.1"))
	float RetryCooldown = 1.f;

private:
	// -----------------------------------------------------------------------
	// 运行时状态
	// -----------------------------------------------------------------------

	enum class EPickupState : uint8 { Scatter, Idle, Magnet };

	EPickupState State = EPickupState::Idle;

	FName ItemID;
	int32 Count = 0;

	/** 散开插值：起点 / 落点 / 已飞时间。 */
	FVector ScatterStart = FVector::ZeroVector;
	FVector ScatterEnd   = FVector::ZeroVector;
	float   ScatterTime  = 0.f;

	/** 吸附目标（玩家）与当前速度。 */
	TWeakObjectPtr<AMFCharacter> MagnetTarget;
	float MagnetSpeed = 0.f;

	/** 背包满回落的冷却截止时间（世界秒）。 */
	float RetryReadyTime = 0.f;

	UFUNCTION()
	void OnMagnetBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	void TickScatter(float DeltaTime);
	void TickIdle();
	void TickMagnet(float DeltaTime);

	void StartMagnet(AMFCharacter* Target);

	/** 尝试把内容加入目标背包。全部入包 → 销毁；部分/失败 → 回落冷却。 */
	void TryGiveTo(AMFCharacter* Target);
};
