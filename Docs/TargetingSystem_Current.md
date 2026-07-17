# 索敌 / 目标选择机制 —— 现状整理

> 2026-07-13 整理 ｜ 目的：作为"更新索敌机制"前的**现状基线**，理清当前实现、真源、以及未接线的占位。
> 一句话：**纯 C++ 组件化管线** —— 球形雷达感知 → 威胁列表选最近敌人 → StateTree 读目标 → 攻击 GA 二次过滤；阵营用 `MF.Team.*` GameplayTag（三态：敌/友/中立）。**逃跑、仇恨值、致盲失效** 三块是占位，未接线。

---

## 0. 整体数据流（AI 出生 → 发现敌人 → 攻击）

```
AMFSpawnAIManager::SpawnSinglePet
  → AMFPetBase / AMFAICharacter（构造已带 RadarSensingComp + ThreatComp）
  → ApplyPetConfig（写感知/威胁配置） → SetFaction(Team.Enemy) → RunStateTree
       │
   ┌───┴─── UMFRadarSensingComponent：每 0.15s 球形 overlap(ECC_Pawn) → AreHostile 过滤
   │            └─ OnTargetDetected / OnTargetLost（委托）
   ├─────── UMFThreatComponent：订阅雷达 → EvaluateTargets → ScoreTarget(最近者胜)
   │            └─ CurrentTarget + 给 ASC 加 MF.AI.Perception.HasTarget tag
   └─────── AMFPetAIController（UStateTreeAIComponent）
                └─ 以 HasTarget tag 为条件进 Combat 状态
                     → FSTTask_GetThreatTarget（把 CurrentTarget 写入 ST 变量 TargetActor）
                     → 内置 MoveTo 靠近/面向 → FSTCond_CanAutoUseSkill 门控
                     → FSTTask_ActivateAttack（TryActivateAbility）
                          ├─ 近战 GA_AIAttackBase：身前形状 overlap → FilterTarget → ApplyDamage
                          └─ 远程 GA_AIRangedAttackBase：取 CurrentTarget → 发射投射物
                                                     → UMFProjectileSubsystem 命中判定
       所有伤害层都过 UMFCombatStatics::PassesTargetFilter → UMFFactionStatics::AreHostile
```

**关键类调用链**：`AMFSpawnAIManager` →（`UMFRadarSensingComponent` → `UMFThreatComponent`）→ `AMFPetAIController` → `FSTTask_GetThreatTarget` / `FSTTask_ActivateAttack` → `GA_AIAttackBase` / `GA_AIRangedAttackBase` → `UMFCombatStatics::PassesTargetFilter` → `UMFFactionStatics::AreHostile`。

---

## 1. 阵营判定（敌 / 友 / 中立）

- **表示**：阵营 = ASC 上的 Loose GameplayTag `MF.Team.*`（**非枚举**）。定义 `MFGameplayTags.h:134-143`：`Team_Player` / `Team_Enemy` / `Team_Boss`（父 `MF.Team`）。
- **真源**：`UMFFactionStatics`（`GAS/…/MFFactionStatics.cpp`）
  - `GetTeamTags(ASC)` `:7` — 收集所有 `MatchesTag(MF.Team)` 的标签。
  - `SetFaction(ASC, Tags)` `:25` — 先清旧 `MF.Team.*` 再加新；传空 = 清为中立。
  - `AreSameTeam(A,B)` `:40` — 任一空/中立 → false；否则 `TeamB.HasAny(TeamA)`。
  - `AreHostile(A,B)` `:51` — 任一 ASC 空 / 任一方 Team 为空（中立）→ **false**；双方都有 Team 且不共享 → **true**。
- **三态语义**（`.h:42-49`）：同队 / 敌对 / 双方中立 —— **`!AreSameTeam ≠ AreHostile`**。全工程"敌方"统一走 `AreHostile`，中立不误伤。
- **阵营在哪赋予**：
  - `SpawnAIManager` 生成的 AI：构造 `SpawnFactionTags = Team.Enemy`，`SpawnSinglePet` 里 `SetFaction` **覆盖** config（`MFSpawnAIManager.cpp:206-210`）—— 同一宠类型野外是敌、被召唤是友，阵营由 spawn 入口决定。
  - 关卡直接摆的 AI：`MFAIConfig.DefaultOwnedTags`（通常含 `Team.Enemy`）。
  - 召唤宠：召唤时翻成 `Team.Player`。
  - **玩家 config 须含 `Team.Player`**（否则玩家被判中立，宠物不会认它为友）。

---

## 2. AI 感知（怎么发现目标）

