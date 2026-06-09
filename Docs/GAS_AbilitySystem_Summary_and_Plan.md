# ProjectMF — GAS 技能系统总结 & 扩展规划

> 版本：2026-06-09（v2，含技能基类重构 + 区域/Combo 子系统方向）
> 范围：`Source/ProjectMF/GAS`、`Source/ProjectMF/Projectile`、属性/角色相关
> 用途：① 沉淀现有 GAS 架构；② 规划「技能基类重构 + 3 个移动技能 + 投掷物区域化 + 区域/Combo 子系统 + 多类新 GE」的方案与排期。

---

# 第一部分：现有 GAS 系统总结

## 1. 架构总览

ProjectMF 的战斗能力全部基于 UE5 的 **Gameplay Ability System (GAS)**，由六块组成：

| 模块 | 职责 | 位置 |
|------|------|------|
| **ASC** | `UAbilitySystemComponent`，挂在每个 `AMFCharacterBase` 上，持有技能/标签/属性集 | `MFCharacterBase` |
| **属性集 AttributeSet** | 数值载体（血量/攻防/移速等） | `MFAttributeSetBase` / `MFCombatAttributeSet` / `MFPlayerAttributeSet` |
| **技能 GA** | `UGameplayAbility` 派生，前缀 `GA_`，承载技能逻辑 | `GAS/Public/GA_*` |
| **效果 GE** | `UGameplayEffect`，蓝图资产，改属性 / 加标签 | `Content/Gameplay/GAS/GameplayEffect/*`（目前只有 `GE_Damage`、`GE_Init_*`） |
| **数据资产 DataAsset** | 把技能参数从代码里抽出，运行时可调 | `MFAttackDataBase` 体系 |
| **投射物子系统** | 轻量 ISM 投射物模拟，与 GA 解耦 | `Projectile/MFProjectileSubsystem` |
| **标签 GameplayTags** | 状态/阵营/技能/伤害键的统一声明 | `MFGameplayTags` |

## 2. 技能（GA）继承关系（现状）

```
UGameplayAbility (引擎)
└── UMFGameplayAbilityBase ............ 抽象基类，仅提供 GetMFCharacter()
    ├── UGA_Pick / UGA_CatchPet / UGA_SummonPet  （玩家）
    ├── UGA_AIAttackBase ............. 【近战/AOE 基类】(Abstract)
    └── UGA_AIRangedAttackBase ....... 【远程基类】(Abstract)
            ├── UGA_ThrowProjectile / UGA_FallingBoulder / UGA_BulletCurtain
```

> 第二部分会在此之上插入「拥有者轴」基类（Player / Pet）。

## 3. 数据资产继承关系（2026-06-09 重构后）

```
UMFAttackDataBase : UDataAsset          ← 通用：AttackAnim / DamageGE / DamageMultiplier / TargetFilter
   ├── UMFAttackAbilityData (近战)       + 形状/时序/多段/持续
   └── UMFRangedAttackDataBase           ← 远程通用：AnimToSpawnDelay / Speed / MaxRange / CollisionRadius / ProjectileMesh
         ├── UMFProjectileAttackData     + bSplashOnMaxRange / SplashRadius
         ├── UMFFallingBoulderData       + ImpactRadius（MaxRange 复用为下落高度）
         └── UMFBulletCurtainData        + 弹幕 burst 参数
```

> 约定：每个 GA 在编辑器里**只暴露一个数据资产字段**；远程基类通过虚函数 `GetRangedData()` 取子类资产。

## 4. 属性与伤害管线

| 属性集 | 属性 | 说明 |
|--------|------|------|
| `UMFAttributeSetBase` | `Health` / `MaxHealth` | `PreAttributeChange` 夹紧 `[0, MaxHealth]` |
| | `MoveSpeed` | ⚠️**当前仅 debug 显示，未同步到 `MaxWalkSpeed`** |
| | `Damage`（meta） | 瞬时伤害中转，`PostGE` 消费后清零 |
| `UMFCombatAttributeSet` | `Attack` / `Defense` / `FleeThreshold` | 攻击力(SetByCaller) / 平砍减免 / 逃跑阈值 |
| `UMFPlayerAttributeSet` | —（空 stub） | 预留 |

