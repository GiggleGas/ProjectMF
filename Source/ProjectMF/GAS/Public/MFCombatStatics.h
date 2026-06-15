// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MFAttackTypes.h"            // FMFOnHitEffect
#include "MFAreaEffectSubsystem.h"    // FMFAreaHandle
#include "MFCombatStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UMFAreaEffectData;

/**
 * 战斗施加的共享静态：伤害、命中附加效果、生成区域场。
 * 近战 / 远程 GA、区域子系统、以及未来的 combo / 道具 等都复用这里，避免重复。
 */
UCLASS()
class PROJECTMF_API UMFCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 施加一次瞬时伤害：FinalMag = 来源 Attack × DamageMultiplier × 来源 OutgoingDamageMultiplier，
	 * 写入 SetByCaller(MF.Attack.Data.Damage) 后施加 DamageGE。Source/Target/DamageGE 任一为空则跳过。
	 */
	static void ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	                        TSubclassOf<UGameplayEffect> DamageGE, float DamageMultiplier);

	/**
	 * 施加一组命中附加效果（按 Kind 查 UMFCombatEffectSettings 映射表解析 GE 与数值键，
	 * 逐条 roll 概率、写 Duration/Magnitude 的 SetByCaller 后施加）。
	 */
	static void ApplyOnHitEffects(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	                              const TArray<FMFOnHitEffect>& Effects, float Level = 1.f);

	/**
	 * 生成一个带来源的"场"（统一入口）。从 Instigator 取 World 找区域子系统并注册。
	 * 任何地方都可调用：技能 / combo / 道具 / 投掷落点 / 撼地落点 等。
	 */
	UFUNCTION(BlueprintCallable, Category = "MF|Combat", meta = (DefaultToSelf = "Instigator"))
	static FMFAreaHandle SpawnAreaEffect(AActor* Instigator, const UMFAreaEffectData* AreaData, const FVector& Location);
};
