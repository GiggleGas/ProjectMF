# AI 行为决策系统（技能自描述 + 通用战斗树）设计方案

> 2026-07-17 立项。目标：让 StateTree 收敛为少数几棵"原型树"，新增 AI/新技能组合不再需要手配树。
> 核心思路：**技能相关知识从树下沉到技能数据**，树退化为通用决策循环。

---

## 1. 问题与目标

### 现状问题

当前每个技能在 StateTree 里占一个专属 state：

- 节点上手填 `AbilityTag` + `ActiveStateTag`（`STTask_ActivateAttack`），二者必须与技能蓝图配对；
- 进入条件手挂 `STCond_CanAutoUseSkill`（再填一遍同一个 tag）；
- 走位逻辑（贴脸/拉开距离）按技能写在树结构里。

结果：**树的结构 = 技能配置表**。宠物技能组合一变就要重配树，AI 种类增多后编辑器工作量线性增长。

### 目标

1. 一棵通用战斗子树服务所有技能组合（近战怪/远程怪/混合宠共用）；
2. 新增一种 AI = 建一个 `UMFPetConfig` + 从原型树里挑一棵，**零树编辑**；
3. 新增一个技能 = 建 GA + AttackData（填好 AI 决策字段），自动被决策系统纳入；
4. 召唤宠 Auto/Manual 门禁、指令系统事件管道行为不变。

### 非目标（v2 及以后，见 §7）

- 风筝走位（保持最小距离拉开）；
- 视线（LOS）检查；
- 冲锋/跳跃类移动技能作为"接近手段"参与决策；
- 效用评分（utility scoring）——v1 只做优先级 + 冷却过滤。

---

## 2. 架构总览

```
┌─ 数据层 ─────────────────────────────────────────────┐
│ UMFAttackDataBase                                    │
│   + FMFSkillAIConfig AIConfig   ← 新增（本方案核心）  │
│     Priority / MinRange / PreferredRange / MaxRange  │
│   （射程默认值复用现有字段：近战 Range / 远程 MaxRange）│
└──────────────────────────────────────────────────────┘
                 ↑ 读
┌─ 决策层 ─────────────────────────────────────────────┐
│ FMFSkillSelector（静态工具，无状态）                   │
│   SelectSkill(ASC) → FMFSkillCandidate               │
│   过滤：已授予 + 冷却就绪 + Auto门禁(召唤宠)           │
│   排序：Priority 降序，平局取先授予                    │
└──────────────────────────────────────────────────────┘
                 ↑ 调用
┌─ 编排层（通用 ST 节点 ×3）───────────────────────────┐
│ STTask_SelectSkill      → 输出选中技能+射程参数        │
│ STTask_MoveToSkillRange → 按选中技能射程走位           │
│ STTask_ActivateChosenSkill → 激活 + OnAbilityEnded 等结束│
└──────────────────────────────────────────────────────┘
                 ↑ 承载
┌─ 资产层（原型树，手工维护仅此几棵）───────────────────┐
│ ST_Enemy_Generic   游荡 → 索敌 → 通用战斗循环          │
│ ST_Wildlife        游荡 → 受击逃跑                     │
│ ST_SummonedPet     现有指令树，战斗子状态换新三节点     │
│ ST_Boss_Xxx        特例怪手工定制（允许存在）           │
└──────────────────────────────────────────────────────┘
```

技能过程差异（冲锋位移/弹幕站桩/落石选点）不进决策层——那是 GA 激活后自己的事，
现有 `GA_AIAttackBase` / `GA_AIRangedAttackBase` 管线已经覆盖，本方案不动。

---

## 3. 组件规格

### 3.1 FMFSkillAIConfig（新增 USTRUCT，进 MFAttackTypes.h 或独立头）

```cpp
/** 攻击技能的 AI 决策参数。挂在 UMFAttackDataBase 上，供 FMFSkillSelector 读取。 */
USTRUCT(BlueprintType)
struct FMFSkillAIConfig
{
    GENERATED_BODY()

    /** 选择优先级，高者先选（同优先级取先授予的）。0 = 默认。 */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    int32 Priority = 0;

    /** 释放最小距离（cm）。目标近于此距离时本技能不参与走位目标计算（v1 不主动拉开）。 */
    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0"))
    float MinRange = 0.f;

    /**
     * 理想站位距离（cm）。<=0 表示用技能自身检测射程
     * （近战 = AttackData.Range，远程 = MaxRange × 0.8 经验系数）。
     * MoveToSkillRange 以此为 AcceptanceRadius。
     */
    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0"))
    float PreferredRange = 0.f;
};
```

