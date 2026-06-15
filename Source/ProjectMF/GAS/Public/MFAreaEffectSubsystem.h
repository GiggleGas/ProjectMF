// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MFAttackTypes.h"   // EAttackTargetFilter, FMFOnHitEffect
#include "MFAreaEffectSubsystem.generated.h"

class UMFAreaEffectData;
class UGameplayEffect;

/**
 * 区域效果句柄。生成方持有它以便提前取消（Cancel）。UID=0 表示无效。
 */
USTRUCT(BlueprintType)
struct FMFAreaHandle
{
	GENERATED_BODY()

	uint32 UID = 0;

	bool IsValid() const { return UID != 0; }
	void Invalidate()    { UID = 0; }
};

/**
 * World 子系统：驱动所有活动的"场"。
 *
 * 设计仿 UMFProjectileSubsystem：slot 化实例数组 + FTickableGameObject 独立 tick。
 * 每个场每 TickInterval 做一次球形 overlap → 阵营/死亡过滤 → 对目标施加伤害与状态效果
 * （周期重刷模型：控制类靠 GE 的刷新-stacking 维持，伤害类每 tick 打一次瞬时伤害）。
 *
 * 通过 UMFCombatStatics::SpawnAreaEffect 生成（统一入口，携带来源 Instigator）。
 */
UCLASS()
class PROJECTMF_API UMFAreaEffectSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/**
	 * 注册一个场。Instigator 作为来源（其 ASC 用于伤害/效果施加与阵营判定）。
	 * 返回句柄，可传给 Cancel 提前结束。
	 */
	FMFAreaHandle RegisterArea(AActor* Instigator, const UMFAreaEffectData* Data, const FVector& Location);

	/** 提前结束一个场。对无效句柄安全。 */
	void Cancel(FMFAreaHandle Handle);

	// -----------------------------------------------------------------------
	// FTickableGameObject
	// -----------------------------------------------------------------------
	virtual void    Tick(float DeltaTime) override;
	virtual bool    IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	/** 单个场的运行时状态。bActive=false 表示空闲槽。 */
	struct FAreaInstance
	{
		uint32 UID = 0;
		bool   bActive = false;

		FVector Location = FVector::ZeroVector;
		float   Radius = 0.f;
		float   RemainingLife = 0.f;
		float   TickInterval = 0.5f;
		float   TickAccumulator = 0.f;

		EAttackTargetFilter          TargetFilter = EAttackTargetFilter::EnemyOnly;
		TSubclassOf<UGameplayEffect> DamageGE;          // 类引用，play 期间不会被 GC（同投射物子系统约定）
		float                        DamageMultiplier = 0.f;
		TArray<FMFOnHitEffect>       Effects;           // 纯值（enum+float），GC 安全

		TWeakObjectPtr<AActor>       Instigator;
		TWeakObjectPtr<AActor>       VisualActor;       // 随场生成的表现 Actor，区域结束时销毁
	};

	/** 扁平 slot 数组，永不收缩。 */
	TArray<FAreaInstance> Instances;
	/** 空闲槽索引，供复用。 */
	TArray<int32> FreeSlots;
	/** 单调递增 UID。 */
	uint32 NextUID = 1;
	/** 活动场计数（驱动 IsTickable）。 */
	int32 ActiveCount = 0;

	/** 执行一次施加：overlap → 过滤 → 伤害 + 效果。 */
	void ApplyAreaTick(const FAreaInstance& Inst);

	int32 FindSlotByUID(uint32 UID) const;
};
