// Copyright ProjectMF. All Rights Reserved.

#include "AT_WaitPetTarget.h"
#include "MFCatchPetConfig.h"
#include "MFCatchable.h"
#include "MFLog.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"

// ============================================================
// CVar：落点 Debug 可视化
// MF.Debug.CatchAim 1  → 在落点绘制球体（绿色=无宠物/黄色=有宠物）
// MF.Debug.CatchAim 0  → 关闭（默认）
// ============================================================

static int32 GMFCatchAimDebug = 0;
static FAutoConsoleVariableRef CVarMFCatchAimDebug(
	TEXT("MF.Debug.CatchAim"),
	GMFCatchAimDebug,
	TEXT("Show catch aim debug: 1=draw landing point sphere, 0=off (default)."),
	ECVF_Default);

// ============================================================
// 工厂
// ============================================================

UAbilityTask_WaitPetTarget* UAbilityTask_WaitPetTarget::CreateWaitPetTarget(
	UGameplayAbility*        OwningAbility,
	FName                    TaskInstanceName,
	const UMFCatchPetConfig* InConfig)
{
	UAbilityTask_WaitPetTarget* Task =
		NewAbilityTask<UAbilityTask_WaitPetTarget>(OwningAbility, TaskInstanceName);

	if (!InConfig)
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("AT_WaitPetTarget: Config is null! Task will not function correctly."));
	}
	Task->Config = InConfig;
	return Task;
}

// ============================================================
// 生命周期
// ============================================================

void UAbilityTask_WaitPetTarget::Activate()
{
	Super::Activate();

	if (!Config)
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("AT_WaitPetTarget::Activate — Config is null, ending task."));
		EndTask();
		return;
	}

	// 开启 Tick
	bTickingTask = true;

	// 显示鼠标指针（等距视角下瞄准需要光标）
	if (APlayerController* PC = GetOwnerController())
	{
		PC->bShowMouseCursor = true;
	}

	MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Activated — aiming phase started."));
}

void UAbilityTask_WaitPetTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!Config || !IsValid(this)) return;

	APlayerController* PC = GetOwnerController();
	if (!PC) return;

	AActor* Avatar = GetOwnerAvatar();

	// ---------------------------------------------------------------
	// 1. 射线检测 1：世界几何落点（ECC_Visibility）
	// ---------------------------------------------------------------
	FVector LandingPoint = FVector::ZeroVector;
	DoWorldTrace(LandingPoint);

	// ---------------------------------------------------------------
	// 2. 射线检测 2：宠物检测（ECC_Pawn + IMFCatchable）
	// ---------------------------------------------------------------
	bool    bCanCatch = false;
	AActor* NewTarget = DoPetTrace(bCanCatch);

	// ---------------------------------------------------------------
	// 3. 脚部发射点 + 调试可视化
	// ---------------------------------------------------------------
	if (Avatar)
	{
		FVector FeetPos = Avatar->GetActorLocation();
		if (const ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			FeetPos.Z -= Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}

		DrawAimDebug(FeetPos, LandingPoint, NewTarget != nullptr);
	}

	// ---------------------------------------------------------------
	// 4. 高亮状态切换
	// ---------------------------------------------------------------
	if (NewTarget != CurrentTarget.Get())
	{
		// 清除旧目标描边
		if (AActor* Old = CurrentTarget.Get())
		{
			RemoveHighlight(Old);
			MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Target lost — %s"), *Old->GetName());
			OnTargetLost.Broadcast();
		}

		CurrentTarget = NewTarget;

		if (NewTarget)
		{
			bCurrentTargetCanBeCaught = bCanCatch;
			const int32 Stencil = bCanCatch
				? Config->CatchableStencilValue
				: Config->UncatchableStencilValue;
			ApplyHighlight(NewTarget, Stencil);

			MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Target found — %s, CanCatch=%d"),
				*NewTarget->GetName(), bCanCatch ? 1 : 0);

			OnTargetFound.Broadcast(NewTarget, bCanCatch);
		}
	}
	else if (NewTarget && bCanCatch != bCurrentTargetCanBeCaught)
	{
		// 同一目标，但可抓状态变化（理论上少见，但做好防御）
		bCurrentTargetCanBeCaught = bCanCatch;
		const int32 Stencil = bCanCatch
			? Config->CatchableStencilValue
			: Config->UncatchableStencilValue;
		ApplyHighlight(NewTarget, Stencil);

		MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Same target %s, CanCatch changed to %d"),
			*NewTarget->GetName(), bCanCatch ? 1 : 0);

		OnTargetFound.Broadcast(NewTarget, bCanCatch);
	}

	// ---------------------------------------------------------------
	// 5. 输入检测（边沿触发）
	// ---------------------------------------------------------------

	// 左键：确认（仅当目标有效且可抓时）
	const bool bLeftDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	if (bLeftDown && !bWasLeftMouseDown)
	{
		if (CurrentTarget.IsValid() && bCurrentTargetCanBeCaught)
		{
			MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Confirmed target — %s"),
				*CurrentTarget->GetName());
			OnTargetConfirmed.Broadcast(CurrentTarget.Get());
			// 注意：此处不调用 EndTask()，由 GA 决定是否继续
		}
		else if (CurrentTarget.IsValid() && !bCurrentTargetCanBeCaught)
		{
			MF_LOG_WARNING(LogMFCatch,
				TEXT("AT_WaitPetTarget: Left click on uncatchable target — %s, ignoring."),
				*CurrentTarget->GetName());
		}
		else
		{
			MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Left click with no valid target, ignoring."));
		}
	}
	bWasLeftMouseDown = bLeftDown;

	// 右键 / ESC：取消
	const bool bRightDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
	if (bRightDown && !bWasRightMouseDown)
	{
		MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: Aim cancelled by player (right click)."));
		OnCancelled.Broadcast();
		// GA 收到此委托后会调用 EndTask()
	}
	bWasRightMouseDown = bRightDown;
}

