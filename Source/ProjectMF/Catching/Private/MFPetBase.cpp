// Copyright ProjectMF. All Rights Reserved.

#include "MFPetBase.h"
#include "MFPetConfig.h"
#include "MFItemTypes.h"
#include "MFAttributeSetBase.h"
#include "MFPetAIController.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFHomeAnchorComponent.h"
#include "MFPetCommandComponent.h"
#include "MFLog.h"
#include "MFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "PaperZDAnimationComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

// ============================================================
// 构造
// ============================================================

AMFPetBase::AMFPetBase()
{
	// AMFCharacterBase 的 Tick 已足够，宠物不需要额外的 ActorTick 逻辑
	// 具体 AI 行为由 StateTree / Mass 驱动

	// 设置默认 Controller 类，使 AutoPossessAI 自动 Possess AMFPetAIController。
	// AMFSpawnAIManager 之后调用 RunStateTree() 绑定具体资产。
	AIControllerClass = AMFPetAIController::StaticClass();

	// 玩家指令载体（指令系统 M0）：仅宠物持有。
	CommandComp = CreateDefaultSubobject<UMFPetCommandComponent>(TEXT("CommandComp"));
}

// ============================================================
// 配置注入
// ============================================================

void AMFPetBase::ApplyPetConfig(const UMFPetConfig* Config)
{
	if (!Config)
	{
		return;
	}

	CachedPetConfig = Config;

	// 1. 通用 AI 配置（GAS / OverheadWidget / HitFlash）— 由基类统一处理
	ApplyAIConfig(Config);

	// 2. 动画（最先执行，让后续 StateTree 启动时 AnimInstance 已就位）
	if (Config->AnimInstanceClass && AnimationComponent)
	{
		AnimationComponent->SetAnimInstanceClass(Config->AnimInstanceClass);
	}

	// 3. 身份
	if (!Config->AIConfigID.IsNone())
	{
		AIConfigID = Config->AIConfigID;
	}

	// 4. 感知（Radar 须先于 Threat 写入，Threat 内部校验 EngagementRadius <= SensingRadius）
	if (UMFRadarSensingComponent* MyRadarComp = FindComponentByClass<UMFRadarSensingComponent>())
	{
		MyRadarComp->ApplyConfig(Config->RadarConfig);
	}
	if (UMFThreatComponent* MyThreatComp = FindComponentByClass<UMFThreatComponent>())
	{
		MyThreatComp->ApplyConfig(Config->ThreatConfig);
	}

	// 5. 行为：出生锚点/回家配置（家点由组件 BeginPlay 自记，此处只写半径配置）
	if (UMFHomeAnchorComponent* MyAnchorComp = FindComponentByClass<UMFHomeAnchorComponent>())
	{
		MyAnchorComp->ApplyConfig(Config->AnchorConfig);
	}
}

// ============================================================
// IMFCatchable 实现
// ============================================================

bool AMFPetBase::CanBeCaught_Implementation(const AActor* Catcher) const
{
	if (bIsCaught)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::CanBeCaught — %s is already caught, returning false."),
			*GetName());
		return false;
	}

	MF_LOG(LogMFCatch,
		TEXT("AMFPetBase::CanBeCaught — %s is available to be caught by %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	return true;
}

void AMFPetBase::OnCaught_Implementation(AActor* Catcher)
{
	bIsCaught = true;

	MF_LOG(LogMFCatch,
		TEXT("AMFPetBase::OnCaught — %s has been caught by %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	// Actor 的销毁由调用方（GA_CatchPet::EndCatch）在序列化完成后负责。
	// 子类在 Super:: 之后可播放收服动画/粒子，动效结束前不要提前 Destroy。
}

// ============================================================
// 序列化
// ============================================================

void AMFPetBase::SerializeToInstance(FMFPetInstance& InOutInstance) const
{
	InOutInstance.AIConfigID = AIConfigID;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::SerializeToInstance — ASC not found on %s, AttributeSnapshot will be empty."),
			*GetName());
		return;
	}

	bool bFound = false;
	InOutInstance.AttributeSnapshot.Add(TEXT("Health"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetHealthAttribute(), bFound));
	InOutInstance.AttributeSnapshot.Add(TEXT("MaxHealth"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMaxHealthAttribute(), bFound));
	InOutInstance.AttributeSnapshot.Add(TEXT("MoveSpeed"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMoveSpeedAttribute(), bFound));
}

void AMFPetBase::RestoreFromInstance(const FMFPetInstance& Instance)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::RestoreFromInstance — ASC not found on %s, attributes not restored."),
			*GetName());
		return;
	}

	auto Restore = [&](const TCHAR* Key, const FGameplayAttribute& Attr)
	{
		if (const float* Val = Instance.AttributeSnapshot.Find(Key))
		{
			ASC->SetNumericAttributeBase(Attr, *Val);
		}
	};

	Restore(TEXT("Health"),    UMFAttributeSetBase::GetHealthAttribute());
	Restore(TEXT("MaxHealth"), UMFAttributeSetBase::GetMaxHealthAttribute());
	Restore(TEXT("MoveSpeed"), UMFAttributeSetBase::GetMoveSpeedAttribute());
}

