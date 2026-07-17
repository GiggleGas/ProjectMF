# 出生锚点 + 回家 + 绕家巡逻 —— 设计方案

> 2026-07-13 立项 ｜ **2026-07-13 定稿（v2）** ｜ 基线见 `TargetingSystem_Current.md`
> 需求（用户）：①追击时长要能配（追玩家太久）；②每个 AI 记住出生点、有游离半径；③离家太远先走回出生点再巡逻。
> **关键定调**：回家是**低优先级填充行为**（不是强制护送）——**战斗 > 回家 > 巡逻**，回家路上撞见敌人可直接进战；放弃追击**纯靠时间**（`LockDuration`），不做空间硬上限。→ 方案大幅简化。

---

## 1. 现状与问题

| 现象 | 根因 |
|---|---|
| 追玩家太久 | `LockDuration=5s` 默认偏久；且目标只要在 `SensingRadius=1000` 内就无限追 |
| 巡逻越走越远 | `FSTTask_FindRandomNavPoint` 绕 **pawn 当前位置**采样（`STTask_FindRandomNavPoint.cpp:46`）→ 漂移 |
| 不记得出生点 | AI 出生后无"家"数据，追出去回不来、也不知该守哪 |

## 2. 方案：战斗 > 回家 > 巡逻

一个"家" + 一个"游离半径"。三个状态按优先级：

| 状态 | 进入条件 | 行为 |
|---|---|---|
| **战斗**（最高） | 有目标（`MF.AI.Perception.HasTarget`） | 追击/攻击当前目标。**不受离家距离限制**——多远都打，包括回家路上 |
| **回家**（中） | 无目标 **且** 离家 > `WanderRadius` | 走回出生点。走到家（或被战斗抢占）为止 |
| **巡逻**（低） | 无目标 **且** 在家附近 | 绕出生点在 `WanderRadius` 内随机巡逻 |

**"回家路上能进战" = 天然实现**：回家状态运行中，威胁组件一旦锁定目标（`HasTarget` 变 true），优先级更高的战斗状态**自动抢占**。不需要任何额外逻辑，也不需要"强制脱战/清目标"。

**"追太久"怎么收 = 纯时间**：靠 `LockDuration`（目标脱离感知范围 N 秒后放弃），默认从 5s **下调到 2~3s**。目标死亡/消失后 AI 丢失目标 → 若离家远则回家。**不做"离家太远强制放弃目标"**（那会引入边界反复，已否决）。

> 巡逻采样点都在 `WanderRadius` 内，AI 巡逻不会自己走出圈 → **巡逻本身不触发回家**；只有战斗把 AI 拉出圈、之后丢了目标，才会回家。无边界抖动。

## 3. 数据配置（只要一个半径）

### 3.1 `UMFHomeAnchorComponent`（新，AI 模块）

```cpp
// 出生记锚点；提供回家/巡逻判定。AMFAICharacter 构造挂，所有 AI/宠共用。
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class UMFHomeAnchorComponent : public UActorComponent
{
    FVector HomeLocation;
    bool    bHomeSet = false;
    FMFHomeAnchorConfig Config;             // 来自 PetConfig->AnchorConfig

public:
    void    ApplyConfig(const FMFHomeAnchorConfig& In);
    void    SetHome(const FVector& Loc);            // 记锚点（出生时）
    FVector GetHomeLocation() const;
    float   GetDistanceFromHome() const;            // owner 当前离家距离
    bool    IsBeyondWander() const;                 // 离家 > WanderRadius（回家触发）
    bool    IsAtHome() const;                        // 离家 < HomeArrivalTolerance（到家）
    bool    IsEnabled() const { return Config.bEnableHomeAnchor && bHomeSet; }
};
```

### 3.2 `FMFHomeAnchorConfig`（进 `UMFPetConfig`，与 RadarConfig/ThreatConfig 并列）

```cpp
USTRUCT(BlueprintType)
struct FMFHomeAnchorConfig
{
    /** 启用锚点/回家（false = 旧行为：绕当前位置巡逻、不回家）。 */
    UPROPERTY(EditDefaultsOnly) bool bEnableHomeAnchor = true;

    /** 游离半径（cm）：巡逻在以锚点为心此半径内；离家超此值且无目标 → 回家。 */
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float WanderRadius = 800.f;

    /** 到家容差（cm）：离锚点小于此值算"已到家"，恢复巡逻。 */
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=10))  float HomeArrivalTolerance = 120.f;
};
```

### 3.3 追击时长（威胁配置，已有字段改默认）

`FMFThreatConfig.LockDuration` 已可配（PetConfig）→ **默认 5s 下调到 2~3s**。
语义：目标**脱离感知范围后**的宽限追击时长。目标一直在感知范围内仍无限追（用户接受，靠回家兜底：玩家跑出感知圈 2~3s 后 AI 放弃 → 若离家远则走回家）。

## 4. StateTree C++ 新增/改造（精简）

沿用现有惯例（`FStateTreeAIActionTaskBase` + InstanceData 绑变量 + `TStateTreeExternalDataHandle<AAIController>` 拿 pawn/组件）。

