# 出生锚点 + 拴绳(Leash)+ 回家 —— 设计方案

> 2026-07-13 ｜ 状态：方案待评审，**未改代码** ｜ 基线见 `TargetingSystem_Current.md`
> 需求（用户）：①追击时长要能配（现在追玩家太久）；②每个 AI 记住出生点，有"游离半径"；③离家太远先走回出生点，再随机巡逻。经典 **leash / 拴绳** 机制。

---

## 1. 现状与问题

| 现象 | 现状根因 |
|---|---|
| 追玩家太久 | 威胁 `LockDuration=5s`（`FMFThreatConfig`）：目标**离开感知范围后**仍追 5s；且目标只要在 `SensingRadius=1000` 内就**无限追**，无空间上限 |
| 巡逻越走越远 | `FSTTask_FindRandomNavPoint` 绕 **pawn 当前位置**采样（`STTask_FindRandomNavPoint.cpp:46`）→ 随机游走漂移，无锚点约束 |
| 不记得出生点 | AI 出生后无任何"家"数据，追出去就回不来 |

**核心**：缺一个**空间锚点 + 拴绳半径**。追击该由「离家多远」约束（空间），而不只靠「追多久」（时间）。

---

## 2. 方案总览

三件事：
1. **存出生锚点** + 三个半径配置（游离/拴绳/到家容差）。
2. **追击时长可配** = 下调 `LockDuration` 默认 + 新增 leash 空间硬约束。
3. **回家 StateTree**：离家 > 拴绳半径 → 抢占进「回家」状态（清目标 + 走回锚点）→ 到家 → 回「绕家巡逻」。

### 2.1 锚点数据放哪（决策点 A）

| 选项 | 说明 | 取舍 |
|---|---|---|
| **A. 新组件 `UMFHomeAnchorComponent`（推荐）** | 与 `RadarSensing`/`Threat` 并列，`PetConfig.AnchorConfig` 与 `RadarConfig`/`ThreatConfig` 并列，配置模式一致 | 内聚、可扩展（领地/多 AI 共享锚点），代价：加第三个组件 |
| B. 并入 `UMFThreatComponent` | 不加组件 | 威胁组件职责从"纯索敌"扩到"领地"，巡逻任务读威胁组件拿家点略跨职责 |

推荐 **A**：领地/拴绳是独立概念，威胁组件保持纯索敌。下文按 A 写。

---

## 3. 数据配置

### 3.1 `UMFHomeAnchorComponent`（新，AI 模块）

```cpp
// 出生时记录锚点；提供 leash 查询。所有 AI/宠共用（AMFAICharacter 构造时挂）。
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class UMFHomeAnchorComponent : public UActorComponent
{
    // 出生锚点（BeginPlay 或 SpawnSinglePet 后由 SetHome 写入）。
    FVector HomeLocation;
    bool    bHomeSet = false;

    FMFHomeAnchorConfig Config;   // 来自 PetConfig->AnchorConfig

public:
    void  ApplyConfig(const FMFHomeAnchorConfig& In);
    void  SetHome(const FVector& Loc);                  // 记锚点
    FVector GetHomeLocation() const;
    float GetDistanceFromHome(const FVector& From) const;
    bool  IsBeyondLeash() const;                        // owner 离家 > LeashRadius
    bool  IsAtHome() const;                             // owner 离家 < HomeArrivalTolerance
    bool  IsEnabled() const { return Config.bEnableHomeAnchor && bHomeSet; }
};
```

### 3.2 `FMFHomeAnchorConfig`（进 `UMFPetConfig`）

```cpp
USTRUCT(BlueprintType)
struct FMFHomeAnchorConfig
{
    /** 是否启用锚点/拴绳（false = 旧行为，绕当前位置巡逻、无 leash）。 */
    UPROPERTY(EditDefaultsOnly) bool bEnableHomeAnchor = true;

    /** 巡逻游离半径（cm）：巡逻选点在以锚点为心的此半径内。 */
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float WanderRadius = 800.f;

    /** 拴绳半径（cm）：追击离家超此值 → 放弃目标回家。通常 > WanderRadius。 */
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=100)) float LeashRadius = 1500.f;

    /** 到家判定容差（cm）：离锚点小于此值算"已到家"。 */
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=10))  float HomeArrivalTolerance = 120.f;
};
```

