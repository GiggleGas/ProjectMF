// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MFTelegraphTypes.h"
#include "MFTelegraphSubsystem.generated.h"

class ADecalActor;
class UMaterialInterface;

/**
 * UMFTelegraphSubsystem — 预警显示的统一入口（WorldSubsystem），与 TimeControl/AreaEffect 同构。
 *
 * 攻击/技能在前摇期请求一个地面预警（圆/矩形 decal），前摇结束/命中时隐藏。
 * Decal 池化复用，敌我皆可见（世界渲染，不做阵营过滤）。
 */
UCLASS()
class PROJECTMF_API UMFTelegraphSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 显示一个预警，返回句柄。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Telegraph")
	FMFTelegraphHandle Show(const FMFTelegraphRequest& Request);

	/** 更新已有预警的位置/尺寸/颜色（跟随时调）。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Telegraph")
	void Update(const FMFTelegraphHandle& Handle, const FMFTelegraphRequest& Request);

	/** 隐藏并回收预警。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Telegraph")
	void Hide(const FMFTelegraphHandle& Handle);

	virtual void Deinitialize() override;

private:
	ADecalActor*        AcquireDecalActor();
	void                ApplyToDecal(ADecalActor* Decal, const FMFTelegraphRequest& Request) const;
	UMaterialInterface* ResolveBaseMaterial() const;

	/** 所有 decal（池）；FreeSlots 为空闲下标；HandleToSlot 把句柄映射到池槽。 */
	UPROPERTY()
	TArray<TObjectPtr<ADecalActor>> Pool;

	TArray<int32>      FreeSlots;
	TMap<uint32, int32> HandleToSlot;
	uint32             NextId = 1;
};