- **不用 UE AIPerception**，自研球形雷达 `UMFRadarSensingComponent`（`AI/…/MFRadarSensingComponent`）。
- **创建**：`AMFAICharacter` 构造建 `RadarSensingComp`（`MFAICharacter.cpp:42`）+ `ThreatComp`（`:45`），所有 AI/宠/Boss 共用基类都自带。
- **配置** `FMFRadarSensingConfig`（`.h:24-40`），来源 `UMFPetConfig::RadarConfig`：
  - `SensingRadius = 1000`、`ScanInterval = 0.15s`、`SensingChannel = ECC_Pawn`。
  - **无视野角度、无视线遮挡、无丢失时间**（纯球形，适配 2D 俯视；丢失在威胁层处理）。
- **扫描** `PerformScan()` `:158`：`OverlapMultiByChannel(ECC_Pawn, Sphere)` → 每个候选 `AreHostile` 过滤 → 差集比较 → 新进/离开广播。**感知阶段就按阵营过滤**（只把敌对放进列表）。
- **输出**：委托 `OnTargetDetected` / `OnTargetLost`（`.h:93,97`）给威胁组件；`GetPerceivedActors()` 供查询。雷达**本身不写黑板、不授 tag**。

---

## 3. AI 索敌决策（选哪个目标）—— 核心

- **载体**：`AMFPetAIController` 持 `UStateTreeAIComponent`，`RunStateTree(PetConfig->StateTreeAsset)`（Possess 后）。
- **选目标真源**：`UMFThreatComponent`（**非 EQS、非 StateTree 内遍历**）。
  - 数据：`ThreatRecords`（每条 = Actor + GraceTimer + bInSensingRange），状态 `EMFThreatState { Idle, Locked }`。
  - `EvaluateTargets()` `:276` → `ScoreTarget` 取最高 → `Locked`。
  - **`ScoreTarget(Target, OwnerLoc)` `:315`**：`Dist > EngagementRadius → -BIG`；否则 `return EngagementRadius - Dist`（**越近分越高，纯距离**）。`:325` 注释留了"仇恨值/Boss权重/HP%"扩展点，**当前未叠加**。
  - **当前目标**：`CurrentTarget`（`TWeakObjectPtr`），`SetCurrentTarget` `:329` 联动在 ASC 增删 `MF.AI.Perception.HasTarget` tag（供 StateTree 进 Combat 的条件）。
- **StateTree 取目标**：`FSTTask_GetThreatTarget` — `EnterState` 把 `GetCurrentTarget()` 写入 ST 变量 `TargetActor`；`Tick` 每帧刷新，目标丢失 → `Failed`（退 Combat）。
- **配置** `FMFThreatConfig`（`.h:64-96`，来源 `PetConfig->ThreatConfig`）：
  - `EngagementRadius = 600`（选目标距离，clamp ≤ SensingRadius）。
  - `LockDuration = 5s` 宽限：目标离开感知 → 启 GraceTimer，宽限内**仍锁定可追击**；期满移除重评估；返回则取消宽限。
  - 目标死亡：`TWeakObjectPtr` 失效 → `CleanupThreatList` 清 → 重选。
- **接近/面向**：**无自定义 C++ MoveTo 任务**（只有 `STTask_FindRandomNavPoint` 巡逻）；靠近/面朝靠 StateTree 内置 MoveTo 绑 `TargetActor`（在 ST 资产配，代码不可见）。

---

## 4. 攻击目标过滤

- **枚举** `EAttackTargetFilter { EnemyOnly, AllyOnly, All }`（`MFAttackTypes.h:27`，默认 EnemyOnly）。
- **唯一真源** `UMFCombatStatics::PassesTargetFilter(SourceASC, Candidate, Filter)`（`MFCombatStatics.cpp:90`）：
  1. 空 → false；2. **跳过 `State.Dead` / `State.Carried`**；3. `All`→true；4. `EnemyOnly→AreHostile`，`AllyOnly→AreSameTeam`。
- **近战** `GA_AIAttackBase`：`CollectTargets` 按 ShapeType（Sphere/Sector/Box）在**身前形状区域** overlap(ECC_Pawn) —— **不直接用威胁目标**（威胁目标只负责让 AI 挪到/转向敌人）→ 逐个 `FilterTarget` → `ApplyDamage`。
- **远程** `GA_AIRangedAttackBase`：`ActivateAbility` 取 `GetCurrentTarget()`（威胁目标）→ 发射 → 同一个 `PassesTargetFilter`。
- **投射物** `UMFProjectileSubsystem::PassesTargetFilter`（`:253`）：镜像逻辑（跳过 Dead/Carried + `AreHostile`/`AreSameTeam`）。`TargetFilter` 来自发射参数。
- **区域场** `MFAreaEffectSubsystem::AreaPassesFilter`（`:29`）：同语义**第四次复刻**（未调用 CombatStatics，但一致）。
  > ⚠️ 过滤逻辑在 CombatStatics / 投射物 / 区域场 **三处各写一遍**（语义一致但重复）——更新时留意同步。

---

## 5. 玩家侧索敌