**半径关系（建议）**：`HomeArrivalTolerance < WanderRadius < LeashRadius`。
活动范围直觉：巡逻不超离家 `WanderRadius`；发现敌人可追到离家 `LeashRadius`，超了就回家 → AI 整体被"拴"在锚点 `LeashRadius` 内。

### 3.3 追击时长（威胁配置微调）

`FMFThreatConfig.LockDuration` 已可配（PetConfig），但默认 5s 偏久 → **建议默认下调到 2~3s**。
语义要分清：
- `LockDuration` = 目标**脱离感知范围后**的宽限追击时长（时间约束）。
- `LeashRadius` = 离家硬上限（空间约束）。
- 两者**互补**：时间管"目标躲进掩体/暂时消失还追不追"，空间管"追出领地就放弃"。「追玩家太久」两个一起调最稳。

---

## 4. StateTree C++ 新增/改造

沿用现有惯例（`FStateTreeAIActionTaskBase` + InstanceData 绑变量 + `TStateTreeExternalDataHandle<AAIController>` 拿 pawn/组件，同 `STTask_GetThreatTarget`）。

| 新增/改 | 类型 | 作用 |
|---|---|---|
| **`FSTCond_IsBeyondLeash`** | 条件 | 读 HomeAnchor：owner 离家 > LeashRadius → true。用于进 ReturnHome / 挡 Combat |
| **`FSTTask_GetHomeLocation`** | 任务 | 输出锚点坐标到 ST 变量 `HomeLoc`（同 GetThreatTarget 输出模式），供 MoveTo |
| **`FSTTask_FindRandomNavPoint` 改造** | 任务 | InstanceData 加可选 `Origin`（默认 pawn 位置，向后兼容）；绑 `HomeLoc` → 绕家巡逻 |
| **`FSTTask_ForceDropTarget`** | 任务 | 调威胁组件新接口清目标（ReturnHome 进入时脱战） |
| **`UMFThreatComponent::ForceDropCurrentTarget()`** | C++ 接口 | 清 `CurrentTarget` + 从威胁列表移除 + 撤 `HasTarget` tag（当前无公开清目标接口，需补） |

> `FindRandomNavPoint` 改造最小：只在 InstanceData 加一个 `FVector Origin` + `bool bUseOrigin`（或 Origin 为零则回退 pawn 位置），第 46 行 `Origin` 取值改为"绑定值优先，否则 pawn 位置"。旧 StateTree 不绑 = 行为不变。

---

## 5. 返回锚点的 StateTree 资产结构（你在编辑器配）

状态优先级（高→低）：**ReturnHome > Combat > Patrol/Idle**。StateTree 每帧重评估 Enter 条件，`IsBeyondLeash` 变 true 时**抢占**（即使正在 Combat）。

```
[Root · Selector]
│
├─ [ReturnHome]                       ← 优先级最高
│    Enter 条件: IsBeyondLeash == true
│    进入即脱战:  Task_ForceDropTarget      （清威胁目标，回家路上不追）
│    Tasks:      Task_GetHomeLocation → HomeLoc
│                Move To (HomeLoc)
│    Transitions:
│      Move To Succeeded  → [Patrol]
│      IsAtHome == true   → [Patrol]        （双保险）
│
├─ [Combat]
│    Enter 条件: HasTag(MF.AI.Perception.HasTarget)  AND  NOT IsBeyondLeash
│    Tasks:      Task_GetThreatTarget → Move To(Target) → Cond_CanAutoUseSkill → Task_ActivateAttack
│    Transitions:
│      GetThreatTarget Failed（丢目标） → [Patrol]
│      IsBeyondLeash == true            → [ReturnHome]   （追出界，抢占）
│
└─ [Patrol / Idle]                    ← 默认
     Tasks:  Task_FindRandomNavPoint(Origin = HomeLoc, Min/Max = 0 / WanderRadius)
             Move To (ResultLocation)
             Wait (停顿 Ns)
             → 循环
     Transitions:
       HasTag(MF.AI.Perception.HasTarget) → [Combat]
```

