---
name: 远程攻击系统实现计划
description: 轻量投射物+落石两种远程攻击完整实现规划，供后续 agent 按序执行
type: project
originSessionId: a56e360f-1263-435e-a650-3da291ad3bee
---
# 远程攻击系统实现计划

**Why:** AI 需要远程攻击能力；投射物用 Subsystem+ISM 而非 SpawnActor，避免场景 Actor 爆炸。
**How to apply:** 按"实现顺序"逐步完成，每步完成后打勾。

---

## 一、整体分层

```
StateTree
  └─ FSTTask_ActivateAbilityByTag        ← 通用化的技能激活任务（取代 STTask_ActivateAttack 的硬编码）
        │ 激活 GA（通过可配置 AbilityTag）
        ▼
  UGA_AIRangedAttackBase                 ← 远程攻击抽象 GA 基类
    ├─ UGA_ThrowProjectile               ← 类型1：投掷武器（直线飞行）
    └─ UGA_FallingBoulder               ← 类型2：落石（从上方落下，落地结算）
        │ 类型1 → 注册到 Subsystem（无 Actor）
        │ 类型2 → Spawn AMFFallingRockActor（含警示 Decal）
        ▼
  UMFProjectileSubsystem                 ← WorldSubsystem，持有 struct 数组 + ISM Renderer
  AMFProjectileRenderer                  ← 场景唯一 Actor，按 Mesh 分 ISM 组件
```

---

## 二、文件清单

### 已完成 ✅

| 文件 | 说明 |
|------|------|
| `Source/ProjectMF/Projectile/Public/MFProjectileTypes.h` | 全部数据类型：枚举、结构体、委托、句柄 |
| `Source/ProjectMF/GAS/Public/MFGameplayTags.h` | 补充 `Ability_RangedAttack`、`State_RangedAttacking` 声明 |
| `Source/ProjectMF/GAS/Private/MFGameplayTags.cpp` | 补充两个新 tag 的 UE_DEFINE_GAMEPLAY_TAG_COMMENT 定义 |
| `Source/ProjectMF/ProjectMF.Build.cs` | 添加 `Projectile/Public` 和 `Projectile/Private` 包含路径 |

### 待实现（按序）

| 优先级 | 文件 | 说明 |
|--------|------|------|
| 1 | `Projectile/Public/MFProjectileRenderer.h` `.cpp` | ISM 渲染 Actor |
| 2 | `Projectile/Public/MFProjectileSubsystem.h` `.cpp` | WorldSubsystem，Tick 驱动模拟 |
| 3 | `GAS/Public/MFProjectileAttackData.h` | DataAsset：投掷武器配置 |
| 4 | `GAS/Public/MFFallingBoulderData.h` | DataAsset：落石配置 |
| 5 | `GAS/Public/GA_AIRangedAttackBase.h` `.cpp` | 远程攻击 GA 抽象基类 |
| 6 | `GAS/Public/GA_ThrowProjectile.h` `.cpp` | 类型1：投掷武器 GA |
| 7 | `Projectile/Public/MFFallingRockActor.h` `.cpp` | 落石 Actor（含 Decal） |
| 8 | `GAS/Public/GA_FallingBoulder.h` `.cpp` | 类型2：落石 GA |
| 9 | `AI/Public/STTask_ActivateAbilityByTag.h` `.cpp` | 通用 StateTree 任务 |

---

## 三、各组件详细规格

### 3.1 MFProjectileTypes.h（已完成）

关键类型速查：