`UMFAttackDataBase` 增加：

```cpp
/** AI 决策参数（优先级/站位距离）。 */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|AI")
FMFSkillAIConfig AIConfig;

/** 本技能的最大有效释放距离。子类覆写：近战返回 Range，远程返回 MaxRange。 */
virtual float GetAIMaxRange() const { return 150.f; }
```

- `UMFAttackAbilityData::GetAIMaxRange()` → `return Range;`
- `UMFRangedAttackDataBase::GetAIMaxRange()` → `return MaxRange;`
- 落石（MaxRange 语义是"下落高度"）在 `UMFFallingBoulderData` 单独覆写，返回合理投射距离。

**现有数据资产不需要立即改**：所有字段有合理默认值，只有想调优先级/站位时才填。

### 3.2 FMFSkillSelector（新增静态工具，AI/Public/MFSkillSelector.h）

```cpp
/** 一次技能决策的结果。 */
struct FMFSkillCandidate
{
    FGameplayAbilitySpecHandle SpecHandle;
    float MinRange      = 0.f;
    float MaxRange      = 150.f;
    float PreferredRange = 150.f;   // 已解析（含 <=0 时的默认推导）
    bool  IsValid() const { return SpecHandle.IsValid(); }
};

namespace MFSkillSelector
{
    /**
     * 从 ASC 已授予技能中选出当前最优的可自动释放攻击技能。
     * 过滤：AbilityTags 含 MF.Ability.* + 是 UMFPetGameplayAbility 且有 AttackDataBase
     *       + 不在冷却 + Auto门禁（非召唤恒过；召唤宠须 spec 带 MF.SkillMode.Auto）。
     * 排序：AIConfig.Priority 降序；平局取授予顺序靠前者。
     * 无可用技能返回 Invalid（战斗状态据此退出/等待）。
     */
    FMFSkillCandidate SelectSkill(UAbilitySystemComponent* ASC);
}
```

- 冷却判定和 Auto 门禁逻辑**从 `STCond_CanAutoUseSkill::TestCondition` 原样搬入**（那是唯一实现处，搬完该条件节点退役）。
- 无状态、可被任何"脑子"调用——将来上 Mass 或代码 FSM 也复用这一层。

### 3.3 三个通用 StateTree 节点（AI/Public|Private/）

**STTask_SelectSkill**

| | |
|---|---|
| 输入 | 无（ASC 从 AIController Pawn 取，同现有节点） |
| 输出（绑定到树变量） | `FGameplayAbilitySpecHandle SelectedSkill`、`float PreferredRange`、`float MaxRange` |
| EnterState | 调 `SelectSkill()`；有结果 → 写输出、Succeeded；无 → Failed |

短任务，Succeeded 后由树排到下一节点。战斗循环靠树的状态重进天然实现"每轮重选"。

**STTask_MoveToSkillRange**

| | |
|---|---|
| 输入 | `AActor* TargetActor`（绑现有 GetThreatTarget 输出）、`float PreferredRange` |
| EnterState | 已在射程内 → 直接 Succeeded；否则 `MoveToActor(Target, AcceptanceRadius=PreferredRange)` |
| Tick | 每帧刷距离，进入射程 → StopMovement + Succeeded；目标失效 → Failed |
| ExitState | StopMovement |

替代原先按技能写死的走位分支。近战（PreferredRange≈150）和远程（≈1200）走同一节点。

**STTask_ActivateChosenSkill**

| | |
|---|---|
| 输入 | `FGameplayAbilitySpecHandle SelectedSkill` |
| EnterState | `TryActivateAbility(Handle)`；失败 → Failed。成功 → 绑 `ASC->OnAbilityEnded`，Running |
| 结束判定 | OnAbilityEnded 委托匹配 Handle → Succeeded（**取代 ActiveStateTag 轮询，无需再配对状态标签**） |
| ExitState | spec 仍 Active（被打断）→ `CancelAbilityHandle`；解绑委托 |

`GA_AIAttackBase` 里的 loose `State_Attacking` 标签保留（动画/表现在用），只是不再驱动脑子。

