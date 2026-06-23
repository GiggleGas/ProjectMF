// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MFTimeControlSubsystem.generated.h"

/**
 * UMFTimeControlSubsystem — 全局时间膨胀的统一入口（WorldSubsystem）。
 *
 * 把"谁在改世界时间、最终多少"收口到一处：多个来源（玩家林克时间、Boss 减速、Boss 时停…）
 * 各自 Request 自己的倍率；子系统取所有有效请求的**最小值**（最极端的慢生效）应用到
 * SetGlobalTimeDilation；来源 Release 后重算。避免多处各自 SetGlobalTimeDilation 互相覆盖。
 *
 * 用法：
 *   Request(this, 0.2f)  // 进入减速
 *   Release(this)        // 退出（务必成对；来源 UObject 失效会被自动清理）
 *   时停：传很小的值（0 会被 WorldSettings.MinGlobalTimeDilation 夹紧）。
 */
UCLASS()
class PROJECTMF_API UMFTimeControlSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 注册/更新某来源请求的时间倍率（0~1，越小越慢）。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Time", meta = (DefaultToSelf = "Source"))
	void RequestTimeDilation(UObject* Source, float Dilation);

	/** 撤销某来源的时间请求。 */
	UFUNCTION(BlueprintCallable, Category = "MF|Time", meta = (DefaultToSelf = "Source"))
	void ReleaseTimeDilation(UObject* Source);

	/** 当前生效的全局时间倍率（无请求时为 1.0）。 */
	UFUNCTION(BlueprintPure, Category = "MF|Time")
	float GetEffectiveDilation() const { return CurrentDilation; }

	virtual void Deinitialize() override;

private:
	/** 重算最小倍率并应用；顺带清理失效弱引用。 */
	void Recompute();

	TMap<TWeakObjectPtr<UObject>, float> Requests;
	float CurrentDilation = 1.f;
};
