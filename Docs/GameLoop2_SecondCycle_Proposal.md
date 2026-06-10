# 第二循环策划案 ——「可玩版本：局外元层 + 撤离带出」

> 状态：草案 v1（2026-06-10）
> 定位：在第一循环（竞技场战斗闭环）之上，套一层**局外元层**，把工程从"一个验证关卡"升级为**有头有尾的可玩版本**。
> 一句话目标：玩家能从**主菜单进入 → 选关 → 编队带宠入局 → 打第一循环 → 撤离/获胜把宠物带出局外 → 存档 → 下次再带入**，并填充 **2 个 Boss + 10 只差异化 Pet** 的内容量。

---

## 0. 与第一循环的关系

- **第一循环（已基本落地）** = 单关卡内的战斗闭环：抓宠倒计时 → 生成围墙 + Boss → 宠物自动战斗 → 胜利/失败。由 `AMFGameMode` 的 `M1_*` 区块编排（[MFGameMode.cpp](../Source/ProjectMF/GameLoop/Private/MFGameMode.cpp)）。**第一循环的收尾标志 = 技能实现完成**。
- **第二循环（本案）** = **局内不动**，新增**局外（元层）**把多次局内串成一条玩家旅程。第二循环达成 = 一个**可对外演示的完整可玩版本**。

> 关键现状：**当前没有任何持久层**。宠物数据 `FMFPetInstance` 全部挂在玩家角色的 `UMFInventoryComponent.PetSlots` 上（[MFInventoryComponent.h](../Source/ProjectMF/Inventory/Public/MFInventoryComponent.h)），关卡一 `OpenLevel` 重载就销毁。没有 `GameInstance` 子类、没有 `SaveGame`、没有主菜单、没有关卡选择。**这一整层是第二循环要新建的主体工作量。**

---

## 1. 本循环明确目标（验收标准）

| # | 目标 | 验收 |
|---|---|---|
| G1 | 完整流程跑通 | 主菜单 → 选关 → 编队 → 入局 → 撤离/获胜 → 结算 → 回主菜单，全程无需重开编辑器 |
| G2 | 局外存档 | 带出的宠物写入磁盘存档，重启游戏后仍在仓库里 |
| G3 | 带入带出 | 局外选中的宠物能种入局内并召唤；局内存活宠物能带回局外 |
| G4 | 主动撤离 | 局内有撤离点/撤离键，可在打 Boss 前主动带宠离场 |
| G5 | 永久损失 + 复活 | 带入宠物阵亡 = 从存档永久删除；通关获得复活道具可救回 |
| G6 | 线性解锁 | 通关 Boss1 关卡后才解锁 Boss2 关卡 |
| G7 | 内容量 | 2 个机制不同的 Boss + 10 只玩法差异化的 Pet 全部可玩 |

---

## 2. 完整流程图

```
┌─────────────┐
│   主菜单     │  开始 / 继续 / 退出
│  MainMenu    │
└──────┬──────┘
       ↓ 开始
┌─────────────┐
│  关卡选择     │  线性解锁：关卡1(Boss1) → [通关后] → 关卡2(Boss2)
│ LevelSelect  │
└──────┬──────┘
       ↓ 选定关卡
┌─────────────┐
│  仓库 + 编队  │  从局外仓库勾选 N 只宠物作为本局出征队
│   Loadout    │  （可选：消耗复活道具救回墓碑里的宠物）
└──────┬──────┘
       ↓ 出征（写 PendingLoadout → OpenLevel 关卡地图）
┌──────────────────────────────────────────┐
│                局内（第一循环）              │
│  关卡 BeginPlay：用 PendingLoadout 种入背包    │
│  ┌────────────────────────────────────┐  │
│  │ 抓宠倒计时 → 围墙+Boss → 宠物自动战斗  │  │   ← 第一循环原样
│  └────────────────────────────────────┘  │
│  三条出口：                                  │
│   ① 主动撤离（撤离点/键，打Boss前可走）        │
│   ② 获胜（打死Boss）                         │
│   ③ 全灭/失败（出战宠物死光）                  │
└──────┬───────────────┬──────────────┬─────┘
       ↓①撤离           ↓②获胜          ↓③失败
┌──────────────────────────────────┐   ┌──────────┐
│        结算：带出 Handoff           │   │ 失败结算  │
│  收集存活宠物 → ExtractionResult     │   │ 仅墓碑回收│
│  获胜额外：复活道具 + 解锁下一关       │   └────┬─────┘
└──────────────────┬───────────────┘        ↓
                   ↓  OpenLevel(主菜单)         ↓
┌──────────────────────────────────────────────┐
│  回主菜单：元层结算 ExtractionResult → 写存档      │
│  · 存活宠物并入仓库  · 阵亡的带入宠物进墓碑          │
│  · 发放复活道具 / 解锁进度                          │
└──────────────────────────────────────────────┘
```