**伤害流程：**`GA.ApplyDamageToTarget`（`Attack × DamageMultiplier` → SetByCaller `MF.Attack.Data.Damage`）→ `GE_Damage` 写入 `Damage`(meta) → 目标 `PostGE`：`FinalDamage = max(Damage − Defense, 1)` → 扣血 → 广播 `OnHealthChanged/OnDeath/OnLowHealth`。GE 是**蓝图资产**（无 C++ ExecCalc），伤害靠 **SetByCaller**。回复目前无对称管线。

## 5. 标签体系（`MFGameplayTags`）

状态 `State.Picking/Dead/InCombat/Attacking/RangedAttacking`；技能 `Ability.*`；阵营 `Team(.Player/.Enemy/.Boss)`；伤害键 `Attack.Data.Damage`；抓宠 `Catching.*`；感知 `AI.Perception.HasTarget`。

## 6. 技能激活路径

- **玩家**：输入 → `TryActivateAbilitiesByTag`；或 `SendGameplayEvent`（召唤携带 slot）。
- **AI（含宠物/敌人/Boss）**：StateTree Task 按 `AbilityTags` 激活；或 `FMFAICommand.AbilityTagToActivate`。
- **授予**：`AMFCharacterBase::DefaultAbilities` 在 `BeginPlay` 授予；初始属性由 `DefaultInitEffect` 写入。

## 7. 投射物子系统（`UMFProjectileSubsystem`）

World Subsystem + FTickableGameObject，slot 化实例数组，ISM 渲染，每帧 sweep + 距离判定。**不负责伤害**，命中/到达射程/取消 → 回调 GA 的 `FOnProjectileResolved` 由 GA 处理。API：`Launch → Handle`、`Cancel(Handle)`。**这是投掷物区域化的扩展点**。

---

# 第二部分：扩展需求规划（v2，已按拍板方向更新）

## 0. 已确认的架构方向

| 主题 | 决定 |
|------|------|
| 技能分类 | 加「拥有者轴」基类：`UMFPlayerGameplayAbility` / `UMFPetGameplayAbility`。**Pet 基类涵盖所有 AI 战斗者（宠物+敌人+Boss）**。本批新技能全部归 **Pet**（玩家技能会另行提示）。门禁细则待策划案更新后补。 |
| 投掷物 | 投掷数据资产自带「撞击 DamageGE（命中瞬间伤害）」+ **可选区域配置**。命中 actor 或飞行结束时：无区域→结束；有区域→在落点向**区域子系统**注册区域。 |
| 区域 | 统一进 **`UMFAreaEffectSubsystem`**（圆形）。区域携带一组 GE，对范围内目标施加。 |
| GE 施加模式 | 由 GE 的 DurationPolicy 推导：**Instant** → 区域每 tick 施加（燃烧持续掉血）；**Duration** → 进入施加一次、离开移除（缠绕/冰冻/减速）。 |
| GE 组织 | 引入 **`UMFGameplayEffectBase : UGameplayEffect`** 基类承载共享功能（效果身份标签等），具体效果为其 BP 子类。 |
| Combo | 叠加发生在**目标身上**。独立 **`UMFComboSubsystem`**：区域子系统结算完目标的效果标签后，交给 Combo 系统查数据表，(TagA,TagB) 同时满足 → 施加一个新 GE。**先两两**；**同种效果叠层 = TODO 后续迭代**。 |
| 表现/性能 | 后置，先把逻辑跑通。 |

## A. 技能基类重构（拥有者轴）