```cpp
// 结算原因
enum class EMFProjectileResolveReason : uint8 { HitTarget, MaxRange, Cancelled }

// 结算结果（传给 GA 回调）
struct FMFProjectileResult { Reason; TWeakObjectPtr<AActor> HitActor; FVector FinalPosition; }

// 委托（非动态，单播）
DECLARE_DELEGATE_OneParam(FOnProjectileResolved, const FMFProjectileResult&)

// GA 持有的句柄
struct FMFProjectileHandle { uint32 UID; bool IsValid(); void Invalidate(); }

// GA → Subsystem 的输入
struct FMFProjectileLaunchParams {
    FVector Origin, Direction;         // Direction 由 GA 预计算
    float Speed, MaxRange, CollisionRadius;
    TObjectPtr<UStaticMesh> Mesh;
    TWeakObjectPtr<AActor> Instigator;
    TSubclassOf<UGameplayEffect> DamageGE;
    float DamageMultiplier;
    EAttackTargetFilter TargetFilter;
    FOnProjectileResolved OnResolved;  // 绑定后传入
}

// Subsystem 内部状态（slot 式，bActive=false 表示空闲）
struct FMFProjectileInstance {
    uint32 UID; bool bActive;
    FVector CurrentPos, Direction;
    float Speed, MaxRange, DistanceTraveled, CollisionRadius;
    TObjectPtr<UStaticMesh> Mesh; int32 ISMInstanceIndex; // -1=未分配
    TWeakObjectPtr<AActor> Instigator;
    TSubclassOf<UGameplayEffect> DamageGE; float DamageMultiplier;
    EAttackTargetFilter TargetFilter;
    FOnProjectileResolved OnResolved;
    void InitFromParams(const FMFProjectileLaunchParams&, uint32 UID);
    void Reset();
}
```

---

### 3.2 AMFProjectileRenderer

**文件：** `Projectile/Public/MFProjectileRenderer.h` / `Private/MFProjectileRenderer.cpp`
**父类：** `AActor`
**用途：** 场景中唯一一个 Actor，按 Mesh 类型管理多个 UInstancedStaticMeshComponent。

```cpp
UCLASS()
class AMFProjectileRenderer : public AActor
{
    // Key: Mesh 指针; Value: 对应的 ISM 组件
    UPROPERTY()
    TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> ISMMap;

    // 每个 ISM 的空闲槽索引池（隐藏不删，防止 RemoveInstance 打乱索引）
    TMap<UStaticMesh*, TArray<int32>> FreeSlotMap;

public:
    // 分配槽，写入初始 Transform，返回槽 Index
    int32  AcquireSlot(UStaticMesh* Mesh, const FTransform& InitialTransform);
    // 更新槽 Transform（每帧由 Subsystem 调用）
    void   UpdateSlot(UStaticMesh* Mesh, int32 SlotIndex, const FTransform& NewTransform);
    // 释放槽（Scale 设 0 隐藏，Index 回池）
    void   ReleaseSlot(UStaticMesh* Mesh, int32 SlotIndex);
}
```

**ISM 槽回收策略：**
- `AcquireSlot`：若 FreeSlotMap 有空闲 index → 取出并 `UpdateInstanceTransform(index, T, true)`；否则 `AddInstance(T)` → 记录新 index。
- `ReleaseSlot`：`UpdateInstanceTransform(index, FTransform(FQuat::Identity, Pos, FVector::ZeroVector), true)` 隐藏实例（Scale=0），index 加入 FreeSlotMap。
- **不调用** `RemoveInstance`，保持所有 index 稳定。

---

### 3.3 UMFProjectileSubsystem

**文件：** `Projectile/Public/MFProjectileSubsystem.h` / `Private/MFProjectileSubsystem.cpp`
**父类：** `UWorldSubsystem`，实现 `FTickableGameObject`
**依赖：** `MFProjectileTypes.h`，`MFProjectileRenderer.h`，`MFAttackTypes.h`

