// Copyright ProjectMF. All Rights Reserved.

#include "MFCharacter.h"
#include "MFCamera.h"
#include "MFGameplayTags.h"
#include "MFInventoryComponent.h"
#include "MFGameMode.h"
#include "MFLog.h"
#include "MFPlayerConfig.h"
#include "MFPlayerController.h"
#include "MFCommandComponent.h"
#include "MFPlayerAttributeSet.h"
#include "MFPetBase.h"
#include "MFAttributeSetBase.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
#include "MFLootSubsystem.h"
#include "MFLootPickup.h"
#include "MFLootTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

AMFCharacter::AMFCharacter()
{
	// --- Spring Arm ---
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength         = 1200.f;
	CameraSpringArm->bDoCollisionTest        = false;
	CameraSpringArm->bUsePawnControlRotation = false;
	CameraSpringArm->bInheritPitch           = false;
	CameraSpringArm->bInheritYaw             = false;
	CameraSpringArm->bInheritRoll            = false;
	CameraSpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

	// --- Camera ---
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);

	// --- Camera Controller ---
	CameraController = CreateDefaultSubobject<UMFCameraController>(TEXT("CameraController"));
	CameraController->Initialize(CameraSpringArm, CameraComponent);

	// --- Inventory ---
	InventoryComponent = CreateDefaultSubobject<UMFInventoryComponent>(TEXT("InventoryComponent"));

	// --- Player-only AttributeSet ---
	PlayerAttributeSet = CreateDefaultSubobject<UMFPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
}

void AMFCharacter::BeginPlay()
{
	// 在 Super::BeginPlay 之前将 Config 值写入基类属性，
	// 这样 InitAbilitySystemComponent() 和头顶 Widget 初始化能正确读取。
	// Super 之前写背包配置（InitAbilitySystemComponent 不依赖背包）。
	// GAS（属性/技能/标签/受击闪光）改由 ApplyGASConfig 在 InitAbilitySystemComponent 内从 PlayerConfig 应用。
	if (PlayerConfig && InventoryComponent)
	{
		InventoryComponent->AIRegistry            = PlayerConfig->AIRegistry;
		InventoryComponent->MaxResourceSlots      = PlayerConfig->MaxResourceSlots;
		InventoryComponent->MaxPetSlots           = PlayerConfig->MaxPetSlots;
		InventoryComponent->SummonedPetTeamTags   = PlayerConfig->SummonedPetTeamTags;
		InventoryComponent->InitResourceSlots();
	}

	Super::BeginPlay();
}

void AMFCharacter::ApplyGASConfig()
{
	if (!PlayerConfig) return;

	ApplyAttributeInitData(PlayerConfig->InitAttributes);

	// 玩家技能不走"自动/手动"门禁（那是召唤宠物的机制），一律按手动授予（bAutoRelease=false）。
	for (const TSubclassOf<UMFGameplayAbilityBase>& AbilityClass : PlayerConfig->DefaultAbilities)
	{
		GrantAbility(AbilityClass, /*bAutoRelease=*/false);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!PlayerConfig->DefaultOwnedTags.IsEmpty())
		{
			ASC->AddLooseGameplayTags(PlayerConfig->DefaultOwnedTags);
		}
	}

	HitFlashDuration = PlayerConfig->HitFlashDuration;
}

void AMFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerConfig)
	{
		MF_LOG_WARNING(LogMFCharacter, TEXT("AMFCharacter: PlayerConfig is not set — no input bindings applied."));
		return;
	}

	if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (PlayerConfig->MoveAction)
		{
			EI->BindAction(PlayerConfig->MoveAction, ETriggerEvent::Triggered, this, &AMFCharacter::HandleMove);
		}
		if (PlayerConfig->PickAction)
		{
			EI->BindAction(PlayerConfig->PickAction, ETriggerEvent::Started,   this, &AMFCharacter::HandlePickStarted);
			EI->BindAction(PlayerConfig->PickAction, ETriggerEvent::Completed, this, &AMFCharacter::HandlePickCompleted);
		}
		if (PlayerConfig->RotateCameraAction)
		{
			EI->BindAction(PlayerConfig->RotateCameraAction, ETriggerEvent::Started, this, &AMFCharacter::HandleCameraRotate);
		}
		if (PlayerConfig->CatchPetAction)
		{
			EI->BindAction(PlayerConfig->CatchPetAction, ETriggerEvent::Completed, this, &AMFCharacter::HandleCatchPet);
		}
		else
		{
			MF_LOG_WARNING(LogMFCharacter, TEXT("AMFCharacter: PlayerConfig->CatchPetAction is not set — catch ability cannot be activated via input."));
		}

		// DEMO BEGIN — 召唤按键临时绑定（1-5 对应 slot 1-5）
		// TODO: GA_PetWheel 完成后，删除此段并改为激活轮盘 Ability
		for (int32 i = 0; i < PlayerConfig->SummonSlotActions.Num() && i < 5; ++i)
		{
			if (PlayerConfig->SummonSlotActions[i])
			{
				EI->BindAction(PlayerConfig->SummonSlotActions[i], ETriggerEvent::Started,
					this, &AMFCharacter::HandleSummonSlot, i + 1);
			}
		}
		// DEMO END

		if (PlayerConfig->StartBossBattleAction)
		{
			EI->BindAction(PlayerConfig->StartBossBattleAction, ETriggerEvent::Started,
				this, &AMFCharacter::HandleStartBossBattle);
		}

		// 指令系统：把命令模式 / 点击绑到 PlayerController 的 UMFCommandComponent。
		if (AMFPlayerController* MFPC = Cast<AMFPlayerController>(GetController()))
		{
			if (UMFCommandComponent* CmdComp = MFPC->GetCommandComponent())
			{
				if (PlayerConfig->CommandModeAction)
				{
					// 命令模式经 GA_CommandMode（倒计时 + CD）激活/取消，不再直连组件。
					EI->BindAction(PlayerConfig->CommandModeAction, ETriggerEvent::Started,
						this, &AMFCharacter::HandleCommandMode);
				}
				if (PlayerConfig->CommandClickAction)
				{
					EI->BindAction(PlayerConfig->CommandClickAction, ETriggerEvent::Started,
						CmdComp, &UMFCommandComponent::OnCommandClickStarted);
					EI->BindAction(PlayerConfig->CommandClickAction, ETriggerEvent::Completed,
						CmdComp, &UMFCommandComponent::OnCommandClickCompleted);
				}
				if (PlayerConfig->CarryPetAction)
				{
					// 一个键统一：就近友方宠——濒死→复活，存活→抱起；再按取消/放下（切换式）。
					EI->BindAction(PlayerConfig->CarryPetAction, ETriggerEvent::Started,
						this, &AMFCharacter::HandleCarryOrRevive);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Camera accessors
// ---------------------------------------------------------------------------

bool AMFCharacter::GetBillboardCameraForward(FVector& OutForward) const
{
	if (!CameraComponent) return false;
	OutForward = CameraComponent->GetForwardVector();
	return true;
}

float AMFCharacter::GetCameraYawForDirectionality() const
{
	return CameraController ? CameraController->GetSpriteOrientationYaw() : 0.f;
}

// ---------------------------------------------------------------------------
// Input handlers
// ---------------------------------------------------------------------------

void AMFCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, CameraSpringArm->GetRelativeRotation().Yaw, 0.f);
	const FVector  ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector  RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (!FMath::IsNearlyZero(MoveInput.X)) AddMovementInput(RightDir,   MoveInput.X);
	if (!FMath::IsNearlyZero(MoveInput.Y)) AddMovementInput(ForwardDir,  MoveInput.Y);
}

void AMFCharacter::HandlePickStarted()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(MFGameplayTags::Ability_Player_Pick));
	}
}

void AMFCharacter::HandlePickCompleted()
{
	if (AbilitySystemComponent)
	{
		const FGameplayTagContainer PickTag(MFGameplayTags::Ability_Player_Pick);
		AbilitySystemComponent->CancelAbilities(&PickTag);
	}
}

void AMFCharacter::HandleCameraRotate(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->SnapCamera(Value.Get<float>() >= 0.f ? 1 : -1);
	}
}