```
UMFGameplayAbilityBase（抽象·通用）
│   └─ 通用门禁：ActivationBlockedTags += State.Dead
│
├── UMFPlayerGameplayAbility ……… 玩家技能基类
│     ├── UGA_Pick / UGA_CatchPet / UGA_SummonPet
│
└── UMFPetGameplayAbility ……… AI 战斗技能基类（宠物 + 敌人 + Boss）
      └─ 门禁：State.Dead + State.Stunned（+ 未召唤/逃跑中等动态门禁，细则待定）
      ├── UGA_AIAttackBase（近战，reparent 到此）
      ├── UGA_AIRangedAttackBase（远程：throw/boulder/curtain，reparent 到此）
      └── UMFMovementAbilityBase（charge/jump/groundslam，见 G）
```

- **静态门禁**：`ActivationBlockedTags`（构造里设）。
- **动态门禁**：override `CanActivateAbility()`（如「宠物未激活」「逃跑中」）。
- **改动**：新建两个基类；把现有 3 个玩家 GA reparent 到 Player 基类、两个攻击基类 reparent 到 Pet 基类（reparent 仅改父类，不动各自逻辑）。
- 门禁清单待**策划案更新后再补**，此阶段先搭骨架 + 通用 Dead/Stunned。

## B. 基础设施补强（属性 / 伤害管线 / 标签）

| # | 任务 | 涉及文件 | 说明 |
|---|------|----------|------|
| B1 | **MoveSpeed → MaxWalkSpeed 同步** | `MFCharacterBase.cpp` | ASC 注册 `GetMoveSpeedAttribute()` 变化回调 → 写 `MaxWalkSpeed`。减速/定身依赖。 |
| B2 | **伤害修正属性** | `MFCombatAttributeSet` | `IncomingDamageMultiplier`（易伤/减伤，默认1）、`OutgoingDamageMultiplier`（增伤，默认1），夹紧 `[0,+∞)`。 |
| B3 | **Healing meta + 对称管线** | `MFAttributeSetBase` | 加 `Healing`(meta)；`PostGE` 对称处理 `Health += Healing` 并复用 `OnHealthChanged`。 |
| B4 | **伤害公式接修正系数** | `MFAttributeSetBase` PostGE | `FinalDamage = max(Damage − Defense, 1) × IncomingDamageMultiplier`（已实现）。 |
| B5 | **出伤接增伤系数** | 近战/远程 GA 的 `ApplyDamageToTarget` | `FinalMagnitude = Attack × DamageMultiplier × OutgoingDamageMultiplier`。 |
| B6 | **新标签** | `MFGameplayTags` | 状态 `State.Stunned/Slowed/Blinded`；技能 `Ability.Charge/Jump/GroundSlam`；效果身份 `Effect.Burn/Freeze/Root/Slow/Vulnerable/DamageUp/Heal/Blind...`（combo 用）。 |
| B7 | **眩晕禁动** | `MFCharacterBase.cpp` | `State.Stunned` 标签事件：加→`DisableMovement()`、移除→恢复。 |
| B8 | **致盲目标失效** | `MFThreatComponent` 等 | 持有 `State.Blinded` 时目标获取返回空。 |
| B9 | **属性初始化配置化（扁平）** | `MFAttributeInitData` + 3 配置 + 2 apply 点 | 用 `FMFAttributeInitData` 扁平结构体取代 `DefaultInitEffect`(GE)，代码 `SetNumericAttributeBase` 直接 init；见下。 |

### B9 细化：属性初始化从 init-GE 改为配置结构体