```cpp
UCLASS()
class UMFProjectileSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    // 模拟数据（slot 式数组）
    UPROPERTY()
    TArray<FMFProjectileInstance> Instances;

    TArray<int32> FreeInstanceSlots;  // 可复用的 Instances 下标

    // 渲染 Actor（Initialize 时 Spawn，World 销毁时自动清理）
    UPROPERTY()
    TObjectPtr<AMFProjectileRenderer> Renderer;

    uint32 NextUID = 1;  // 单调递增

public:
    // GA 调用：注册投射物，返回句柄
    FMFProjectileHandle Launch(const FMFProjectileLaunchParams& Params);

    // GA 在 EndAbility(cancelled) 时调用
    void Cancel(FMFProjectileHandle Handle);

    // UWorldSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

private:
    void TickInstance(FMFProjectileInstance& Inst, float DeltaTime);
    void ResolveInstance(FMFProjectileInstance& Inst, EMFProjectileResolveReason Reason, AActor* HitActor);
    bool PassesTargetFilter(const FMFProjectileInstance& Inst, AActor* Candidate) const;
    int32 FindInstanceSlotByUID(uint32 UID) const;
}
```

**Tick 内部逻辑（每帧，每个 bActive 实例）：**

```
Step = Direction * Speed * DeltaTime
NewPos = CurrentPos + Step

// 1. Sweep trace（球形，半径 CollisionRadius）
SweepResult = SweepSingleByChannel(CurrentPos, NewPos, Radius, ECC_Pawn)
if SweepResult.bBlockingHit:
    Candidate = SweepResult.GetActor()
    if Candidate && PassesTargetFilter(Inst, Candidate):
        ResolveInstance(Inst, HitTarget, Candidate)
        continue

// 2. 距离检查
DistanceTraveled += |Step|
if DistanceTraveled >= MaxRange:
    ResolveInstance(Inst, MaxRange, nullptr)
    continue

// 3. 更新位置 & ISM
CurrentPos = NewPos
Renderer->UpdateSlot(Mesh, ISMInstanceIndex, FTransform(Rotation, NewPos))
```

**ResolveInstance：**

```
Build FMFProjectileResult { Reason, HitActor, CurrentPos }
Renderer->ReleaseSlot(Mesh, ISMInstanceIndex)
OnResolved.ExecuteIfBound(Result)
Inst.Reset()
FreeInstanceSlots.Add(SlotIndex)
```

**PassesTargetFilter：** 与 `UGA_AIAttackBase::FilterTarget_Implementation` 逻辑一致：
- 检查目标 ASC 是否有 `State_Dead` tag（跳过）
- 按 `EAttackTargetFilter` 比较 `MF.Team.*` tag

---

### 3.4 DataAssets

#### UMFProjectileAttackData（投掷武器）

**文件：** `GAS/Public/MFProjectileAttackData.h`（无需 .cpp）
**父类：** `UDataAsset`

```cpp
UPROPERTY(EditDefaultsOnly) float AnimToSpawnDelay = 0.2f;  // 动画出手帧延迟
UPROPERTY(EditDefaultsOnly) float Speed = 800.f;             // cm/s
UPROPERTY(EditDefaultsOnly) float MaxRange = 1500.f;         // cm
UPROPERTY(EditDefaultsOnly) float CollisionRadius = 15.f;    // 碰撞球半径
UPROPERTY(EditDefaultsOnly) TObjectPtr<UStaticMesh> ProjectileMesh;
UPROPERTY(EditDefaultsOnly) bool bSplashOnMaxRange = false;  // 到达最远距离是否溅射
UPROPERTY(EditDefaultsOnly, meta=(EditCondition="bSplashOnMaxRange")) float SplashRadius = 80.f;
UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> DamageGE;
UPROPERTY(EditDefaultsOnly) float DamageMultiplier = 1.f;
UPROPERTY(EditDefaultsOnly) EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;
```

#### UMFFallingBoulderData（落石）

**文件：** `GAS/Public/MFFallingBoulderData.h`（无需 .cpp）
**父类：** `UDataAsset`

