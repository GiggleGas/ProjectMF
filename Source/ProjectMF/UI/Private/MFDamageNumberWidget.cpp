// Copyright ProjectMF. All Rights Reserved.

#include "MFDamageNumberWidget.h"
#include "MFFloatingTextSettings.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UMFDamageNumberWidget::ShowDamageNumber(const UObject* WorldContext, const FVector& WorldLocation, float Amount, bool bHeal)
{
	const UMFFloatingTextSettings* Settings = GetDefault<UMFFloatingTextSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	UClass* WidgetClass = Settings->WidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!PC)
	{
		return;
	}

	// 头顶世界坐标 → 屏幕坐标（在相机后方则不显示）
	const FVector Anchor = WorldLocation + FVector(0.f, 0.f, Settings->HeadZOffset);
	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(Anchor, ScreenPos))
	{
		return;
	}

	// TODO(对象池)：当前每个飘字都 CreateWidget + 结束 RemoveFromParent。战斗数字密集时
	// 创建/销毁开销与 GC 压力偏大，后续应改为 widget 池（取用/归还、不销毁）。
	UMFDamageNumberWidget* Widget = CreateWidget<UMFDamageNumberWidget>(PC, WidgetClass);
	if (!Widget)
	{
		return;
	}

	Widget->AddToViewport();
	Widget->InitNumber(Amount, bHeal, ScreenPos);
}

void UMFDamageNumberWidget::InitNumber(float Amount, bool bHeal, const FVector2D& InScreenPos)
{
	const UMFFloatingTextSettings* Settings = GetDefault<UMFFloatingTextSettings>();

	if (AmountText)
	{
		AmountText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
		AmountText->SetColorAndOpacity(FSlateColor(bHeal ? Settings->HealColor : Settings->DamageColor));
	}

	ScreenPos = InScreenPos;

	// 随机速度 + 随机发射角（相对竖直向上；屏幕 Y 向下为正，故上抛分量取负）→ 每个数字飞出方向/速度不同
	const float Speed = FMath::Max(0.f,
		Settings->LaunchUpSpeed + FMath::FRandRange(-Settings->LaunchSpeedJitter, Settings->LaunchSpeedJitter));
	const float AngleRad = FMath::DegreesToRadians(
		FMath::FRandRange(-Settings->LaunchAngleJitter, Settings->LaunchAngleJitter));
	Velocity = FVector2D(FMath::Sin(AngleRad) * Speed, -FMath::Cos(AngleRad) * Speed);

	// 随机稳定大小 → 每个数字最终大小不同
	SettleScaleV = FMath::Max(0.05f, 1.f + FMath::FRandRange(-Settings->ScaleJitter, Settings->ScaleJitter));

	LifetimeSec = Settings->Lifetime;
	PopTimeSec  = Settings->PopTime;
	StartScaleV = Settings->StartScale;
	OvershootV  = Settings->OvershootScale;
	GravityV    = Settings->Gravity;
	FadeOutSec  = Settings->FadeOutTime;

	SetPositionInViewport(ScreenPos, true);
	SetRenderScale(FVector2D(StartScaleV * SettleScaleV, StartScaleV * SettleScaleV));
	SetRenderOpacity(1.f);

	bInitialized = true;
}

void UMFDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bInitialized)
	{
		return;
	}

	Elapsed += InDeltaTime;

	// 上抛短弧（屏幕空间）
	Velocity.Y += GravityV * InDeltaTime;
	ScreenPos  += Velocity * InDeltaTime;
	SetPositionInViewport(ScreenPos, true);

	// 缩放过冲：StartScale →(0.6*PopTime)→ Overshoot →(PopTime)→ 1.0
	float Scale = 1.f;
	if (Elapsed < PopTimeSec)
	{
		const float Half = PopTimeSec * 0.6f;
		Scale = (Elapsed < Half)
			? FMath::Lerp(StartScaleV, OvershootV, Elapsed / Half)
			: FMath::Lerp(OvershootV, 1.f, (Elapsed - Half) / FMath::Max(PopTimeSec - Half, KINDA_SMALL_NUMBER));
	}
	// 乘以本数字的随机稳定缩放：过冲曲线收敛到 SettleScaleV 而非固定 1.0
	SetRenderScale(FVector2D(Scale * SettleScaleV, Scale * SettleScaleV));

	// 末段淡出
	const float FadeStart = LifetimeSec - FadeOutSec;
	const float Opacity = (Elapsed <= FadeStart || FadeOutSec <= 0.f)
		? 1.f
		: FMath::Clamp(1.f - (Elapsed - FadeStart) / FadeOutSec, 0.f, 1.f);
	SetRenderOpacity(Opacity);

	if (Elapsed >= LifetimeSec)
	{
		RemoveFromParent();
	}
}