- **动机**：初始数值就地明文可见、配置内聚、少一类 `GE_Init_*` 资产、代码可控可调试。
- **结构体** `FMFAttributeInitData`（新文件 `GAS/Public/MFAttributeInitData.h`）：`MaxHealth / MoveSpeed / Attack / Defense / FleeThreshold`（Health 自动 = MaxHealth；增减伤系数恒 1 不入配置）。**当前为扁平值，无等级缩放。**
- **应用助手** `AMFCharacterBase::ApplyAttributeInitData(Data)`：用 `SetNumericAttributeBase` 写基础集；战斗集（Attack/Defense/FleeThreshold）仅在 `GetSet<UMFCombatAttributeSet>()` 存在时写（玩家无战斗集）。
- **替换面**：`DefaultInitEffect` 从 `AMFCharacterBase` / `UMFPlayerConfig` / `UMFAIConfig` 移除；2 个 apply 点（`InitAbilitySystemComponent`、`AMFAICharacter::ApplyAIConfig`）改调助手；`UMFPetConfig` 继承、召唤路径"配置 init → 快照覆盖"顺序不变。
- **迁移成本**：原 `GE_Init_*` 数值需手填进各配置的 `InitAttributes`；旧 GE 资产随后可删。
- **取舍记录**：放弃了 GE 的等级曲线（ScalableFloat）能力，但本项目 init 本就是扁平、成长走快照，当前不损失实用能力。等级缩放留给养成系统（见 Part 3 末）。

## C. GE 体系（基类 + 具体效果）

- **`UMFGameplayEffectBase : UGameplayEffect`**：承载共享功能——`EffectTag`（效果身份，combo 用，如 `Effect.Burn`）、标准 SetByCaller magnitude 键约定；并把 `EffectTag` 纳入 GrantedTags（使目标身上带上该效果标签，供区域结算与 combo 读取）。
- **具体效果 = BP 子类**，放 `Content/Gameplay/GAS/GameplayEffect/`，强度走 SetByCaller：

| GE | DurationPolicy | 机制 | 效果标签 |
|----|------|------|----------|
| `GE_Burn` | Instant（区域每 tick 施加）| 写 Damage meta | `Effect.Burn` |
| `GE_Freeze` | Duration | 禁动（同眩晕，或独立 Frozen 标签）| `Effect.Freeze` |
| `GE_Root` 缠绕 | Duration | 禁移动（MoveSpeed×0 或禁动）| `Effect.Root` |
| `GE_Slow` | Duration | `MoveSpeed × (1−slow%)` | `Effect.Slow` |
| `GE_Stun` | Duration | GrantsTags `State.Stunned` | `Effect.Stun` |
| `GE_Vulnerable` | Duration | `IncomingDamageMultiplier ×= k` | `Effect.Vulnerable` |
| `GE_DamageUp` | Duration | `OutgoingDamageMultiplier ×= k` | `Effect.DamageUp` |
| `GE_Heal` | Instant | 写 Healing meta | `Effect.Heal` |
| `GE_Regen` | Duration+Period | 每 Period 写 Healing meta | `Effect.Heal` |
| `GE_Blind` | Duration | GrantsTags `State.Blinded` | `Effect.Blind` |

## D. 投掷物（携带区域配置）

**`UMFProjectileAttackData` 调整：**
- 保留 `DamageGE`（命中瞬间伤害，沿用现有出伤）。
- 新增可选 `TObjectPtr<UMFAreaEffectData> AreaConfig`：非空则在命中/结束落点向区域子系统注册区域。

**`GA_ThrowProjectile::HandleProjectileResolved` 调整：**
- `HitTarget`：施加 DamageGE（命中瞬间伤害）；若 `AreaConfig` 非空 → 在 `Result.FinalPosition` 注册区域。
- `MaxRange`（飞行结束）：若 `AreaConfig` 非空 → 注册区域；否则结束。
- 无区域配置 → 维持现状结束。

## E. 区域效果子系统 `UMFAreaEffectSubsystem`（新）

**数据资产 `UMFAreaEffectData`：** `Radius` / `Duration` / `TickInterval` / `TargetFilter` / `Effects[]`（`TSubclassOf<UMFGameplayEffectBase>` 列表）/ `DamageMultiplier`（含伤害 GE 时走 SetByCaller）/（表现后置：Decal/Niagara）。