```cpp
UPROPERTY(EditDefaultsOnly) float AnimToSpawnDelay = 0.5f;  // 动画→生成落石延迟
UPROPERTY(EditDefaultsOnly) float FallHeight = 600.f;        // 初始高度（相对目标上方 cm）
UPROPERTY(EditDefaultsOnly) float FallSpeed = 900.f;         // 下落速度 cm/s
UPROPERTY(EditDefaultsOnly) float ImpactRadius = 150.f;      // 落地伤害半径
UPROPERTY(EditDefaultsOnly) TSubclassOf<AMFFallingRockActor> BoulderClass;
UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> DamageGE;
UPROPERTY(EditDefaultsOnly) float DamageMultiplier = 1.f;
UPROPERTY(EditDefaultsOnly) EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;
```

---

### 3.5 UGA_AIRangedAttackBase（GA 抽象基类）

**文件：** `GAS/Public/GA_AIRangedAttackBase.h` / `Private/GA_AIRangedAttackBase.cpp`
**父类：** `UMFGameplayAbilityBase`
**ActivationOwnedTags（BP 默认值中配置）：** `MF.Character.State.RangedAttacking`

```cpp
UCLASS(Abstract, Blueprintable)
class UGA_AIRangedAttackBase : public UMFGameplayAbilityBase
{
    UPROPERTY(EditDefaultsOnly, Category="RangedAttack|Config")
    TObjectPtr<UPaperZDAnimSequence> AttackAnim;

    // ActivateAbility 流程
    virtual void ActivateAbility(...) override;
    //   1. GetCurrentTarget() → 若无目标 EndAbility(cancelled)
    //   2. PlayAnimationOverride(AttackAnim, ...)
    //   3. SetTimer(AnimToSpawnDelay) → OnSpawnTimer()
    //      → SpawnProjectile(Target)

    // 子类实现：生成投射物或落石
    UFUNCTION(BlueprintNativeEvent)
    void SpawnProjectile(AActor* Target);
    virtual void SpawnProjectile_Implementation(AActor* Target) {}

    // 通用：伤害结算（复用 GA_AIAttackBase 的逻辑）
    void ApplyDamageToTarget(AActor* Target,
                              TSubclassOf<UGameplayEffect> DamageGE,
                              float DamageMultiplier);

    // 通用：目标过滤
    bool FilterTarget(AActor* Candidate, EAttackTargetFilter Filter) const;

protected:
    AMFAICharacter* GetAICharacter() const;
    AActor* GetCurrentTarget() const;  // 从 ThreatComponent 取

    FTimerHandle SpawnTimer;
    TWeakObjectPtr<AActor> CachedTarget;

    // 子类通过此设置延迟（从 DataAsset 读）
    float AnimToSpawnDelay = 0.2f;
}
```

---

### 3.6 UGA_ThrowProjectile（类型1：投掷武器）

**文件：** `GAS/Public/GA_ThrowProjectile.h` / `Private/GA_ThrowProjectile.cpp`
**父类：** `UGA_AIRangedAttackBase`
**AbilityTags（BP 配置）：** `MF.Ability.RangedAttack`

```cpp
UCLASS(Blueprintable)
class UGA_ThrowProjectile : public UGA_AIRangedAttackBase
{
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UMFProjectileAttackData> ProjectileData;

    virtual void ActivateAbility(...) override;
    // 从 ProjectileData 读 AnimToSpawnDelay 后调 Super

    virtual void SpawnProjectile_Implementation(AActor* Target) override;
    // 1. Build FMFProjectileLaunchParams:
    //      Origin    = GetAICharacter()->GetActorLocation()（或武器 Socket）
    //      Direction = (Target.Pos - Origin).GetSafeNormal()
    //      其余字段从 ProjectileData 填充
    //      OnResolved 绑定到 HandleProjectileResolved
    // 2. Subsystem->Launch(Params) → 存 ActiveHandle
    // GA 保持 Running（等 OnResolved 回调）

    void HandleProjectileResolved(const FMFProjectileResult& Result);
    // HitTarget  → FilterTarget → ApplyDamageToTarget → EndAbility
    // MaxRange   → if bSplashOnMaxRange: 球形 Overlap → ApplyDamage → EndAbility
    // Cancelled  → ActiveHandle.Invalidate()（已在 Cancel 里处理）

    virtual void EndAbility(...) override;
    // 若 ActiveHandle.IsValid() → Subsystem->Cancel(ActiveHandle)

    FMFProjectileHandle ActiveHandle;
}
```