void UAbilityTask_WaitPetTarget::OnDestroy(bool bInOwnerFinished)
{
	// 清除当前高亮描边
	if (AActor* Target = CurrentTarget.Get())
	{
		RemoveHighlight(Target);
		MF_LOG(LogMFCatch, TEXT("AT_WaitPetTarget: OnDestroy — cleared highlight on %s"),
			*Target->GetName());
	}

	// 隐藏鼠标指针（仅当没有其他需要显示的逻辑时）
	if (APlayerController* PC = GetOwnerController())
	{
		PC->bShowMouseCursor = false;
	}

	Super::OnDestroy(bInOwnerFinished);
}

// ============================================================
// 内部辅助
// ============================================================

APlayerController* UAbilityTask_WaitPetTarget::GetOwnerController() const
{
	if (Ability)
	{
		const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
		if (Info)
		{
			return Info->PlayerController.Get();
		}
	}
	MF_LOG_WARNING(LogMFCatch, TEXT("AT_WaitPetTarget: GetOwnerController returned null."));
	return nullptr;
}

AActor* UAbilityTask_WaitPetTarget::GetOwnerAvatar() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->GetAvatarActor();
	}
	return nullptr;
}

bool UAbilityTask_WaitPetTarget::DoWorldTrace(FVector& OutLandingPoint) const
{
	OutLandingPoint = FVector::ZeroVector;

	APlayerController* PC = GetOwnerController();
	if (!PC || !Config) return false;

	// ECC_Visibility：命中世界几何（地形、墙、地板等）
	FHitResult HitResult;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		OutLandingPoint = HitResult.ImpactPoint;
		return true;
	}

	// 备用：未命中几何时，将鼠标射线延伸到最大瞄准距离
	FVector WorldLoc, WorldDir;
	if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		OutLandingPoint = WorldLoc + WorldDir * Config->AimLineLength;
	}

	return false;
}

AActor* UAbilityTask_WaitPetTarget::DoPetTrace(bool& bOutCanCatch) const
{
	bOutCanCatch = false;

	APlayerController* PC = GetOwnerController();
	if (!PC || !Config) return nullptr;

	// 对 Pawn（宠物）类型对象做追踪
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	FHitResult HitResult;
	const bool bHit = PC->GetHitResultUnderCursorForObjects(
		ObjectTypes,
		false, // bTraceComplex
		HitResult);

	if (!bHit || !IsValid(HitResult.GetActor())) return nullptr;

	AActor* HitActor = HitResult.GetActor();

	// 必须实现 IMFCatchable 才算是可交互目标
	if (!HitActor->GetClass()->ImplementsInterface(UMFCatchable::StaticClass()))
	{
		return nullptr;
	}

	bOutCanCatch = IMFCatchable::Execute_CanBeCaught(HitActor, GetOwnerAvatar());
	return HitActor;
}

void UAbilityTask_WaitPetTarget::ApplyHighlight(AActor* Actor, int32 StencilValue)
{
	if (!IsValid(Actor)) return;

	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
		{
			Prim->SetRenderCustomDepth(true);
			Prim->SetCustomDepthStencilValue(StencilValue);
		}
	}
}

void UAbilityTask_WaitPetTarget::RemoveHighlight(AActor* Actor)
{
	if (!IsValid(Actor)) return;

	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
		{
			Prim->SetRenderCustomDepth(false);
			Prim->SetCustomDepthStencilValue(0);
		}
	}
}

void UAbilityTask_WaitPetTarget::DrawAimDebug(
	const FVector& FeetPos,
	const FVector& LandingPoint,
	bool           bHasPet) const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World) return;

	// 始终绘制从脚部到落点的预瞄线
	DrawDebugLine(
		World,
		FeetPos,
		LandingPoint,
		FColor::Cyan,
		false,   // bPersistentLines
		-1.f,    // LifeTime = 单帧
		0,       // DepthPriority
		2.f);    // Thickness

	// 落点球体：仅当 MF.Debug.CatchAim 开启时显示
	if (GMFCatchAimDebug)
	{
		// 绿色 = 光标下无宠物；黄色 = 有宠物
		const FColor SphereColor = bHasPet ? FColor::Yellow : FColor::Green;
		DrawDebugSphere(
			World,
			LandingPoint,
			20.f,  // Radius
			8,     // Segments
			SphereColor,
			false, // bPersistentLines
			-1.f); // LifeTime = 单帧
	}
#endif
}
