# 投掷消耗品 / 纯 Flipbook 投射物子系统 设计方案

> 2026-07-10 立项 ｜ 修订 1：统一进 `UMFProjectileSubsystem` ｜ 修订 2：全 2D，废弃 ISM ｜ **修订 3（2026-07-13）：载体对齐基底——全 `PaperFlipbook`，不是 PaperSprite**
> 需求：给玩家投掷物技能——点物品 → 点目标位置 → **抛物线**扔出**可配 2D 外观**的投掷物，落点生成回血场；有飞行速度和最远距离配置。
> 载体基底（2026-07-13 用户定调）：**所有场景 actor 表现统一 `PaperFlipbook`（单帧=静态图）**，见 `SceneObject_UnifiedSprite_Architecture.md` §3.0。修订 2 版本误用 `UPaperSprite`/`PaperSpriteComponent`，本修订全面替换为 flipbook，S4 照本版施工。

---

## 1. 定位：一条 flipbook 渲染路，贯穿所有投射物

全项目对 mesh 的依赖只有一处：`UMFRangedAttackDataBase::ProjectileMesh`（被投掷 `GA_ThrowProjectile` / 落石 `FallingBoulder` / 弹幕 `BulletCurtain` 三类共用）→ `LaunchParams.Mesh` → `Instance.Mesh` → `AMFProjectileRenderer` 的 ISM。

「全 flipbook」= 把这条链的 `UStaticMesh` 全换成 `UPaperFlipbook`，Renderer 从「ISM 批渲」重构成「`PaperFlipbookComponent` 池」。**不保留 ISM、不引入 PaperSprite**——单帧 flipbook 即静态图，多帧 = 飞行动画（旋转/拖尾帧）白送；与角色/掉落物/采集物同一条渲染路。

## 2. 子系统三处改动

### 2.1 渲染：Renderer 重构为 `PaperFlipbookComponent` 池（删 ISM）

`AMFProjectileRenderer` 内部整个换掉：

```cpp
// 删除：ISMMap（TMap<UStaticMesh*, ISM>）、GetOrCreateISM、按 mesh 分组
// 改为：一个可复用的 PaperFlipbookComponent 池
UPROPERTY() TArray<TObjectPtr<UPaperFlipbookComponent>> Pool;   // 挂在本 Actor 上
TArray<int32> FreeSlots;                                          // 空闲组件索引

int32 AcquireSlot(UPaperFlipbook* Flipbook, const FTransform& Xf); // 取空闲组件→SetFlipbook→显示→返回 index
void  UpdateSlot(int32 Slot, const FTransform& Xf);                // 每 tick 更新 transform（含 billboard）
void  ReleaseSlot(int32 Slot);                                     // 隐藏 + 归还
```

- slot 复用机制与原 ISM 版同构（空闲池、稳定索引），载体从「ISM instance」换成「池里的 PaperFlipbookComponent」。
- **billboard**：子系统每帧更新 slot transform 时让 flipbook 面向相机（与 `UMFSpriteVisualComponent::TickBillboard` 同算法，池内联轻量版——海量下不逐一挂组件）。
- 组件按需增长、只增不删（同 ISM slot 池策略），避免运行时反复 create/destroy。
- **准入检查**：本步及后续新增代码不得引入 `UPaperSprite`/`UStaticMesh` 场景外观（基底规则，见架构文档 §3.0）。

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

## 3. 数据配置（字段类型 = 载体源头，一律 `UPaperFlipbook`）

### 3.1 现有攻击基类：mesh → flipbook

```cpp
// UMFRangedAttackDataBase：
- TObjectPtr<UStaticMesh>   ProjectileMesh;       // 删
+ TObjectPtr<UPaperFlipbook> ProjectileFlipbook;   // 换（单帧=静态弹体，多帧=飞行动画）
```

→ 现有投掷/落石/弹幕的 DataAsset **重配 flipbook 资产**（编辑器活，见 §5 步 5；每个原 mesh 需一个对应 flipbook，静态的做单帧）。

### 3.2 新增 `UMFThrowableData`（玩家投掷参数）

```cpp
UCLASS(BlueprintType)
class UMFThrowableData : public UDataAsset
{
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UPaperFlipbook> FlightFlipbook;    // 飞行外观（单帧或动画）
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float Speed = 900.f;     // 水平速度 cm/s
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float MaxRange = 1500.f; // 最远落点 cm
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=0))   float ArcHeight = 250.f; // 抛物线顶高 cm
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=0.1)) float VisualScale = 1.f;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UMFAreaEffectData> ImpactArea;      // 落点回血场
};
```

### 3.3 `FMFItemDef`：消耗品字段 + 场景外观字段一并对齐（并入 S5 一次做）

```cpp
TSubclassOf<UGameplayEffect>  UseEffect;       // 直接对宠（非投掷）——保留
TObjectPtr<UMFThrowableData>  ThrowableData;    // 配了它 = 投掷模式，优先；取代临时的 UseAreaData

// 场景外观字段同步迁基底（S5 与上行同一次改，DT_Item 只重配一遍）：
- TObjectPtr<UPaperSprite>   WorldSprite;       // 删
+ TObjectPtr<UPaperFlipbook> WorldFlipbook;      // 换（掉落物场景外观；单帧=静态）
```