---

### 3.7 AMFFallingRockActor（落石 Actor）

**文件：** `Projectile/Public/MFFallingRockActor.h` / `Private/MFFallingRockActor.cpp`
**父类：** `AActor`
**注：** 此 Actor 仍然 Spawn（因需要 Decal），但数量少（Boss 技能级别）。

```cpp
UCLASS()
class AMFFallingRockActor : public AActor
{
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RockMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDecalComponent> WarningDecal;

    // GA 调用，初始化后开始下落
    void InitFall(FVector GroundTarget, float InFallSpeed, float InImpactRadius);

    // 落地时广播（GA 绑定后结算伤害）
    DECLARE_DELEGATE_OneParam(FOnBoulderLanded, FVector /*LandedPos*/);
    FOnBoulderLanded OnLanded;

    virtual void Tick(float DeltaTime) override;
    // 每帧 Z -= FallSpeed * DeltaTime
    // 当 Z <= GroundTarget.Z → 触发 OnLanded，停止 Tick

private:
    FVector GroundTarget;
    float FallSpeed;
    float ImpactRadius;   // 仅传递给 GA 用，Actor 不自行结算
    bool bLanded = false;
}
```

---

### 3.8 UGA_FallingBoulder（类型2：落石）

**文件：** `GAS/Public/GA_FallingBoulder.h` / `Private/GA_FallingBoulder.cpp`
**父类：** `UGA_AIRangedAttackBase`
**AbilityTags（BP 配置）：** `MF.Ability.RangedAttack`

```cpp
UCLASS(Blueprintable)
class UGA_FallingBoulder : public UGA_AIRangedAttackBase
{
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UMFFallingBoulderData> BoulderData;

    virtual void SpawnProjectile_Implementation(AActor* Target) override;
    // 1. GroundPos = Target->GetActorLocation()（记录落点，不跟踪）
    // 2. SpawnLocation = GroundPos + FVector(0, 0, BoulderData->FallHeight)
    // 3. SpawnActor<AMFFallingRockActor>(SpawnLocation)
    // 4. Rock->InitFall(GroundPos, FallSpeed, ImpactRadius)
    // 5. Rock->OnLanded.BindUObject(this, &UGA_FallingBoulder::HandleLanded)

    void HandleLanded(FVector LandedPos);
    // 球形 Overlap at LandedPos（radius = ImpactRadius）
    // for each Overlap: FilterTarget → ApplyDamageToTarget
    // Rock->Destroy()
    // EndAbility

    virtual void EndAbility(...) override;
    // if SpawnedRock.IsValid() && !bLanded → SpawnedRock->Destroy()

    TWeakObjectPtr<AMFFallingRockActor> SpawnedRock;
    bool bLanded = false;
}
```

**伤害结算：** 落地时执行 `SphereOverlapActors(LandedPos, ImpactRadius)`，对每个通过 FilterTarget 的目标调 `ApplyDamageToTarget`。

---

### 3.9 FSTTask_ActivateAbilityByTag（通用 StateTree Task）

**文件：** `AI/Public/STTask_ActivateAbilityByTag.h` / `AI/Private/STTask_ActivateAbilityByTag.cpp`
**父类：** `FStateTreeAIActionTaskBase`

对应现有 `FSTTask_ActivateAttack` 的通用化版本（保留原 Task 不删，新增此 Task）。

