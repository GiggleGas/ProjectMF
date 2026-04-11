// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MFCatchable.generated.h"

/**
 * IMFCatchable — 可被抓取的实体接口。
 *
 * 所有能被玩家抓取的宠物 Actor 需实现此接口。
 * AT_WaitPetTarget 在追踪命中时通过此接口判断是否可抓，以及触发收服/失败回调。
 *
 * Blueprint 实现方式：
 *   在 Blueprint Actor 的 Class Settings → Interfaces 中添加 MFCatchable，
 *   然后在 Event Graph 中重写各事件节点。
 */
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMFCatchable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTMF_API IMFCatchable
{
	GENERATED_BODY()

public:
	/**
	 * 返回此实体当前是否可以被指定 Catcher 抓取。
	 *
	 * 典型判断条件：
	 *   - 宠物等级 vs 玩家等级
	 *   - 是否已被其他玩家占有
	 *   - 特殊宠物是否携带了指定道具
	 *   - 是否处于战斗/逃跑状态
	 *
	 * @param Catcher  尝试抓取的玩家角色。
	 * @return true = 白色描边（可抓），false = 红色描边（不可抓）。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Catching")
	bool CanBeCaught(const AActor* Catcher) const;

	/**
	 * 收服成功回调。
	 * 在此处理宠物加入玩家队伍、播放动画、销毁野生状态等逻辑。
	 *
	 * @param Catcher  成功收服的玩家角色。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Catching")
	void OnCaught(AActor* Catcher);

	/**
	 * 抓取失败回调（QTE 超时或玩家取消）。
	 * 可在此触发宠物逃跑动画或恢复正常 AI 行为。
	 *
	 * @param Catcher  尝试抓取失败的玩家角色。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Catching")
	void OnCatchFailed(AActor* Catcher);
};