**关键点**：Combat 的 Enter 多了 `AND NOT IsBeyondLeash` —— 离家太远时**不进也不保持** Combat，ReturnHome 优先。这一条是 leash 生效的核心。

---

## 6. 完整数据流

```
出生: SpawnSinglePet → HomeAnchor.SetHome(出生位置)
  ↓
Patrol: FindRandomNavPoint(绕 HomeLoc, WanderRadius) → 巡逻不漂移，被拴在锚点附近
  ↓ 发现敌人(HasTarget tag)
Combat: 追击目标（威胁组件锁定）
  ↓ 追击中离家距离渐增，超过 LeashRadius
IsBeyondLeash=true → 抢占 [ReturnHome]:
     ForceDropTarget(脱战) → GetHomeLocation → MoveTo(锚点)
  ↓ 到家 (IsAtHome / MoveTo 成功)
回 [Patrol]: 绕锚点巡逻
  （追击时间也受 LockDuration 约束：目标脱离感知 N 秒也放弃）
```

---

## 7. 决策点（需拍板）

| 点 | 默认（推荐） | 备选 |
|---|---|---|
| A. 锚点数据放哪 | **新组件 `UMFHomeAnchorComponent`** | 并入威胁组件 |
| B. 回家是否脱战 | **进 ReturnHome 即清目标**（回家=脱战，符合"回家后巡逻"） | 保留目标（到家若敌人还在门口，继续打） |
| C. LockDuration 默认 | **下调到 2~3s** | 保持 5s，纯靠 leash 空间约束 |
| D. 锚点记录时机 | **SpawnSinglePet 生成后 SetHome**（也覆盖手摆 AI 的 BeginPlay 兜底） | 仅 BeginPlay 自记 |
| E. 巡逻中心 | **锚点（WanderRadius 内）** | 保留绕当前位置（不解决漂移，不推荐） |
| F. 手摆(非 SpawnManager)AI | HomeAnchor BeginPlay 用当前位置兜底自记 | 需手动指定锚点 Actor |

---

## 8. 实现顺序（全 C++ + 编辑器配 StateTree）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `UMFHomeAnchorComponent` + `FMFHomeAnchorConfig`（进 PetConfig）；`AMFAICharacter` 构造挂组件；`SpawnSinglePet` 后 `SetHome` | 编译过；log 出生锚点 |
| 2 | `UMFThreatComponent::ForceDropCurrentTarget()` 公开接口 | 调用后脱战、tag 撤销 |
| 3 | `FSTCond_IsBeyondLeash` + `FSTTask_GetHomeLocation` + `FSTTask_ForceDropTarget` | 编译过；节点在 ST 编辑器可见 |
| 4 | `FSTTask_FindRandomNavPoint` 加 `Origin` 输入（默认 pawn 位置，向后兼容） | 旧 ST 行为不变；绑 HomeLoc 后绕家 |
| 5 | （编辑器）改 Pet StateTree 资产：加 ReturnHome 状态 + Combat 加 NOT IsBeyondLeash + Patrol 绑 HomeLoc；PetConfig 配 AnchorConfig | 见验收 |

**验收**：AI 巡逻被拴在锚点 `WanderRadius` 内不漂移；追敌人追出 `LeashRadius` → 脱战走回出生点 → 到家恢复绕家巡逻；`LockDuration` 调小后目标脱离感知的追击时间明显缩短。用 `MF.Debug.ThreatSystem` + 建议给 HomeAnchor 加 cvar 画锚点/两个半径圈辅助调参。

---

## 9. Debug 建议（配合验证）

- 新增 cvar `MF.Debug.HomeAnchor 1`：画锚点(十字)+ WanderRadius(绿圈)+ LeashRadius(红圈)+ owner→home 连线，直观调半径。
- log（`LogMFAI`）：SetHome、进入 ReturnHome（离家距离）、到家、ForceDropTarget。
