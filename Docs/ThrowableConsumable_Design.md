# 投掷消耗品 / 纯 Sprite 投射物子系统 设计方案

> 2026-07-10 立项 ｜ 修订 1：统一进 `UMFProjectileSubsystem` ｜ **修订 2（定稿方向）：全 sprite，废弃 ISM**
> 需求：给玩家投掷物技能——点物品 → 点目标位置 → **抛物线**扔出**可配 sprite** 的投掷物，落点生成回血场；有飞行速度和最远距离配置。
> 架构决策（2026-07-10 用户拍板）：**未来所有投射物一律 sprite，不用 ISM/StaticMesh**。故不做"mesh + sprite 双后端"，直接把子系统渲染层**从 ISM 重构成纯 `PaperSpriteComponent` 池**，全项目投射物（投掷/落石/弹幕）统一走 sprite。

---

## 1. 定位：一条 sprite 渲染路，贯穿所有投射物

全项目对 mesh 的依赖只有一处：`UMFRangedAttackDataBase::ProjectileMesh`（被投掷 `GA_ThrowProjectile` / 落石 `FallingBoulder` / 弹幕 `BulletCurtain` 三类共用）→ `LaunchParams.Mesh` → `Instance.Mesh` → `AMFProjectileRenderer` 的 ISM。

「全 sprite」= 把这条链的 `UStaticMesh` 全换成 `UPaperSprite`，Renderer 从「ISM 批渲」重构成「`PaperSpriteComponent` 池」。**不保留 ISM、不做双后端**——比双后端方案更简单（一条渲染路、无类型分支），也与项目 2D 美术风格一致（角色、掉落物都是 sprite，投射物也 sprite）。

## 2. 子系统三处改动

### 2.1 渲染：Renderer 重构为 `PaperSpriteComponent` 池（删 ISM）

`AMFProjectileRenderer` 内部整个换掉：

```cpp
// 删除：ISMMap（TMap<UStaticMesh*, ISM>）、GetOrCreateISM、按 mesh 分组
// 改为：一个可复用的 PaperSpriteComponent 池
UPROPERTY() TArray<TObjectPtr<UPaperSpriteComponent>> Pool;   // 挂在本 Actor 上
TArray<int32> FreeSlots;                                       // 空闲组件索引

int32 AcquireSlot(UPaperSprite* Sprite, const FTransform& Xf); // 取空闲组件→SetSprite→显示→返回 index
void  UpdateSlot(int32 Slot, const FTransform& Xf);            // 每 tick 更新 transform（含 billboard）
void  ReleaseSlot(int32 Slot);                                 // 隐藏 + 归还
```

- slot 复用机制与原 ISM 版同构（空闲池、稳定索引），只是载体从「ISM instance」换成「池里的 PaperSpriteComponent」。
- **billboard**：子系统每帧更新 slot transform 时让 sprite 面向相机（复用掉落物 `UpdateBillboard` 同款算法）。
- 组件按需增长、只增不删（同 ISM slot 池策略），避免运行时反复 create/destroy。

### 2.2 弹道：方向×速度 → 速度矢量 + 重力

支持抛物线，同时直线是它的特例：

```cpp
// LaunchParams / Instance：Direction(单位)+Speed(标量) → 换成
FVector Velocity = FVector::ZeroVector;  // cm/s，含方向与大小
float   GravityZ = 0.f;                   // cm/s²，0=直线，>0=抛物线

// tick 积分：
Inst.Velocity.Z -= Inst.GravityZ * dt;    // GravityZ=0 → 直线
Inst.CurrentPos += Inst.Velocity * dt;
Inst.DistanceTraveled += (Inst.Velocity * dt).Size();
```

- 投掷/弹幕/落石现有的直线行为：`Velocity=Dir*Speed, GravityZ=0`，**保持不变**。
- 玩家投掷消耗品：入口反算初速度（§4），`GravityZ>0` → 抛物线落点击点。

### 2.3 结算：加落点 reason

```cpp
enum class EMFProjectileResolveReason { HitTarget, HitGround, MaxRange, Cancelled };
//                                                  ↑ 新增：抛物线下坠命中地面/到达落点
```

投掷物落地 → `OnResolved(HitGround)` → 回调 `SpawnAreaEffect`；战斗物命中目标 → `HitTarget` → 判伤（不变）。结算全在调用方回调里，子系统只上报。

## 3. 数据配置

### 3.1 现有攻击基类：mesh → sprite

```cpp
// UMFRangedAttackDataBase：
- TObjectPtr<UStaticMesh> ProjectileMesh;      // 删
+ TObjectPtr<UPaperSprite> ProjectileSprite;   // 换
```

→ 现有投掷/落石/弹幕的 DataAsset **重配 sprite**（编辑器活，见 §5 步 4）。

### 3.2 新增 `UMFThrowableData`（玩家投掷参数）

```cpp
UCLASS(BlueprintType)
class UMFThrowableData : public UDataAsset
{
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UPaperSprite> FlightSprite;        // 飞行外观
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float Speed = 900.f;     // 水平速度 cm/s
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float MaxRange = 1500.f; // 最远落点 cm
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=0))   float ArcHeight = 250.f; // 抛物线顶高 cm
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=0.1)) float SpriteScale = 1.f;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UMFAreaEffectData> ImpactArea;      // 落点回血场
};
```

### 3.3 `FMFItemDef`：`UseAreaData` → `ThrowableData`

