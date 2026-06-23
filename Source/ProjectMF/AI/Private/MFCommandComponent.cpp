// Copyright ProjectMF. All Rights Reserved.

#include "MFCommandComponent.h"

#include "MFCommandTypes.h"
#include "MFPetCommandComponent.h"
#include "MFPetBase.h"
#include "MFGameplayTags.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

UMFCommandComponent::UMFCommandComponent()
{
	// 命令模式下每帧画选中高亮（debug）。Tick 体内对非命令模式早退。
	PrimaryComponentTick.bCanEverTick = true;
}

void UMFCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCommandModeActive && SelectedPet.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			DrawDebugSphere(World, SelectedPet->GetActorLocation(), 60.f, 16, FColor::Green, false, 0.f, 0, 2.f);
		}
	}
}

// ============================================================================
// 命令模式
// ============================================================================

void UMFCommandComponent::ToggleCommandMode()
{
	if (bCommandModeActive) ExitCommandMode();
	else                    EnterCommandMode();
}

void UMFCommandComponent::EnterCommandMode()
{
	bCommandModeActive = true;

	if (APlayerController* PC = GetPC())
	{
		bSavedShowMouseCursor   = PC->bShowMouseCursor;
		PC->bShowMouseCursor    = true;
		PC->bEnableClickEvents  = true;
		PC->bEnableMouseOverEvents = true;
	}

	// M4：在此 SetGlobalTimeDilation 放慢世界。
	MF_LOG(LogMFAI, TEXT("[Command] 进入命令模式"));
}

void UMFCommandComponent::ExitCommandMode()
{
	bCommandModeActive = false;
	bPressedOnPet      = false;
	ClearSelection();

	if (APlayerController* PC = GetPC())
	{
		PC->bShowMouseCursor   = bSavedShowMouseCursor;
		PC->bEnableClickEvents = false;
	}

	// M4：在此恢复 SetGlobalTimeDilation(1.0)。
	MF_LOG(LogMFAI, TEXT("[Command] 退出命令模式"));
}

// ============================================================================
// 点击处理
// ============================================================================

void UMFCommandComponent::OnCommandClickStarted()
{
	if (!bCommandModeActive) return;

	AMFPetBase* Pet = GetPetUnderCursor();
	if (!Pet)
	{
		ClearSelection();
		bPressedOnPet = false;
		return;
	}

	const double Now = FPlatformTime::Seconds(); // 真实时间，林克时间膨胀下双击仍准
	const bool bDoubleClick = (LastClickedPet.Get() == Pet) && (Now - LastClickRealTime <= DoubleClickThreshold);

	if (bDoubleClick)
	{
		IssuePetReadyManualSkill(Pet);
		LastClickedPet = nullptr; // 重置，避免三连击重复释放
		bPressedOnPet  = false;
		return;
	}

	SelectPet(Pet);
	bPressedOnPet     = true;
	LastClickRealTime = Now;
	LastClickedPet    = Pet;
	if (APlayerController* PC = GetPC())
	{
		PC->GetMousePosition(PressScreenPos.X, PressScreenPos.Y);
	}
}

void UMFCommandComponent::OnCommandClickCompleted()
{
	if (!bCommandModeActive)
	{
		bPressedOnPet = false;
		return;
	}

	if (bPressedOnPet && SelectedPet.IsValid())
	{
		FVector2D NowPos = FVector2D::ZeroVector;
		APlayerController* PC = GetPC();
		if (PC && PC->GetMousePosition(NowPos.X, NowPos.Y))
		{
			// 移动超过阈值 → 判定为拖拽 → 下达移动指令。
			if (FVector2D::Distance(NowPos, PressScreenPos) > DragThreshold)
			{
				FVector Ground;
				if (GetGroundPointUnderCursor(Ground))
				{
					IssuePetMove(SelectedPet.Get(), Ground);
				}
			}
		}
	}

	bPressedOnPet = false;
}

// ============================================================================
// 拾取 / 指令
// ============================================================================

