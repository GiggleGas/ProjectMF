// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MFHitReactInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMFHitReactInterface : public UInterface { GENERATED_BODY() };

/**
 * 受击视觉反馈接口。实现该接口的 Actor 在受到实际伤害时会收到通知。
 * C++ 默认实现：Flipbook 闪红后还原。蓝图子类可覆盖做额外特效。
 */
class PROJECTMF_API IMFHitReactInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Combat")
	void ReactToHit();
};