```cpp
TSubclassOf<UGameplayEffect> UseEffect;       // 直接对宠（非投掷）——保留
TObjectPtr<UMFThrowableData>  ThrowableData;   // 配了它 = 投掷模式，优先
```

## 4. 玩家投掷入口（沿用已做的瞄准态）

```
双击消耗品 → 有 ThrowableData → 进瞄准态
左键点地面：
  · GetHitResultUnderCursor 取落点；距离 > MaxRange → 沿方向夹到最远处
  · 反算弹道（已知 Origin/Target/Speed/ArcHeight）：
      T          = 水平距离 / Speed
      GravityZ   = 8*ArcHeight / T²
      Velocity.Z = GravityZ*T/2 + (Target.Z - Origin.Z)/T
      Velocity.XY= 水平方向 * Speed
  · 扣 1 个消耗品
  · Launch({ Origin, Velocity, GravityZ, MaxRange, Sprite=FlightSprite, VisualScale,
             OnResolved = 落地时 SpawnAreaEffect(玩家, ImpactArea, 落点) })
  · 退出瞄准态
右键 → 取消，不消耗
```

回血场 `AllyOnly` 按玩家阵营（Team.Player）→ 影响范围内召唤宠。

## 5. 实现顺序（先重构渲染并回归，再叠抛物线投掷）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | Renderer 重构为 `PaperSpriteComponent` 池（删 ISM）；`LaunchParams`/`Instance`/`ProjectileMesh` 全换 `UPaperSprite` | **回归**：现有投掷/落石/弹幕改用 sprite 后照常飞、照常命中 |
| 2 | 弹道矢量化（`Velocity+GravityZ`，tick 积分）；三类攻击传 `Dir*Speed, Gravity=0` | **回归**：直线行为与改前一致 |
| 3 | 加 `HitGround` reason（下坠 sweep 命中地面上报） | Launch 抛物线 sprite（Gravity>0）→ 落地触发回调 |
| 4 | `UMFThrowableData` + `FMFItemDef` 换 `ThrowableData`；投掷入口反算弹道 + Launch（落地 SpawnAreaEffect）；接 `TickThrowAiming` | 双击消耗品→点地→sprite 抛物线飞→落点回血场 |
| 5 | （编辑器）现有 3 类攻击 DataAsset 重配 `ProjectileSprite`；`DA_HealField` + `DA_HealSnackThrow` + DT_Item 治疗零食指向它 | 见验收 |

**验收**：现有 AI 远程攻击（投掷/落石/弹幕）换 sprite 后行为不变；双击「治疗零食」→ 瞄准 → 点地 → sprite 抛物线飞 → 落点回血场 → 召唤宠回血 → 扣 1；超最远处落最远点；右键取消不消耗。

## 6. 性能：唯一要留意的点

纯 sprite = **没有硬件 instancing**（Paper2D 无 instanced 组件，这是删 ISM 的代价）。缓解与边界：

- **共享 subsystem tick**：所有活跃投射物由子系统一次性更新，无 per-actor tick 开销——已比"一投射物一 Actor"省很多。
- **组件池复用**：不反复 create/destroy，同屏 N 发只用 N 个池组件。
- **量级判断**：同屏几十~一两百发（含弹幕）现代硬件无压力。真正的风险只在**弹幕爆量到数百上千发**。
- **未来出路（仍不回 ISM/mesh）**：若哪天弹幕量爆炸，可在 sprite 层内部引入 instancing（sprite-atlas + billboard 材质批渲），对上层依旧是「配 sprite」，不违背"全 sprite"方向。**当前不预先做**。

## 7. 优缺点 / 风险

**优点**
- 渲染只有一条 sprite 路，无 mesh/sprite 分支——比双后端方案更简单。
- 全项目投射物统一 sprite，契合 2D 美术；投掷/落石/弹幕/玩家投掷共用一套 Launch/tick/结算。
- 抛物线 + sprite 成为配置项，未来任意投射物直接配。

**缺点 / 代价**
- 无 instancing（见 §6），弹幕爆量时才需再优化。
- 要迁移现有 3 类远程攻击资产 mesh→sprite（编辑器重配）。
- 重构 Renderer（删 ISM）+ 弹道矢量化，改动面覆盖所有投射物。

**风险点（按严重度）**
1. **回归现有 3 类远程攻击**（最高）：删 ISM + 换 sprite + 弹道矢量化，都在公共路径上，改错会静默弄坏投掷/落石/弹幕。→ 实现顺序把「重构 + 回归」压在步 1–2，先绿再叠新功能。
2. **抛物线落点精度**：反算初速度 + 地面高低不平时落地判定。→ 用下坠 sweep 撞地，点击点只定水平方向 + 最远夹取。
3. **sprite 池 / billboard bug**：slot 复用或朝向错乱。→ 掉落物已验证同款 billboard，风险可控。

## 8. 决策点（默认已选，可否决）

| 点 | 默认 | 备选 |
|---|---|---|
| 渲染 | **纯 `PaperSpriteComponent` 池，删 ISM** | （已否决 ISM 与双后端） |
| 落地判定 | **下坠 sweep 命中地面** | 预算飞行时间 T 定时落地 |
| 超最远距离 | **夹到最远处仍投出** | 拒绝投掷 + 提示 |
| 弹道配置 | **速度 + 弧高两旋钮**（入口反算重力） | 直接暴露 GravityZ + 初速度 |
| 飞行途中碰撞 | **仅落地生效** | 途中命中敌人即触发 |