**子系统职责（World Subsystem，参照投射物子系统 slot 化 + Tick）：**
1. `RegisterArea(Instigator, Data, Location) → Handle`：登记一个圆形区域。
2. **Overlap 跟踪**：维护「进入/离开」事件，得到每个区域当前覆盖的 actor，以及反向的「受影响 actor 集合」。
3. **逐 actor Update**（对每个 overlap 注册的 actor）：
   - 计算它当前身处哪些区域 → 应用各区域的 `Effects`：
     - **Instant 策略 GE** → 每 `TickInterval` 施加一次（燃烧/回复跳动）。
     - **Duration 策略 GE** → 进入时施加一次、离开时移除（缠绕/冰冻/减速）。
   - 结算后，该 actor 身上的「效果标签集合」即为最新。
4. **结算完 → 调用 Combo 子系统**（见 F），传入该 actor 的效果标签集合。
5. 区域到期自销毁；移除其施加的 Duration 型效果。

> 阵营过滤复用 `MFFactionStatics`；伤害复用现有 `ApplyDamageToTarget` 思路（SetByCaller）。

## F. Combo 子系统 `UMFComboSubsystem`（新）

- **输入**：某 actor 当前的效果标签集合（由区域子系统在 Update 末尾传入）。
- **数据表**：`(TagA, TagB) → 结果 GE`（`UDataTable`，行结构 `{ TagA, TagB, ResultGE, ... }`）。
- **逻辑**：两两扫描 actor 的效果标签，命中表中某行且两者**同时存在** → 对该 actor 施加 `ResultGE`（如燃烧+缠绕 → `GE_ComboBurnRoot` 造成额外伤害）。
- **解耦**：对效果来源无感——任何来源（区域/投掷直接命中/近战 debuff）打到目标身上的 `Effect.*` 标签都能参与。区域子系统只是当前的调用方。
- **范围**：先**两两**组合；**同种效果叠层 → TODO（后续迭代）**；多元（>2）组合后续再说。

## G. 移动/战斗技能 ×3（均属 Pet 基类）

**基类 `UMFMovementAbilityBase : UMFPetGameplayAbility`**（共享：门禁、击退工具、沿途 sweep、落点 AOE/区域注册工具）。

| 技能 | 数据资产 | 机制 |
|------|----------|------|
| 冲撞 `GA_Charge` | `UMFChargeData` | 朝向高速突进（LaunchCharacter/Timeline），沿途 sweep；命中敌人施加 DamageGE + 可选击退/控制 GE，每目标一次。 |
| 跳跃 `GA_Jump` | `UMFJumpData` | 抛物线跳到目标点/方向；落地可选触发 AOE/注册区域；可作为撼地前置连段。 |
| 撼地 `GA_GroundSlam` | `UMFGroundSlamData` | 原地砸地径向 AOE：伤害 + 击退 + 可选 GE_Stun/Slow；可选在落点向**区域子系统**注册区域（地裂减速区等）。可复用近战 AOE 的 collect/filter/damage。 |

---

# 第三部分：排期（v2）

> 单人 + AI 协作的**粗略人天**，按依赖排序；日期锚定 2026-06-09。