void AMFCharacter::HandleCatchPet()
{
	if (!AbilitySystemComponent)
	{
		MF_LOG_ERROR(LogMFCharacter, TEXT("AMFCharacter::HandleCatchPet — AbilitySystemComponent is null!"));
		return;
	}

	MF_LOG(LogMFCharacter, TEXT("AMFCharacter: CatchPet key released — trying to activate MF.Ability.Player.CatchPet."));

	const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(
		FGameplayTagContainer(MFGameplayTags::Ability_Player_CatchPet));

	if (!bActivated)
	{
		MF_LOG_WARNING(LogMFCharacter,
			TEXT("AMFCharacter: TryActivateAbilitiesByTag(CatchPet) returned false. "
			     "Check that GA_CatchPet is in DefaultAbilities and CatchConfig is assigned."));
	}
}

void AMFCharacter::HandleSummonSlot(int32 SlotIndex)
{
	if (!AbilitySystemComponent) return;

	FGameplayEventData EventData;
	EventData.EventMagnitude = static_cast<float>(SlotIndex);
	AbilitySystemComponent->HandleGameplayEvent(MFGameplayTags::Ability_Player_SummonPet, &EventData);
}

void AMFCharacter::HandleStartBossBattle()
{
	if (AMFGameMode* GM = Cast<AMFGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RequestBossPhase();
	}
}

void AMFCharacter::HandleCommandMode()
{
	if (!AbilitySystemComponent)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] HandleCommandMode: 无 ASC。"));
		return;
	}

	// 调试：确认输入到达了这里 + 该 tag 的技能是否已授予。
	bool bGranted = false;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(MFGameplayTags::Ability_Player_CommandMode))
		{
			bGranted = true;
			break;
		}
	}
	MF_LOG(LogMFAI, TEXT("[Command] HandleCommandMode 触发。CommandMode GA 已授予=%d"), bGranted ? 1 : 0);

	if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_CommandMode))
	{
		// 已在命令模式 → 取消 GA_CommandMode（提前退出，进 CD）。
		const FGameplayTagContainer CancelTags(MFGameplayTags::Ability_Player_CommandMode);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
		MF_LOG(LogMFAI, TEXT("[Command] 已在命令模式 → 取消（提前退出）。"));
	}
	else
	{
		// 否则激活；冷却中 / 死亡时会被 ActivationBlockedTags 自动挡掉。
		const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(MFGameplayTags::Ability_Player_CommandMode));
		MF_LOG(LogMFAI, TEXT("[Command] TryActivate(CommandMode)=%d（0 且已授予=被冷却/死亡挡；0 且未授予=需加进 PlayerConfig.DefaultAbilities）。"),
			bActivated ? 1 : 0);
	}
}

void AMFCharacter::HandleCarryOrRevive()
{
	if (!AbilitySystemComponent) return;

	// 已在抱 → 放下。
	if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_CarryingPet))
	{
		const FGameplayTagContainer CancelTags(MFGameplayTags::Ability_Player_CarryPet);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
		return;
	}
	// 已在复活 → 取消。
	if (AbilitySystemComponent->HasMatchingGameplayTag(MFGameplayTags::State_RevivingPet))
	{
		const FGameplayTagContainer CancelTags(MFGameplayTags::Ability_Player_RevivePet);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
		return;
	}

	// 优先：就近掉落物 → 拾取入包（满则弹回，见 TryPickUpInto）。
	if (AMFLootPickup* Loot = FindNearestLootPickupInReach())
	{
		Loot->TryPickUpInto(InventoryComponent);
		return;
	}

	// 否则：就近友方宠——濒死→复活，存活→抱起。
	const AMFPetBase* Pet = FindNearestFriendlyPetInReach();
	if (!Pet) return;

	const FGameplayTag AbilityTag = Pet->IsDowned()
		? MFGameplayTags::Ability_Player_RevivePet
		: MFGameplayTags::Ability_Player_CarryPet;
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
}

AMFPetBase* AMFCharacter::FindNearestFriendlyPetInReach() const
{
	UWorld* World = GetWorld();
	if (!World || !PlayerConfig) return nullptr;

	const float   ReachSq = PlayerConfig->CarryReach * PlayerConfig->CarryReach;
	const FVector Origin  = GetActorLocation();

	AMFPetBase* Best   = nullptr;
	float       BestSq = ReachSq;
	for (TActorIterator<AMFPetBase> It(World); It; ++It)
	{
		AMFPetBase* Pet = *It;
		if (!Pet) { continue; }

		const UAbilitySystemComponent* PetASC = Pet->GetAbilitySystemComponent();
		if (!PetASC || !PetASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player)) { continue; }
		if (PetASC->HasMatchingGameplayTag(MFGameplayTags::State_Carried)) { continue; } // 已被抱

		const float DistSq = FVector::DistSquared(Pet->GetActorLocation(), Origin);
		if (DistSq <= BestSq)
		{
			BestSq = DistSq;
			Best   = Pet;
		}
	}
	return Best;
}