| 新增/改 | 类型 | 作用 |
|---|---|---|
| **`FSTCond_IsBeyondWander`** | 条件 | 读 HomeAnchor：无目标时离家 > WanderRadius → true。用于进"回家" |
| **`FSTTask_GetHomeLocation`** | 任务 | 输出锚点坐标到 ST 变量 `HomeLoc`（同 GetThreatTarget 输出模式），供 MoveTo |
| **`FSTTask_FindRandomNavPoint` 改造** | 任务 | InstanceData 加可选 `Origin`（默认 pawn 位置，**向后兼容**）；绑 `HomeLoc` → 绕家巡逻 |

> **威胁组件不用改**（战斗被回家打断靠 StateTree 优先级 + HasTarget，不需要主动清目标）。相比 v1 省掉了 `ForceDropCurrentTarget` / `ForceDropTarget` / `IsBeyondLeash` 挡战斗 / 滞后抑制。

## 5. StateTree 资产结构（你在编辑器配）

优先级（高→低）：**Combat > ReturnHome > Patrol**。

```
[Root · Selector]
│
├─ [Combat]                           ← 最高：有目标就打
│    Enter: HasTag(MF.AI.Perception.HasTarget)
│    Tasks: GetThreatTarget → MoveTo(Target) → CanAutoUseSkill → ActivateAttack
│    退出: 丢目标(GetThreatTarget Failed) → 交回 Selector（据下面条件转 ReturnHome / Patrol）
│
├─ [ReturnHome]                       ← 中：没目标 + 离家远
│    Enter: NOT HasTag(HasTarget)  AND  IsBeyondWander
│    Tasks: GetHomeLocation → HomeLoc；  MoveTo(HomeLoc, 接受半径=HomeArrivalTolerance)
│    退出: MoveTo Succeeded / IsAtHome → [Patrol]
│          期间若 HasTarget → 被 [Combat] 抢占（回家路上进战，天然）
│
└─ [Patrol]                           ← 低：没目标 + 在家附近（默认）
     Tasks: FindRandomNavPoint(Origin=HomeLoc, Max=WanderRadius) → MoveTo → Wait(停顿) → 循环
     退出: HasTarget → [Combat]
```

**和 v1 的差别**：Combat 放最上、进入条件**不含** leash；ReturnHome 降到中间、可被 Combat 抢占、进入时**不清目标**。

## 6. 数据流

```
出生: SpawnSinglePet → HomeAnchor.SetHome(出生位置)
  ↓
Patrol: 绕 HomeLoc 巡逻（不漂移；巡逻不会自己走出圈）
  ↓ 发现敌人(HasTarget)
Combat: 追击/攻击（不管离家多远）
  ↓ 目标脱离感知 LockDuration(2~3s) 后放弃 / 目标死亡 → 丢目标
  ├─ 若离家 > WanderRadius → [ReturnHome]: 走回出生点
  │      ↓ 回家路上又发现敌人 → 被 [Combat] 抢占，继续打
  │      ↓ 到家(IsAtHome)
  └─ 若在家附近 → 直接 [Patrol]
  ↓
Patrol: 绕出生点巡逻
```

## 7. 决策点（已定）

| 点 | 定稿 |
|---|---|
| 状态优先级 | ✅ **战斗 > 回家 > 巡逻**（回家可被战斗抢占） |
| 回家是否脱战 | ✅ **不脱战**（回家路上撞敌人直接进战，天然） |
| 放弃追击 | ✅ **纯时间**（`LockDuration` 下调 2~3s），不做空间硬上限 |
| 锚点数据放哪 | **新组件 `UMFHomeAnchorComponent`**（内聚，PetConfig 配置模式一致） |
| 半径 | ✅ **单一 `WanderRadius`**（巡逻范围兼回家触发）+ 到家容差 |
| 手摆(非 SpawnManager)AI | HomeAnchor BeginPlay 用当前位置兜底记家 |

## 8. 实现顺序（全 C++ + 编辑器配 StateTree）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `UMFHomeAnchorComponent` + `FMFHomeAnchorConfig`（进 PetConfig）；`AMFAICharacter` 构造挂；`SpawnSinglePet` 后 `SetHome`（+ BeginPlay 兜底） | 编译过；log 出生锚点 |
| 2 | `FSTCond_IsBeyondWander` + `FSTTask_GetHomeLocation` | 编译过；ST 编辑器可见节点 |
| 3 | `FSTTask_FindRandomNavPoint` 加 `Origin` 输入（默认 pawn 位置，向后兼容） | 旧 ST 行为不变；绑 HomeLoc 后绕家 |
| 4 | `FMFThreatConfig.LockDuration` 默认改 2~3s | 追击时长缩短 |
| 5 | （编辑器）Pet StateTree 加 ReturnHome 状态；Patrol 的 FindRandomNavPoint 绑 HomeLoc；PetConfig 配 AnchorConfig | 见验收 |

**验收**：AI 巡逻被绕在出生点 `WanderRadius` 内不漂移；追敌人可追远，目标脱离感知 2~3s 后放弃 → 走回出生点 → 到家恢复绕家巡逻；回家路上撞见敌人直接进战、打完继续回家。用 `MF.Debug.ThreatSystem` + 建议给 HomeAnchor 加 cvar 画锚点/游离圈辅助调参。

## 9. Debug 建议

- cvar `MF.Debug.HomeAnchor 1`：画锚点(十字) + `WanderRadius`(绿圈) + owner→home 连线 + 当前状态文字。
- log（`LogMFAI`）：SetHome、进入 ReturnHome（离家距离）、到家。
