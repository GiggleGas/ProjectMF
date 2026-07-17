// Copyright ProjectMF. All Rights Reserved.

#include "MFSpriteVisualComponent.h"
#include "MFLog.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// CVar：MF.SpriteVisual.Debug 1  → billboard/碰撞拟合时打 log + 画调试图元
// ---------------------------------------------------------------------------
static int32 GSpriteVisualDebug = 0;
static FAutoConsoleVariableRef CVarSpriteVisualDebug(
	TEXT("MF.SpriteVisual.Debug"),
	GSpriteVisualDebug,
	TEXT("Enable UMFSpriteVisualComponent debug (billboard normal arrow, collision sphere, verbose log). 1=on, 0=off."),
	ECVF_Default
);

UMFSpriteVisualComponent::UMFSpriteVisualComponent()
{
	// billboard 需要每帧更新（仅在 bDriveBillboardOnTick 时真正执行）。
	PrimaryComponentTick.bCanEverTick = true;
}

// ---------------------------------------------------------------------------
// 绑定
// ---------------------------------------------------------------------------

void UMFSpriteVisualComponent::InitVisual(UPaperFlipbookComponent* InFlipbook, UCapsuleComponent* InCapsule)
{
	Flipbook      = InFlipbook;
	TargetCapsule = InCapsule;

	const AActor* Owner = GetOwner();
	if (!Flipbook)
	{
		MF_LOG_WARNING(LogMFVisual, TEXT("[%s] InitVisual：未绑定 Flipbook，表现将不更新。"),
			Owner ? *Owner->GetName() : TEXT("?"));
	}
	else
	{
		UE_LOG(LogMFVisual, Log, TEXT("[%s] InitVisual：Flipbook=%s  Capsule=%s"),
			Owner ? *Owner->GetName() : TEXT("?"),
			*Flipbook->GetName(),
			InCapsule ? *InCapsule->GetName() : TEXT("<none>"));
	}
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UMFSpriteVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDriveBillboardOnTick)
	{
		TickBillboard();
	}
}

// ---------------------------------------------------------------------------
// ① Billboard —— 与原 AMFCharacterBase::UpdateBillboard 同算法
// ---------------------------------------------------------------------------

void UMFSpriteVisualComponent::TickBillboard()
{
	if (!Flipbook) return;

	FVector CamForward;
	if (!CameraForwardProvider || !CameraForwardProvider(CamForward))
	{
		return;   // 拿不到相机 → 当帧跳过
	}

	// 全场景 sprite 用同一朝向：相机注视方向的反向 → 统一倾角（无逐物角度差）。
	const FVector ToCam = -CamForward;

	// 让 sprite 局部 +Y（Paper2D 法线）指向相机。
	const FRotator BillRot = FRotationMatrix::MakeFromYZ(ToCam, FVector::UpVector).Rotator();
	Flipbook->SetWorldRotation(BillRot);

#if ENABLE_DRAW_DEBUG
	if (GSpriteVisualDebug)
	{
		const AActor* Owner = GetOwner();
		const FVector Origin = Flipbook->GetComponentLocation();
		if (const UWorld* World = GetWorld())
		{
			// 洋红箭头 = sprite 当前法线朝向（应指向相机）。
			DrawDebugDirectionalArrow(World, Origin, Origin + ToCam * 60.f,
				12.f, FColor::Magenta, false, -1.f, 0, 2.f);
		}
		UE_LOG(LogMFVisual, Verbose, TEXT("[%s] Billboard: ToCam=(%.2f,%.2f,%.2f)  Rot=(%.1f,%.1f,%.1f)"),
			Owner ? *Owner->GetName() : TEXT("?"),
			ToCam.X, ToCam.Y, ToCam.Z, BillRot.Pitch, BillRot.Yaw, BillRot.Roll);
	}
#endif
}

// ---------------------------------------------------------------------------
// ② 闪光 —— 与原 FlashSpriteColor / ResetHitFlashColor 同算法
// ---------------------------------------------------------------------------

void UMFSpriteVisualComponent::FlashColor(const FLinearColor& Color, float Duration)
{
	if (!Flipbook)
	{
		MF_LOG_WARNING(LogMFVisual, TEXT("FlashColor：无绑定 Flipbook，忽略。"));
		return;
	}

	Flipbook->SetSpriteColor(Color);

	const AActor* Owner = GetOwner();
	UE_LOG(LogMFVisual, Log, TEXT("[%s] FlashColor: (%.2f,%.2f,%.2f) for %.2fs"),
		Owner ? *Owner->GetName() : TEXT("?"), Color.R, Color.G, Color.B, Duration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FlashTimerHandle, this, &UMFSpriteVisualComponent::ResetColorToWhite,
			Duration, /*bLoop=*/false);
	}
}

void UMFSpriteVisualComponent::ResetColorToWhite()
{
	if (Flipbook)
	{
		Flipbook->SetSpriteColor(FLinearColor::White);
	}
}

// ---------------------------------------------------------------------------
// ③ 碰撞自适应 —— 与原 UpdateCollisionFromFlipbook 同算法
// ---------------------------------------------------------------------------

void UMFSpriteVisualComponent::FitCollisionToFlipbook(float RadiusScale)
{
	UCapsuleComponent* Capsule = TargetCapsule.Get();
	if (!Capsule || !Flipbook)
	{
		MF_LOG_WARNING(LogMFVisual, TEXT("FitCollisionToFlipbook：缺少 Capsule 或 Flipbook，跳过。"));
		return;
	}

	const UPaperFlipbook* FlipbookAsset = Flipbook->GetFlipbook();
	if (!FlipbookAsset || FlipbookAsset->GetNumKeyFrames() == 0) return;

	const UPaperSprite* Sprite = FlipbookAsset->GetKeyFrameChecked(0).Sprite;
	if (!Sprite) return;

	const float PixelsPerUnit = Sprite->GetPixelsPerUnrealUnit();
	if (PixelsPerUnit <= KINDA_SMALL_NUMBER) return;

	// SpriteWidth / PPU = 世界宽度；一半 = 球半径（billboard 角色的水平落脚宽度取 X）。
	const FVector2D SourceSize = Sprite->GetSourceSize();
	const float SpriteWidthUnits = SourceSize.X / PixelsPerUnit;
	const float NewRadius = FMath::Max(1.f, SpriteWidthUnits * 0.5f * RadiusScale);

	// HalfHeight == Radius → 胶囊几何等价于球；bUpdateOverlaps 立即重算 overlap。
	Capsule->SetCapsuleSize(NewRadius, NewRadius, /*bUpdateOverlaps=*/true);

	// 把 sprite 下移 Radius，令其局部原点落在碰撞球底部（视觉落脚点对齐物理）。
	Flipbook->SetRelativeLocation(FVector(0.f, 0.f, -NewRadius));

	const AActor* Owner = GetOwner();
	UE_LOG(LogMFVisual, Log,
		TEXT("[%s] FitCollision: Radius=%.1f  (sprite %.0f x %.0f px, PPU=%.2f, scale=%.2f)"),
		Owner ? *Owner->GetName() : TEXT("?"),
		NewRadius, SourceSize.X, SourceSize.Y, PixelsPerUnit, RadiusScale);

#if ENABLE_DRAW_DEBUG
	if (GSpriteVisualDebug)
	{
		if (const UWorld* World = GetWorld())
		{
			DrawDebugSphere(World, Capsule->GetComponentLocation(), NewRadius,
				16, FColor::Green, false, 2.f, 0, 2.f);
		}
	}
#endif
}