AMFLootPickup* AMFCharacter::FindNearestLootPickupInReach() const
{
	UWorld* World = GetWorld();
	if (!World || !PlayerConfig) return nullptr;

	const float   ReachSq = PlayerConfig->CarryReach * PlayerConfig->CarryReach;
	const FVector Origin  = GetActorLocation();

	AMFLootPickup* Best   = nullptr;
	float          BestSq = ReachSq;
	for (TActorIterator<AMFLootPickup> It(World); It; ++It)
	{
		AMFLootPickup* Loot = *It;
		if (!Loot) { continue; }

		const float DistSq = FVector::DistSquared(Loot->GetActorLocation(), Origin);
		if (DistSq <= BestSq)
		{
			BestSq = DistSq;
			Best   = Loot;
		}
	}
	return Best;
}

void AMFCharacter::MFKillNextPet()
{
#if !UE_BUILD_SHIPPING
	if (!InventoryComponent) { return; }

	// 找第一只"存活（非濒死/非死亡）"出战宠，杀之。每次一只 → 按序击杀。
	for (AMFPetBase* Pet : InventoryComponent->GetActivePetActors())
	{
		if (!Pet || Pet->IsDowned()) { continue; }

		UAbilitySystemComponent* ASC = Pet->GetAbilitySystemComponent();
		if (!ASC || ASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead)) { continue; }

		// HP 清零 + 手动广播 OnDeath → 走完整死亡→濒死流程（HandleDeath + HandlePetDied）。
		ASC->SetNumericAttributeBase(UMFAttributeSetBase::GetHealthAttribute(), 0.f);
		if (UMFAttributeSetBase* Set = const_cast<UMFAttributeSetBase*>(ASC->GetSet<UMFAttributeSetBase>()))
		{
			Set->OnDeath.Broadcast();
		}
		MF_LOG(LogMFCharacter, TEXT("[GM] MFKillNextPet → 击杀 %s（进入濒死）。"), *Pet->GetName());
		return;
	}
	MF_LOG(LogMFCharacter, TEXT("[GM] MFKillNextPet：没有可击杀的存活出战宠。"));
#endif
}

void AMFCharacter::MFSpawnLoot(int32 ItemID, int32 Count)
{
#if !UE_BUILD_SHIPPING
	// 先查总表——未注册的物品生成后也捡不进背包，直接报错更早暴露配置问题。
	if (!UMFItemStatics::ContainsItem(UMFItemSettings::GetItemTable(), ItemID))
	{
		MF_LOG_ERROR(LogMFLoot, TEXT("[GM] MFSpawnLoot：物品总表中不存在 #%d。"), ItemID);
		return;
	}

	if (UMFLootSubsystem* Loot = GetWorld()->GetSubsystem<UMFLootSubsystem>())
	{
		FMFLootResult Result;
		Result.ItemID = ItemID;
		Result.Count  = FMath::Max(Count, 1);
		Loot->SpawnLoot({ Result }, GetActorLocation());
		MF_LOG(LogMFLoot, TEXT("[GM] MFSpawnLoot → #%d x%d。"), ItemID, Result.Count);
	}
#endif
}

void AMFCharacter::MFDropTable(const FString& TableAssetName)
{
#if !UE_BUILD_SHIPPING
	// AssetRegistry 按资产名查 UMFLootTable（不限路径，方便调试）。
	const FAssetRegistryModule& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByClass(UMFLootTable::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetName.ToString() != TableAssetName) { continue; }

		if (UMFLootSubsystem* Loot = GetWorld()->GetSubsystem<UMFLootSubsystem>())
		{
			Loot->DropFromTable(Cast<UMFLootTable>(Asset.GetAsset()), GetActorLocation());
			MF_LOG(LogMFLoot, TEXT("[GM] MFDropTable → %s roll 一次。"), *TableAssetName);
		}
		return;
	}
	MF_LOG_ERROR(LogMFLoot, TEXT("[GM] MFDropTable：找不到掉落表资产 %s。"), *TableAssetName);
#endif
}