---

## 3. 核心规则：宠物的归属与风险（本案的灵魂）

> 这张表是第二循环最重要的设计契约，所有系统围绕它实现。基于你拍板的决策：**永久损失 + 通关复活道具；新抓阵亡直接没**。

| 宠物来源 | 局内阵亡 | 撤离/获胜时存活 | 全灭/失败 |
|---|---|---|---|
| **带入的老宠物**（局外仓库选入） | **从存档永久删除** → 进「墓碑」，可被复活道具救回 | 写回仓库，刷新成长快照 | 同阵亡：永久删除进墓碑 |
| **本局新抓的宠物** | **直接永久消失**（无墓碑、不可救） | 写入仓库，成为永久宠物 | 直接永久消失 |

要点拆解：
- **撤离/获胜带出的范围** = 本局**所有存活**宠物（带入的老宠 + 新抓的），无论是否在出战位。死掉的不带出。
- **永久损失**：带入宠物阵亡即从存档移除，但保留一份"残响"进**墓碑列表**（`LostPets`），留给复活道具救。
- **复活道具**：仅**获胜（打死 Boss）**才掉落。局外消耗 1 个 → 从墓碑捞回 1 只宠物到仓库（满血、保留等级快照）。**撤离不给复活道具**——这是撤离 vs 获胜的核心差别。
- **新抓宠物的风险纯粹**：进局抓到的宝贝，必须活着带出去才算数；死了就是真的没了。这天然制造"见好就撤还是贪 Boss"的张力。

> 北极星呼应：这套"必然损失 + 仅能救回有限几只"的规则，正是 [GameProposal_Outline](GameProposal_Outline.md) 里「必然告别」主题的可玩化最小切片。撤离=温柔带走，全灭=暴烈失去，复活道具=诅咒借出的唯一礼物。

---

## 4. 新增系统清单（局外元层）

> 全部是 greenfield。按"必须有"→"应该有"→"可后置"分级。

### 4.1 元层持久化 —— `UMFSaveGame` + `UMFMetaSubsystem`【必须】

跨关卡（`OpenLevel`）状态不能放 Actor 上，必须放 `GameInstance` 层。

**`UMFSaveGame : USaveGame`** —— 落盘的存档数据：
```cpp
TArray<FMFPetInstance> MetaPetRoster;   // 局外仓库：所有持有的宠物
TArray<FMFPetInstance> LostPets;        // 墓碑：阵亡的带入宠物，可被复活道具救回
int32   HighestUnlockedLevelIndex = 0;  // 线性解锁进度（0=只解锁关卡1）
TMap<FName, int32> MetaConsumables;     // 局外道具数量，至少含「复活道具」
```
> 复用现有 `FMFPetInstance`（[MFItemTypes.h](../Source/ProjectMF/Inventory/Public/MFItemTypes.h)）——它已含 `InstanceID / AIConfigID / Level / Experience / AttributeSnapshot`，且本身就是为序列化设计的（已有 `SerializeToInstance/RestoreFromInstance` 链路）。**无需新数据结构，直接序列化数组即可。**

**`UMFMetaSubsystem : UGameInstanceSubsystem`** —— 运行时元层大脑，活过 `OpenLevel`：
- 持有 `UMFSaveGame*`，封装 `SaveGameToSlot / LoadGameFromSlot`。
- `PendingLoadout : TArray<FMFPetInstance>` —— 局外编队 → 入局的交接数据。
- `PendingExtraction : FMFExtractionResult` —— 局内 → 局外的交接数据（带出的存活宠物 + 阵亡的带入宠物 + 是否获胜）。
- API：`SetLoadout()` / `ConsumeLoadout()`（局内开局取）/ `SubmitExtraction()`（局内结算写）/ `ReconcileExtraction()`（回主菜单时并入存档并落盘）/ `ReviveFromGrave(InstanceID)`。

### 4.2 主菜单 + 关卡选择【必须】
- 一张轻量 `MainMenu` 关卡（纯 UI Level）：开始 / 继续 / 退出。
- 关卡选择 Widget：读 `HighestUnlockedLevelIndex` 渲染 2 个关卡条目，未解锁置灰。
- 选关 → 进编队界面 → 出征用 `OpenLevel(关卡地图名)`。

### 4.3 仓库 + 编队界面（Loadout）【必须】
- 列出 `MetaPetRoster`，玩家勾选最多 N 只（N = 出征上限，建议先 = `MaxPetSlots`）。
- 勾选结果 → `MetaSubsystem.SetLoadout()`。
- 同屏入口：墓碑列表 + "消耗复活道具救回"按钮。
- **本循环不做任何养成**——这个界面只读宠物的名字/等级/类型，不提供升级/洗练/装备（按你的决策，养成留后续循环）。