掉落物 `AMFLootPickup` 随之从自持 `PaperSpriteComponent` 切到基类 `FlipbookComponent`，删除 S3 里为 sprite 留的 `GetBillboardTarget/GetFlashTarget` 覆盖——掉落物自动获得闪光/碰撞拟合能力。

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
  · Launch({ Origin, Velocity, GravityZ, MaxRange, Flipbook=FlightFlipbook, VisualScale,
             OnResolved = 落地时 SpawnAreaEffect(玩家, ImpactArea, 落点) })
  · 退出瞄准态
右键 → 取消，不消耗
```

回血场 `AllyOnly` 按玩家阵营（Team.Player）→ 影响范围内召唤宠。

## 5. 实现顺序（对应架构文档 S4/S5；先重构渲染并回归，再叠抛物线投掷）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | Renderer 重构为 `PaperFlipbookComponent` 池（删 ISM）；`LaunchParams`/`Instance`/`ProjectileMesh` 全换 `UPaperFlipbook` | **回归**：现有投掷/落石/弹幕改用 flipbook 后照常飞、照常命中 |
| 2 | 弹道矢量化（`Velocity+GravityZ`，tick 积分）；三类攻击传 `Dir*Speed, Gravity=0` | **回归**：直线行为与改前一致 |
| 3 | 加 `HitGround` reason（下坠 sweep 命中地面上报） | Launch 抛物线 flipbook（Gravity>0）→ 落地触发回调 |
| 4 | `UMFThrowableData` + `FMFItemDef` 换 `ThrowableData` **并同步 WorldSprite→WorldFlipbook**；掉落物切基类 Flipbook；投掷入口反算弹道 + Launch（落地 SpawnAreaEffect）；接 `TickThrowAiming` | 双击消耗品→点地→flipbook 抛物线飞→落点回血场；掉落物照常 |
| 5 | （编辑器）三类攻击 DataAsset 重配 `ProjectileFlipbook`；DT_Item 重配 `WorldFlipbook`；`DA_HealField` + `DA_HealSnackThrow` + DT_Item 治疗零食指向它 | 见验收 |

**验收**：现有 AI 远程攻击（投掷/落石/弹幕）换 flipbook 后行为不变；双击「治疗零食」→ 瞄准 → 点地 → flipbook 外观抛物线飞 → 落点回血场 → 召唤宠回血 → 扣 1；超最远处落最远点；右键取消不消耗；掉落物换 `WorldFlipbook` 后散开/拾取/billboard 照常。

## 6. 性能：唯一要留意的点

纯 flipbook = **没有硬件 instancing**（Paper2D 无 instanced 组件，这是删 ISM 的代价）。缓解与边界：

- **共享 subsystem tick**：所有活跃投射物由子系统一次性更新，无 per-actor tick 开销。
- **组件池复用**：不反复 create/destroy，同屏 N 发只用 N 个池组件。
- **量级判断**：同屏几十~一两百发（含弹幕）现代硬件无压力。真正的风险只在**弹幕爆量到数百上千发**。
- **未来出路（仍不回 ISM/mesh）**：若哪天弹幕量爆炸，可在 2D 层内部引入 instancing（sprite-atlas + billboard 材质批渲），对上层依旧是「配 flipbook」，不违背基底。**当前不预先做**。

## 7. 优缺点 / 风险

**优点**
- 渲染只有一条 flipbook 路，与角色/掉落物/采集物同基底；投掷/落石/弹幕/玩家投掷共用一套 Launch/tick/结算。
- 抛物线 + flipbook 成为配置项；弹药飞行动画（多帧）零代码。

**缺点 / 代价**
- 无 instancing（见 §6），弹幕爆量时才需再优化。
- 现有 3 类远程攻击资产 mesh→flipbook、DT_Item WorldSprite→WorldFlipbook 需编辑器重配（静态图各包一个单帧 flipbook；量少，手工可行，多了再做批量工具）。
- 重构 Renderer（删 ISM）+ 弹道矢量化，改动面覆盖所有投射物。

**风险点（按严重度）**
1. **回归现有 3 类远程攻击**（最高）：删 ISM + 换 flipbook + 弹道矢量化，都在公共路径上。→ 实现顺序把「重构 + 回归」压在步 1–2，先绿再叠新功能。
2. **抛物线落点精度**：反算初速度 + 地面高低不平时落地判定。→ 用下坠 sweep 撞地，点击点只定水平方向 + 最远夹取。
3. **flipbook 池 / billboard bug**：slot 复用或朝向错乱。→ 与 `UMFSpriteVisualComponent` 同算法（S1–S3 已验证），风险可控。

## 8. 决策点（默认已选，可否决）

| 点 | 默认 | 备选 |
|---|---|---|
| 渲染 | **纯 `PaperFlipbookComponent` 池，删 ISM**（载体=基底 `UPaperFlipbook`） | （已否决 ISM 与 PaperSprite） |
| 落地判定 | **下坠 sweep 命中地面** | 预算飞行时间 T 定时落地 |
| 超最远距离 | **夹到最远处仍投出** | 拒绝投掷 + 提示 |
| 弹道配置 | **速度 + 弧高两旋钮**（入口反算重力） | 直接暴露 GravityZ + 初速度 |
| 飞行途中碰撞 | **仅落地生效** | 途中命中敌人即触发 |