```cpp
USTRUCT()
struct FSTTask_ActivateAbilityByTag_InstanceData
{
    // 编辑器配置
    UPROPERTY(EditAnywhere) FGameplayTag AbilityTag;     // 要激活的能力 Tag
    UPROPERTY(EditAnywhere) FGameplayTag ActiveStateTag; // 判断能力是否结束的状态 Tag

    // 运行时缓存
    FGameplayAbilitySpecHandle ActiveSpecHandle;
}

USTRUCT(DisplayName = "MF Activate Ability By Tag")
struct FSTTask_ActivateAbilityByTag : public FStateTreeAIActionTaskBase
{
    // EnterState: 查找 AbilityTag 对应技能 → TryActivateAbility → Running
    // Tick:       ASC 无 ActiveStateTag → Succeeded（技能正常结束）
    // ExitState:  ASC 还有 ActiveStateTag → CancelAbilityHandle（被打断）
}
```

**使用方式：**
- 激活近战攻击：`AbilityTag = MF.Ability.Attack`，`ActiveStateTag = MF.Character.State.Attacking`
- 激活远程攻击：`AbilityTag = MF.Ability.RangedAttack`，`ActiveStateTag = MF.Character.State.RangedAttacking`

---

## 四、GameplayTag 说明

| Tag（命名空间 MFGameplayTags::） | 字符串 | 用途 |
|---|---|---|
| `Ability_RangedAttack` | `MF.Ability.RangedAttack` | 远程攻击 GA 的 AbilityTag |
| `State_RangedAttacking` | `MF.Character.State.RangedAttacking` | 远程攻击进行中的状态 Tag |

两者已在 `MFGameplayTags.h` 声明、`MFGameplayTags.cpp` 定义完毕。

**GA 蓝图必须配置：**
- `AbilityTags` 添加 `MF.Ability.RangedAttack`
- `ActivationOwnedTags` 添加 `MF.Character.State.RangedAttacking`

---

## 五、伤害流程

与近战攻击完全一致，复用 `UGA_AIAttackBase::ApplyDamageToTarget_Implementation` 的逻辑：

```
Build FGameplayEffectSpecHandle(DamageGE, InstigatorASC)
SetSetByCallerMagnitude(MFGameplayTags::Attack_Data_Damage, DamageMultiplier)
TargetASC->ApplyGameplayEffectSpecToSelf(Spec)
```

`FilterTarget` 逻辑：
```
if TargetASC has State_Dead → skip
if TargetFilter == EnemyOnly  → caster has Team_Player && target has Team_Enemy (or vice versa)
if TargetFilter == AllyOnly   → same team
if TargetFilter == All        → pass
```

---

## 六、实现顺序

1. ✅ `MFProjectileTypes.h` — 已完成
2. ⬜ `MFProjectileRenderer` — ISM 渲染 Actor（无依赖）
3. ⬜ `MFProjectileSubsystem` — 依赖 Types + Renderer
4. ⬜ `MFProjectileAttackData` / `MFFallingBoulderData` — DataAsset，无依赖
5. ⬜ `GA_AIRangedAttackBase` — 依赖 DataAsset 接口概念
6. ⬜ `GA_ThrowProjectile` — 依赖 Subsystem + ProjectileAttackData + Base
7. ⬜ `MFFallingRockActor` — 依赖较少，可并行
8. ⬜ `GA_FallingBoulder` — 依赖 FallingRockActor + BoulderData + Base
9. ⬜ `STTask_ActivateAbilityByTag` — 依赖新 Tag，可并行

---

## 七、关键约定

- **投射物不 Homing**：Direction 在 Launch 时一次性计算，之后直线飞行。
- **落石目标位置不跟踪**：`GroundPos` 在 SpawnProjectile 时记录，目标移走仍在原位落下。
- **Subsystem 不做伤害**：伤害由 GA 在 OnResolved 回调中施加，Subsystem 只管物理模拟和碰撞检测。
- **ISM 槽不删除**：`ReleaseSlot` 把 Scale 设为 0 隐藏，保持 index 稳定；空闲槽入池复用。
- **SweepTrace 防穿透**：用 `SweepSingleByChannel` 而非 Overlap，高速投射物不漏检。