### 4.4 局内：开局种入 + 撤离 + 结算交接【必须】
- **种入**：关卡 `BeginPlay`（GameMode 或 Player）调 `MetaSubsystem.ConsumeLoadout()`，把宠物灌进 `InventoryComponent.PetSlots`（需给 `UMFInventoryComponent` 新增一个 `SeedPetsFromInstances(const TArray<FMFPetInstance>&)` 接口；目前只有逐只 `RegisterCaughtPet`）。种入的宠物 `bIsActive=false`，玩家用 1-5 键召唤，逻辑完全复用现有召唤。
- **撤离机制**（新）：场景里放一个**撤离点 Actor**（进入触发）或绑定一个**撤离输入键**。触发后 GameMode 进入新阶段 `Extracting` → 收集存活宠物 → `SubmitExtraction(bVictory=false)` → 结算 UI → `OpenLevel(MainMenu)`。撤离仅在 `Catching` 阶段（Boss 战开始前）可用，Boss 战中封锁。
- **结算交接**：把现有 `M1_HandleVictory` / `M1_HandleDefeat` 扩成统一的 `CollectSurvivorsAndSubmit(bVictory)`：遍历 `InventoryComponent` 区分"存活/阵亡 × 带入/新抓"，写入 `PendingExtraction`，再回主菜单。

### 4.5 关卡数据：从单 Boss 扩到线性两关【必须】
- 现有 `UMFGameLoopConfig` 是**单关卡单 Boss** 的 DataAsset（[MFGameLoopConfig.h](../Source/ProjectMF/GameLoop/Public/MFGameLoopConfig.h)）。
- 第二循环做**两份 `UMFGameLoopConfig` 资产**（`DA_Level1_Boss1` / `DA_Level2_Boss2`），各配各自的 Boss、节奏、围墙。**无需改 C++**——两个关卡地图各自的 `B_MFGameMode` 引用不同 config 即可。
- 线性解锁：关卡选择界面读 `HighestUnlockedLevelIndex` 控制是否可选；通关 Boss1 时 `ReconcileExtraction` 里 `HighestUnlockedLevelIndex = max(1)`。

### 4.6 复活道具系统【应该有】
- 获胜结算发 1 个复活道具进 `MetaConsumables`。
- 编队界面消耗：`ReviveFromGrave(InstanceID)` 把宠物从 `LostPets` 移回 `MetaPetRoster`，扣 1 道具，落盘。

---

## 5. 内容目标：2 Boss + 10 Pet

### 5.1 设计原则
- **复用现有技能套件**做差异化：近战 `Melee`、远程 `Throw / Boulder / BulletCurtain`、移动 `Charge / Jump / GroundSlam`（[plan_gas_expansion]/[plan_ranged_attack] 已建管线）。10 只宠物按"技能原型 × 定位"铺开，保证手感不重样。
- **2 个 Boss 机制必须对比鲜明**，对应两关的难度递进。
- 主题归类挂靠 GameProposal 的四类伙伴（失主之兽 / 神之残片 / 契约回响 / 诅咒同类），为后续叙事留钩子。

### 5.2 10 只 Pet（草案，可调）

| # | 代号 | 主玩法 | 定位 | 差异点 |
|---|---|---|---|---|
| P1 | 史莱姆猫 SlimeCat | 近战 Melee | 入门均衡 | 教学宠，低门槛 |
| P2 | 撞角兽 | 移动 Charge + 近战 | 冲锋切入 | 高速突进打断 |
| P3 | 投石蛙 | 远程 Throw | 标准远程 | 直线投射物 |
| P4 | 落石鼯 | 远程 Boulder | 范围压制 | 砸点 AOE，怕走位 |
| P5 | 弹幕蛾 | 远程 BulletCurtain | 多目标 | 扇形弹幕，清场 |
| P6 | 跳击鼹 | 移动 Jump + 近战 | 越障突袭 | 跳到目标头顶 |
| P7 | 撼地龟 | 移动 GroundSlam | 控场坦克 | 震地范围控制 |
| P8 | 双刀鼬 | 近战连击 | 高 DPS 脆皮 | 攻速快、血薄 |
| P9 | 护卫石像 | 近战 + 高防 | 前排肉盾 | 高防低速，扛伤 |
| P10 | 余烬之灵 | 远程 + 灼烧区域 | 持续伤害 | 命中留燃烧区（接区域子系统） |

> 实现成本：P1–P3 基本是现成 GA 换数据资产；P4–P7 依赖移动技能 P5 批次（[plan_gas_expansion] 里 P5）；P10 依赖区域子系统。**排期时按依赖排，先上不依赖未完成系统的 6–7 只，移动/区域系宠物随其管线落地补齐。**

### 5.3 2 个 Boss（草案）