AMFPetBase* UMFCommandComponent::GetPetUnderCursor() const
{
	APlayerController* PC = GetPC();
	if (!PC) return nullptr;

	FHitResult Hit;
	if (!PC->GetHitResultUnderCursor(ECC_Pawn, false, Hit))
	{
		return nullptr;
	}

	AMFPetBase* Pet = Cast<AMFPetBase>(Hit.GetActor());
	if (!Pet) return nullptr;

	// 仅召唤宠物可被指挥。
	UAbilitySystemComponent* ASC = Pet->GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(MFGameplayTags::Pet_Summoned))
	{
		return Pet;
	}
	return nullptr;
}

bool UMFCommandComponent::GetGroundPointUnderCursor(FVector& OutLocation) const
{
	APlayerController* PC = GetPC();
	if (!PC) return false;

	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		OutLocation = Hit.ImpactPoint;
		return true;
	}
	return false;
}

void UMFCommandComponent::SelectPet(AMFPetBase* Pet)
{
	SelectedPet = Pet;
	MF_LOG(LogMFAI, TEXT("[Command] 选中宠物 %s"), *GetNameSafe(Pet));
}

void UMFCommandComponent::ClearSelection()
{
	SelectedPet = nullptr;
}

void UMFCommandComponent::IssuePetMove(AMFPetBase* Pet, const FVector& Location)
{
	UMFPetCommandComponent* Cmd = Pet ? Pet->FindComponentByClass<UMFPetCommandComponent>() : nullptr;
	if (!Cmd) return;

	FMFPetCommand Command;
	Command.Type           = EMFCommandType::MoveTo;
	Command.TargetLocation = Location;
	Cmd->IssueCommand(Command);
}

void UMFCommandComponent::IssuePetSkill(AMFPetBase* Pet, FGameplayTag SkillTag)
{
	UMFPetCommandComponent* Cmd = Pet ? Pet->FindComponentByClass<UMFPetCommandComponent>() : nullptr;
	if (!Cmd || !SkillTag.IsValid()) return;

	FMFPetCommand Command;
	Command.Type     = EMFCommandType::CastSkill;
	Command.SkillTag = SkillTag; // 目标由技能自取（当前威胁目标）
	Cmd->IssueCommand(Command);
}

void UMFCommandComponent::IssuePetReadyManualSkill(AMFPetBase* Pet)
{
	UMFPetCommandComponent* Cmd = Pet ? Pet->FindComponentByClass<UMFPetCommandComponent>() : nullptr;
	if (!Cmd) return;

	const FGameplayTag Tag = Cmd->GetReadyManualSkillTag();
	if (!Tag.IsValid())
	{
		MF_LOG(LogMFAI, TEXT("[Command] %s 没有可用的手动技能（手动且不在冷却）。"), *GetNameSafe(Pet));
		return;
	}
	IssuePetSkill(Pet, Tag);
}

APlayerController* UMFCommandComponent::GetPC() const
{
	return Cast<APlayerController>(GetOwner());
}

// ============================================================================
// 调试入口
// ============================================================================

void UMFCommandComponent::DebugIssuePetMove(AActor* Pet, const FVector& Location)
{
	IssuePetMove(Cast<AMFPetBase>(Pet), Location);
}

void UMFCommandComponent::DebugIssuePetSkill(AActor* Pet, FGameplayTag SkillTag)
{
	IssuePetSkill(Cast<AMFPetBase>(Pet), SkillTag);
}

void UMFCommandComponent::DebugMoveNearestPet(float X, float Y, float Z)
{
	if (AMFPetBase* Pet = FindNearestPet())
	{
		IssuePetMove(Pet, FVector(X, Y, Z));
	}
}

void UMFCommandComponent::DebugSkillNearestPet(const FString& SkillTag)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*SkillTag), /*ErrorIfNotFound=*/false);
	if (!Tag.IsValid())
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] MFDebugPetSkill: invalid gameplay tag '%s'."), *SkillTag);
		return;
	}
	if (AMFPetBase* Pet = FindNearestPet())
	{
		IssuePetSkill(Pet, Tag);
	}
}

AMFPetBase* UMFCommandComponent::FindNearestPet() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	const APlayerController* PC = GetPC();
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	const FVector Origin = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	AMFPetBase* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AMFPetBase> It(World); It; ++It)
	{
		AMFPetBase* Pet = *It;
		const float DistSq = FVector::DistSquared(Pet->GetActorLocation(), Origin);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Pet;
		}
	}

	if (!Nearest)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] FindNearestPet: no AMFPetBase found in world."));
	}
	return Nearest;
}