| 阶段 | 内容 | 关键产出 | 估算 | 依赖 |
|------|------|----------|------|------|
| **P0a 地基·管线** | B1–B9：属性 / MoveSpeed 同步 / Healing 管线 / 伤害公式 / 增伤接入 / 标签 / 禁动 / 致盲钩子 / **属性初始化配置化** | 编译通过 + debug 验证数值 | **3–4d** | — |
| **P0b 地基·基类** | A + C 基类：`UMFPlayer/PetGameplayAbility`、reparent 现有 GA、`UMFGameplayEffectBase` | 编译通过 + 现有技能回归正常 | **2d** | — |
| **P1 GE 体系** | C 具体效果：burn/freeze/root/slow/stun/vulnerable/dmgup/heal/regen/blind 蓝图 + 效果身份标签 | 测试技能逐个验证 GE | **2–3d** | P0a/P0b |
| **P2 区域子系统** | E：`UMFAreaEffectData` + `UMFAreaEffectSubsystem`（overlap 跟踪、逐 actor Update、Instant/Duration 双模式施加） | 手动注册区域可对范围目标生效 | **3–4d** | P1 |
| **P3 Combo 子系统** | F：`UMFComboSubsystem` + combo 数据表；区域子系统结算后调用 | 燃烧+缠绕触发额外 GE | **2–3d** | P2 |
| **P4 投掷区域化** | D：`UMFProjectileAttackData` 加 `AreaConfig`；`GA_ThrowProjectile` resolve → 撞击伤害 + 注册区域 | 投掷弹落点生成可玩区域 | **1–2d** | P2 |
| **P5 移动技能** | G：`UMFMovementAbilityBase` + 击退工具；Charge/Jump/GroundSlam（+落点区域） | 3 个技能宠物可用 | **4–5d** | P0b（撼地区域需 P2） |
| **P6 整合联调** | AI StateTree 接线、数值调优、debug 可视化、回归一阶段循环 | 全部技能在战斗中验证 | **2–3d** | P1–P5 |

**总计：约 19–26 人天（~4–5 周）。**

**里程碑：**
1. **M1（P0a+P0b+P1，~1.5 周）**：地基 + 基类重构 + GE 全生效——解锁后续一切。
2. **M2（P2+P3+P4，~2 周）**：区域子系统 + Combo + 投掷区域化——「目标身上多效果叠加」跑通。
3. **M3（P5+P6，~1.5 周）**：三个移动技能 + 全面联调。

**风险与 TODO：**
- B1（MoveSpeed 同步）当前完全缺失，减速/定身/缠绕全依赖，最先验证。
- B3/B4 改 PostGE 属于核心伤害链路，需回归现有伤害/死亡/逃跑逻辑。
- 致盲（B8）依赖威胁/感知系统目标获取可注入「失明」短路，先确认接口。
- **TODO（后续迭代）**：同种效果叠层（如燃烧叠 N 层）、多元（>2）combo、区域表现（Decal/Niagara）、区域/combo 性能与数量级评估。
- 跳跃/冲撞在「2D 贴图 + 3D 场景」下的手感（抛物线、billboard、落地判定）需美术/动画配合，预留调优。

---

# 第四部分：后续 — 养成 / 等级系统（规划，本批不做）

> 由 B9（属性初始化配置化）讨论延伸而来。当前 B9 只做**扁平初始值**；等级缩放整体推迟到本系统。

**「等级成长」拆两层**：① 等级来源（实体需 `Level`，宠物还需 `XP` + 升级逻辑）；② 等级→属性映射。GE 的 ScalableFloat 只省第②层，第①层无论如何都要自建。

**已定方向（避免曲线的复杂）——线性「基值 + 每级成长」**：
```
属性(Lv) = 基值 + 每级增量 × (Lv − 1)
```
- **存储**：用中心 DataTable `DT_CharacterStats`，RowName = `AIConfigID`（复用现有 `DT_AIRegistry` 模式），行结构含各属性的 `Base` + `PerLevel`；可 CSV 导入、便于横向平衡。
- **应用**：`ApplyAttributeInitData` 增加 `Level` 入参 → 查表 → 线性现算。**这是唯一 init 入口，切换时只改这一处**，B9 的接口不变。
- **宠物快照简化**：从「存裸属性」改为「存 `Level + XP`」，召唤时按 Level 现算，单一事实来源，杜绝"快照与等级对不上"的脏数据。
- **玩家 / 无 AIConfigID 的关卡 AI**：补 row key（如 "Player"）或回退到 B9 的内联扁平值。

**待设计**：升级曲线/经验需求、品级/随机浮动、敌人/Boss 按关卡难度的 Level 来源、UI 展示。排期待策划案细化后插入。