| 关 | Boss | 机制核心 | 对应威胁 | 玩家应对 |
|---|---|---|---|---|
| 关卡1 | **骨原徘徊者**（失主之兽巨化） | 近战追击 + 周期性 `GroundSlam` 震地 | 走位躲范围 | 前排顶、远程风筝 |
| 关卡2 | **深渊弹幕女王**（神之残片） | `BulletCurtain` 弹幕 + `FallingBoulder` 落石点名 | 多段位移压力 | 撤离决策、复活道具权衡 |

- 关卡2 明显更难，逼玩家用上"撤离 vs 贪 Boss"和复活道具——把第二循环的新机制压力前置到内容里。

---

## 6. 与现有代码的对接点

| 现有资产 | 第二循环动作 |
|---|---|
| `FMFPetInstance`（[MFItemTypes.h](../Source/ProjectMF/Inventory/Public/MFItemTypes.h)） | **直接复用**作存档单元，无需改结构 |
| `UMFInventoryComponent`（[MFInventoryComponent.h](../Source/ProjectMF/Inventory/Public/MFInventoryComponent.h)） | 新增 `SeedPetsFromInstances()` 开局种入；新增"区分新抓 vs 带入"的来源标记（建议给 `FMFPetInstance` 加一个 `bBroughtIn` 瞬态字段或在 Seed 时登记 InstanceID 集合） |
| `AMFGameMode` 的 `M1_*` 区块（[MFGameMode.cpp](../Source/ProjectMF/GameLoop/Private/MFGameMode.cpp)） | 扩 `Victory/Defeat` 为统一 `CollectSurvivorsAndSubmit`；新增 `Extracting` 阶段；BeginPlay 调 `ConsumeLoadout` |
| `EMFGamePhase` | 新增枚举值 `Extracting`（或 `Extracted`） |
| `UMFGameLoopConfig` | 不改 C++，产出两份关卡资产即可 |
| `RestartGame()` 里的 `OpenLevel` | 复用同模式做主菜单 ↔ 关卡切换 |
| —（全新） | `UMFSaveGame`、`UMFMetaSubsystem`、`FMFExtractionResult`、主菜单/选关/编队 3 个 Widget、撤离点 Actor |

**唯一需要确认的来源标记问题**：判定"带入 vs 新抓"需要在 Seed 时记下哪些 `InstanceID` 是带入的（之后阵亡时据此决定进墓碑还是直接消失）。最轻量做法 = `MetaSubsystem` 持有一个 `BroughtInIDs : TSet<FGuid>`，开局 Seed 时填入，结算时查。

---

## 7. 实现优先级（依赖顺序）

```
① UMFSaveGame + UMFMetaSubsystem（存档读写 + Pending* 交接）   ← 一切的地基
② InventoryComponent.SeedPetsFromInstances + 来源标记          ← 局内能被种入
③ GameMode：BeginPlay 种入 + 统一结算 Submit + Extracting 阶段  ← 局内能交回
④ 主菜单 + 选关 + 编队三件套 UI + OpenLevel 串联                ← 流程闭合（G1 达成）
⑤ 撤离点 Actor / 撤离键                                        ← G4
⑥ 永久损失 + 墓碑 + 复活道具                                   ← G5
⑦ 第二份关卡资产 + 线性解锁                                    ← G6
⑧ 10 Pet 数据资产 + 2 Boss 配置（随技能/区域管线补齐）          ← G7 内容收尾
```

> 里程碑切法：①②③④ 做完就是**最小可玩闭环**（能进出局、能存档、能带宠），先打通它再叠⑤⑥⑦⑧。

---

## 8. 待决问题清单

- **Q1 出征上限 N**：编队最多带几只？建议先 = `MaxPetSlots`；若想强化取舍可设更小（如 3）。
- **Q2 撤离点形态**：固定撤离点 Actor（要走过去，有空间代价）还是即按即走的撤离键？建议**撤离点 Actor**，更贴"撤离"语义也更有张力。
- **Q3 复活道具掉落量**：每次获胜固定 1 个？还是 Boss2 给更多？建议先固定 1。
- **Q4 墓碑容量**：墓碑无限存还是有上限（满了最老的彻底消失）？建议本循环先无限，后续再加压。
- **Q5 存档槽**：单槽覆盖即可（本循环不做多存档）。
- **Q6 新抓宠物的等级**：野外抓到默认 Lv.1（现状），本循环不做局内升级，撤出后即 Lv.1 入库。确认无异议。

---

## 附：一句话总结

第二循环 = **给第一循环装上"局外的进出口"**：一层 `SaveGame + GameInstance 子系统` 承载仓库/墓碑/解锁进度，主菜单/选关/编队三件套把多次局内串成旅程，撤离与获胜两条带出路径配合"永久损失 + 复活道具"的风险规则，最后用 2 Boss + 10 Pet 把内容铺满，交付一个能对外演示的可玩版本。
