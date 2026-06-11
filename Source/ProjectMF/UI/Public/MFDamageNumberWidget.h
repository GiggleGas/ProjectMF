// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFDamageNumberWidget.generated.h"

class UTextBlock;

/**
 * 单个战斗飘字（伤害红字 / 治疗绿字）。
 *
 * 表现：从角色头顶弹出 —— 缩放过冲（跳出感）+ 上抛短弧 + 末段淡出，存活结束自销毁。
 * 动画逻辑全部在 C++（NativeTick）；WBP 子类只需放一个名为 AmountText 的 TextBlock。
 * 参数从 UMFFloatingTextSettings 读取。
 *
 * 用法：UMFDamageNumberWidget::ShowDamageNumber(this, GetActorLocation(), Amount, bHeal);
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API UMFDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 在 WorldLocation 头顶生成一个飘字。读取项目设置（widget 类 / 参数 / 开关）。
	 * 投影到屏幕失败（在相机后方）或功能关闭时静默返回。
	 */
	static void ShowDamageNumber(const UObject* WorldContext, const FVector& WorldLocation, float Amount, bool bHeal);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** 设置文本/颜色、初始屏幕位置与运动参数（从设置读取）。 */
	void InitNumber(float Amount, bool bHeal, const FVector2D& InScreenPos);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmountText;

	// 运行时状态
	FVector2D ScreenPos = FVector2D::ZeroVector;
	FVector2D Velocity   = FVector2D::ZeroVector;
	float Elapsed        = 0.f;

	// 从设置缓存的参数
	float LifetimeSec    = 0.9f;
	float PopTimeSec     = 0.15f;
	float StartScaleV    = 0.4f;
	float OvershootV     = 1.25f;
	float SettleScaleV   = 1.0f;   // 本数字的随机稳定缩放
	float GravityV       = 480.f;
	float FadeOutSec     = 0.35f;

	bool bInitialized = false;
};