void AMFPetBase::OnCatchFailed_Implementation(AActor* Catcher)
{
	MF_LOG_WARNING(LogMFCatch,
		TEXT("AMFPetBase::OnCatchFailed — %s escaped from %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	// TODO: 切换到逃跑 Behavior Tree 节点或触发逃跑 GE/Tag
	// 子类在 Super::OnCatchFailed_Implementation(Catcher) 之后添加具体逻辑
}

// ============================================================
// 被抱起（GA_CarryPet）
// ============================================================

void AMFPetBase::BeginCarried()
{
	// 抱起瞬间在移动 → 打断 StateTree（落下时恢复）；静止则不打断。
	if (!GetVelocity().IsNearlyZero())
	{
		if (AMFPetAIController* AIC = Cast<AMFPetAIController>(GetController()))
		{
			AIC->StopStateTree();
			bCarryInterruptedStateTree = true;
		}
	}

	// 停移动 + 停自身在放的技能（被抱期间不行动）。
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->DisableMovement();
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
		ASC->AddLooseGameplayTag(MFGameplayTags::State_Carried);
	}

	// 关碰撞：宠物免伤（不在 overlap/trace 里）+ 子弹/area 穿过去只命中玩家 + 不和玩家胶囊顶撞。
	SetActorEnableCollision(false);
}

void AMFPetBase::EndCarried(const FVector& DropLocation)
{
	// 先落点（此时碰撞仍关，避免落地瞬间挤压），再开碰撞。
	SetActorLocation(DropLocation, /*bSweep=*/false);
	SetActorEnableCollision(true);

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::State_Carried);
	}

	// 抱起时打断过 StateTree → 落下重启 AI。
	if (bCarryInterruptedStateTree)
	{
		if (AMFPetAIController* AIC = Cast<AMFPetAIController>(GetController()))
		{
			AIC->ResumeStateTree();
		}
		bCarryInterruptedStateTree = false;
	}
}

// ============================================================
// 濒死 / 复活（GA_RevivePet）
// ============================================================

void AMFPetBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDowned) return;

	if (bBeingRevived)
	{
		// 复活读条（被抱起期间，bleed-out 暂停）。
		ReviveRemaining -= DeltaSeconds;
		if (ReviveRemaining <= 0.f)
		{
			ReviveFromDowned();
		}
	}
	else
	{
		// 濒死倒计时，归零未被救 → 真死。
		BleedOutRemaining -= DeltaSeconds;
		if (BleedOutRemaining <= 0.f)
		{
			TrueDeath();
			return;
		}
	}

	DrawDownedBar();
}

void AMFPetBase::EnterDowned()
{
	if (bDowned) return;
	bDowned           = true;
	bBeingRevived     = false;
	BleedOutRemaining = BleedOutDuration;

	// 关碰撞：濒死宠免伤 + 可被玩家抱起救。基类 HandleDeath 已停技能/禁移动。
	SetActorEnableCollision(false);

	MF_LOG(LogMFCatch, TEXT("%s 濒死，%.0fs 内未救将真死。"), *GetName(), BleedOutDuration);
}

void AMFPetBase::BeginRevive(float InReviveDuration)
{
	if (!bDowned) return;
	bBeingRevived   = true;
	ReviveTotal     = FMath::Max(InReviveDuration, 0.1f);
	ReviveRemaining = ReviveTotal;
}

void AMFPetBase::CancelRevive()
{
	// 放下：恢复 bleed-out 从剩余继续倒数。
	bBeingRevived = false;
}

void AMFPetBase::TrueDeath()
{
	bDowned       = false;
	bBeingRevived = false;
	MF_LOG(LogMFCatch, TEXT("%s 濒死读条耗尽 → 真死。"), *GetName());
	OnTrueDeath.Broadcast(); // Inventory 销毁 Actor + 永久损失
}

void AMFPetBase::ReviveFromDowned()
{
	bDowned       = false;
	bBeingRevived = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::State_Dead);
		bool bFound;
		const float MaxHP = ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMaxHealthAttribute(), bFound);
		if (bFound)
		{
			ASC->SetNumericAttributeBase(UMFAttributeSetBase::GetHealthAttribute(), MaxHP);
		}
	}

	SetActorEnableCollision(true);
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
	}

	MF_LOG(LogMFCatch, TEXT("%s 复活回场。"), *GetName());
	OnRevived.Broadcast(); // Inventory 标记实例回出战
}

void AMFPetBase::DrawDownedBar() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World) return;

	const float HeadZ = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f : 100.f;
	const FVector Center = GetActorLocation() + FVector(0.f, 0.f, HeadZ);

	const float HalfW = 40.f;
	const FVector Left  = Center - FVector(HalfW, 0.f, 0.f);
	const FVector Right = Center + FVector(HalfW, 0.f, 0.f);

	const float Frac = bBeingRevived
		? FMath::Clamp(1.f - ReviveRemaining / FMath::Max(ReviveTotal, 0.1f), 0.f, 1.f) // 复活：填充
		: FMath::Clamp(BleedOutRemaining / FMath::Max(BleedOutDuration, 0.1f), 0.f, 1.f); // 濒死：收缩
	const FColor Color = bBeingRevived ? FColor::Green : FColor::Red;

	// 底条（灰）+ 进度条（红=濒死剩余 / 绿=复活进度）。
	DrawDebugLine(World, Left, Right, FColor(60, 60, 60), false, -1.f, 0, 6.f);
	DrawDebugLine(World, Left, Left + (Right - Left) * Frac, Color, false, -1.f, 0, 6.f);
#endif
}