**风险备忘**：若 StateTree 编辑器对 `FGameplayAbilitySpecHandle` 的属性绑定不顺
（理论上普通 USTRUCT 可绑），降级方案 = SelectSkill 输出改为 `FGameplayTag`（技能 AbilityTag），
Activate 节点按 tag 再查一次 spec。功能等价，只多一次遍历。

### 3.4 原型树（编辑器侧）

| 树 | 结构 | 服务对象 |
|---|---|---|
| ST_Enemy_Generic | Root → Wander ↔ Combat[GetThreatTarget → SelectSkill → MoveToSkillRange → ActivateChosenSkill]（HasTarget 标签驱动切换） | 所有常规敌怪/野宠 |
| ST_Wildlife | Wander ↔ Flee（受击/发现威胁即逃） | 纯采集型动物 |
| ST_SummonedPet | 现有指令树保持，仅"自动战斗"子状态替换为新三节点串 | 召唤宠 |
| ST_Boss_* | 手工定制，可自由混用新旧节点 | 特殊怪/Boss |

### 3.5 退役清单（迁移完成后）

- `STCond_CanAutoUseSkill`（逻辑并入 Selector）
- `STTask_ActivateAttack`（被 SelectSkill + ActivateChosenSkill 取代）

迁移期两套共存，所有树切换验证后删。

---

## 4. 排期（合计 2~2.5 天）

| 阶段 | 内容 | 产出/验收 | 工时 |
|---|---|---|---|
| **P1 数据层** | `FMFSkillAIConfig` + `UMFAttackDataBase.AIConfig` + `GetAIMaxRange()` 三处覆写 | 编译过；现有资产不改仍正常 | 0.5d |
| **P2 决策层** | `MFSkillSelector::SelectSkill`（含从 STCond 搬冷却/Auto门禁逻辑） | GM 命令打印选择结果验证（临时 `MF.Debug.SkillSelector`） | 0.5d |
| **P3 编排层** | 三个 ST 节点 | 编译过 + 节点在 ST 编辑器可见可绑 | 0.5d |
| **P4 树收敛** | 编辑器建 ST_Enemy_Generic；一只近战怪 + 一只远程怪切过去；召唤宠树替换战斗子状态 | 见 §5 验证清单 | 0.5d |
| **P5 收尾** | 退役旧节点、其余 AI 逐个切树、文档/记忆更新 | 全部 AI 跑新树 | 0.25~0.5d |

P1–P3 纯 C++（我做）；P4 需要编辑器操作（树资产 + PetConfig 换引用），我出逐步操作清单。
时间点建议：排在 W2 打造合成之后、W3 大地图整合铺怪之前——铺怪正好直接吃新系统红利。

---

## 5. 验证清单（P4 完成标准）

- [ ] 近战怪：发现玩家 → 贴近到 Range → 循环平A；玩家跑远 → 追击 → 脱战回游荡
- [ ] 远程怪：贴近到 PreferredRange 即停 → 施放远程技能 → 冷却期间不抽搐（SelectSkill Failed 时树的等待/降级路径正常）
- [ ] 双技能怪（近战+远程）：远程冷却中自动用近战补（Priority 生效）
- [ ] 召唤宠 Manual 技能不被自动释放；标 Auto 的正常自动放（门禁迁移无回归）
- [ ] 指令系统：手动指令技能不受影响（走的是事件管道，不经 Selector）
- [ ] 技能被打断（濒死/被抱起 StopStateTree）时 CancelAbilityHandle 正确触发，无卡状态标签

---

## 6. 与既有系统的关系

- **指令系统**：不动。`SendStateTreeEvent` 管道、`STCond_CommandTypeIs` / `STTask_ExecuteCommandSkill` 原样保留；Selector 只管"自动"轴。
- **GAS 冷却**：复用 `UMFPetGameplayAbility` 现有 GAS 原生冷却（AbilityTag 身份标签），Selector 只读不改。
- **Mass Phase 2**：Selector 是纯函数层，不依赖 StateTree，将来 Mass processor 可直接调用。

## 7. v2 候选（按需再排）

1. 风筝：目标 < MinRange 时反向走位（MoveToSkillRange 补"拉开"分支）；
2. 冲锋/跳跃类移动技能进决策（"距离远且冲锋就绪 → 用冲锋接近"）；
3. LOS 检查（地图出现遮挡物后）；
4. Priority → 效用评分（血量/距离/仇恨加权）。