**玩家自己没有战斗索敌**：玩家无攻击敌人的 GA（技能仅 Pick/CatchPet/SummonPet/CommandMode/CarryPet/RevivePet）。战斗全靠宠物 AI。玩家侧只有两类"选目标"：

- **命令模式** `UMFCommandComponent`（挂 PlayerController）：
  - `GetPetUnderCursor()` `:193` — 光标选**自己的召唤宠**（须持 `Pet_Summoned` tag），**不是选敌人**。
  - `GetGroundPointUnderCursor()` `:216` — 地面点（移动/投掷落点）。
  - 下达指令 → `UMFPetCommandComponent::IssueCommand` 存 `PendingCommand` + 发 StateTree 事件 `Event_PlayerCommand`。
  - **关键**：玩家只指定"哪只宠 + 放哪个技能 / 去哪"，**打谁仍由该宠威胁组件的当前目标决定**（`FMFPetCommand.TargetActor` 字段存在但玩家路径未填）。
- **抓宠瞄准** `GA_CatchPet` → `AT_WaitPetTarget`：光标 trace(ECC_Pawn) 命中实现 `IMFCatchable` 的野宠，高亮 + 左键确认。与战斗索敌无关。

---

## 6. 逃跑 / 仇恨 / 濒死排除

- **(a) 逃跑 —— ⚠️ 占位未接线**：`FleeThreshold`（`MFCombatAttributeSet.h:62`，默认 0.3）存在；`MFAttributeSetBase.cpp:87` 血量 < MaxHealth×FleeThreshold 时 `OnLowHealth.Broadcast`。**但全工程无订阅者**，无 Flee 状态。算得出、发得出，**没接线，无逃跑行为**。
- **(b) 仇恨 —— ⚠️ 纯"最近敌人"**：`ScoreTarget` 只按距离。唯一能强改目标的是**嘲讽** `GA_Taunt` → `ThreatComponent::ApplyTaunt` → `EvaluateTargets` 嘲讽守卫强制锁定嘲讽源（定时强制，非累积仇恨值）。
- **(c) 濒死 / 被抱 排除**：**双重机制**：
  1. **碰撞关闭**（`BeginCarried` / `EnterDowned` 都 `SetActorEnableCollision(false)`）→ 不在雷达 overlap 结果里 → 根本感知不到。
  2. **Tag 过滤**（`PassesTargetFilter` 排除 `State.Dead` / `State.Carried`）→ 即使感知到也不受伤害。
  > ⚠️ **不对称**：雷达 `IsHostile` 和 `ScoreTarget` **不查 Dead/Carried tag**，排除**完全依赖碰撞关闭**。若某目标带 tag 却仍开碰撞，会进威胁列表被锁定（但伤害仍被 filter 挡下 → "锁而打不到"）。濒死用 `bDowned` bool（非 tag）。
  > ⚠️ **`State.Blinded` 未接线**：`GE_Blind` 会授予 tag，但雷达/威胁代码**无任何 `State_Blinded` 判断** —— 致盲当前不影响索敌。

---

## 7. 未实现 / 占位清单（更新时的候选切入点）

| # | 项 | 现状 |
|---|---|---|
| 1 | **逃跑** | FleeThreshold 算得出、OnLowHealth 发得出，**无消费者**、无 Flee 状态 |
| 2 | **仇恨值系统** | 仅最近距离打分 + 嘲讽强制锁定；ScoreTarget `:325` 留了扩展点 |
| 3 | **致盲失效索敌** | `State.Blinded` tag 无消费者，不影响索敌 |
| 4 | **威胁值/Boss 权重/血量权重** | ScoreTarget 未叠加 |
| 5 | **玩家指定攻击目标** | `FMFPetCommand.TargetActor` 字段在，玩家路径未填（打谁始终威胁组件定） |
| 6 | **过滤逻辑三处重复** | CombatStatics / 投射物 / 区域场 各写一遍，未收敛 |
| 7 | **濒死/被抱排除依赖碰撞** | 雷达/威胁层不查 Dead/Carried tag，靠碰撞关闭兜底（不对称） |

---

## 8. 更新方向备忘（待用户明确）

本文档是现状基线。"更新索敌机制"可能的方向（供讨论，未定）：
- **加威胁值/仇恨系统**：在 `ScoreTarget` 叠加（受击累积、Boss 权重、嘲讽并入统一打分）。
- **接线逃跑**：消费 `OnLowHealth` → StateTree Flee 状态。
- **玩家指定目标**：命令模式选敌人 → 填 `FMFPetCommand.TargetActor` → 威胁组件优先。
- **收敛过滤逻辑**：投射物/区域场改调 `UMFCombatStatics::PassesTargetFilter` 单一真源。
- **接线致盲**：雷达/威胁层加 `State.Blinded` 判断。
- **濒死排除改 tag 驱动**：雷达/威胁层显式查 Dead/Carried，不只依赖碰撞。
