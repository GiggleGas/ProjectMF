# 第二循环策划案 ——「地牢探索 + 局外元层 + 带入带出」

> 状态：草案 v2（2026-06-26，局内从竞技场重做为地牢探索）
> 定位：局内由「竞技场单关」改为**地牢探索**（3 层 × 每层若干房间 × 房间刷可捕捉小怪 × 第 3 层 Boss）；局外**元层（存档 / 主菜单 / 编队）+ 带入带出 + 永久损失规则保持不变**。
> 一句话目标：玩家能从**主菜单 → 编队带宠 → 进入地牢 → 逐层探索/战斗/捕捉小怪 → 每层楼梯处选「撤离带出」或「下一层」→ 第 3 层杀 Boss 通关 → 把存活宠物带出局外 → 存档 → 下次再带入**，并填充 **10 只可捕捉小怪 + 1 个 Boss** 的内容量。

---

## 0. 与第一循环的关系 + v1→v2 变更

- **第一循环（已落地）** = 单关卡竞技场战斗闭环：抓宠倒计时 → 生成围墙 + Boss → 宠物自动战斗 → 胜利 / 失败 / 全灭。由 `AMFGameMode` 的 `M1_*` 区块编排（[MFGameMode.cpp](../Source/ProjectMF/GameLoop/Private/MFGameMode.cpp)）。
- **第二循环（本案 v2）** = 两处改动：
  1. **局内：竞技场 → 地牢探索。** 原「Catching 限时阶段 → Boss 阶段」的两段式被**地牢逐层探索**替代。抓宠从"限时阶段集中抓"变成"探索房间随遇随抓"。Boss 从"开局即生成"变成"第 3 层终点"。
  2. **局外：新增元层（不变于 v1 设计）。** 存档 / 主菜单 / 编队 + 撤离带出 + 永久损失 + 复活道具，把多次地牢运行串成一条玩家旅程。

> **v2 关键收益**：第一循环已实现的**死亡回传链 + 阵营系统 + 全灭判负**（一阶段验证循环三阻断已闭合，2026-06-25）原样复用——地牢里宠物战斗、阵亡进墓碑、全灭判负全部沿用，无需重写。地牢只是把"战斗发生的容器"从一个围墙竞技场换成多房间多层结构。

> **关键现状**：当前**无任何持久层**。宠物数据 `FMFPetInstance` 全挂玩家 `UMFInventoryComponent.PetSlots`（[MFInventoryComponent.h](../Source/ProjectMF/Inventory/Public/MFInventoryComponent.h)），`OpenLevel` 重载即销毁。没有 `GameInstance` 子类 / `SaveGame` / 主菜单。元层 + 地牢生成都是本循环要新建的主体工作量。

---

## 1. 本循环目标（验收标准）

| # | 目标 | 验收 |
|---|---|---|
| G1 | 完整流程跑通 | 主菜单 → 编队 → 进地牢 → 逐层探索 → 撤离/通关 → 结算 → 回主菜单，全程无需重开编辑器 |
| G2 | 局外存档 | 带出的宠物写入磁盘存档，重启游戏后仍在仓库里 |
| G3 | 带入带出 | 局外选中的宠物能种入地牢并召唤；局内存活宠物能带回局外 |
| G4 | 地牢生成 | 每次进入地牢，每层由预制房间模板**随机拼接**而成，3 层结构稳定可玩 |
| G5 | 房间刷怪 + 可捕捉 | 房间内刷出小怪，宠物可与之战斗，玩家可用现有捕捉流程抓捕 |
| G6 | 楼梯决策点 | 每层终点楼梯处可选「撤离带出」或「下一层」；撤离即带宠回局外 |
| G7 | 通关 + 永久损失 + 复活 | 第 3 层杀 Boss = 通关（发复活道具）；带入宠物阵亡 = 永久损失进墓碑，复活道具可救回 |
| G8 | 内容量 | 10 只玩法差异化的可捕捉小怪（按层分布）+ 1 个 Boss 全部可玩 |

> v1 提案里的「线性解锁两关」（旧 G6）按用户决策**降级**：本循环只做**单地牢**。多地牢线性解锁留作未来"地牢 2"扩展。

---

## 2. 完整流程图

```
┌─────────────┐
│   主菜单     │  开始 / 继续 / 退出
│  MainMenu    │
└──────┬──────┘
       ↓ 开始
┌─────────────┐
│  仓库 + 编队  │  从局外仓库勾选 N 只宠物作为本次出征队
│   Loadout    │  （可选：消耗复活道具救回墓碑里的宠物）
└──────┬──────┘
       ↓ 进入地牢（写 PendingLoadout → OpenLevel 地牢地图）
┌──────────────────────────────────────────────────────────┐
│                     地牢局内（第二循环核心）                  │
│  地图 BeginPlay：ConsumeLoadout 种入背包；生成第 1 层          │
│                                                            │
│  ┌─ 第 1 层 ──────────────────────────────────────┐        │
│  │ 预制房间随机拼接 → 房间刷可捕捉小怪              │        │
│  │ 玩家召唤宠物 → 探索房间 → 战斗 / 捕捉 → 到达楼梯  │        │
│  └────────────────────┬────────────────────────────┘        │
│            楼梯决策点：[撤离带出] 或 [下一层]                 │
│                       ↓ 下层                                │
│  ┌─ 第 2 层（更难，更好的小怪）────────────────────┐        │
│  │ 同上结构                                        │        │
│  └────────────────────┬────────────────────────────┘        │
│            楼梯决策点：[撤离带出] 或 [下一层]                 │
│                       ↓ 下层                                │
│  ┌─ 第 3 层（Boss 房）──────────────────────────────┐        │
│  │ Boss 战 → 杀死 Boss = 通关                        │        │
│  └──────────────────────────────────────────────────┘        │
│                                                            │
│  三条出口：                                                  │
│   ① 主动撤离（任意层楼梯处）                                  │
│   ② 通关（杀第 3 层 Boss）                                   │
│   ③ 全灭/失败（出战宠物死光，复用第一循环全灭判负）            │
└──────┬───────────────┬──────────────┬─────────────────────┘
       ↓①撤离           ↓②通关          ↓③失败
┌──────────────────────────────────┐   ┌──────────┐
│        结算：带出 Handoff           │   │ 失败结算  │
│  收集存活宠物 → ExtractionResult     │   │ 仅墓碑回收│
│  通关额外：复活道具 + 完成奖励        │   └────┬─────┘
└──────────────────┬───────────────┘        ↓
                   ↓  OpenLevel(主菜单)         ↓
┌──────────────────────────────────────────────┐
│  回主菜单：ReconcileExtraction → 写存档          │
│  · 存活宠物并入仓库  · 阵亡的带入宠进墓碑          │
│  · 发放复活道具                                  │
└──────────────────────────────────────────────┘
```

---

## 3. 核心规则：宠物的归属与风险（本案的灵魂，沿用 v1 不变）

> 这张表是第二循环最重要的设计契约，所有系统围绕它实现。基于用户拍板：**永久损失 + 通关复活道具；新抓阵亡直接没**。地牢化后唯一变化：「撤离」从竞技场撤离点变成**楼梯处的撤离选项**，规则本身不变。

| 宠物来源 | 局内阵亡 | 撤离/通关时存活 | 全灭/失败 |
|---|---|---|---|
| **带入的老宠物**（局外仓库选入） | **从存档永久删除** → 进「墓碑」，可被复活道具救回 | 写回仓库，刷新成长快照 | 同阵亡：永久删除进墓碑 |
| **本局新抓的小怪** | **直接永久消失**（无墓碑、不可救） | 写入仓库，成为永久宠物 | 直接永久消失 |

要点拆解：
- **带出范围** = 撤离/通关那一刻本局**所有存活**宠物（带入的老宠 + 新抓的小怪），无论是否在出战位。死掉的不带出。
- **永久损失**：带入宠物阵亡即从存档移除，保留一份"残响"进**墓碑列表**（`LostPets`），留给复活道具救。
- **复活道具**：仅**通关（杀第 3 层 Boss）**才掉落。撤离**不给**——这是「见好就撤」与「贪到底通关」的核心差别。局外消耗 1 个 → 从墓碑捞回 1 只宠物（满血、保留等级快照）。
- **新抓小怪的风险纯粹**：地牢里抓到的宝贝，必须活着带出去才算数；死了或全灭就是真的没了。这天然制造"这层见好就撤，还是带着新宝贝继续深入"的张力——**地牢分层结构把这个决策点做成了显式的楼梯选择**。

> 北极星呼应：「必然损失 + 仅能救回有限几只」正是「必然告别」主题的可玩化最小切片。撤离=温柔带走，全灭=暴烈失去，复活道具=诅咒借出的唯一礼物。地牢的逐层加深 = 每下一层都在加注，把这份告别的重量层层垒高。

---

## 4. 地牢局内系统（v2 新增主体）

> 这一整层替代第一循环 `M1_*` 区块的"竞技场阶段机"。建议在 `AMFGameMode` 新开一个 **`M2_*` 地牢区块**（或独立 `UMFDungeonSubsystem`），与 `M1_*` 并存或整体替换。复用其中的：死亡订阅、全灭判负、Boss 生成、阵营设置。

### 4.1 地牢生成器 —— 预制房间模板随机拼接【必须】

**形态（用户拍板）**：每层由**手工预制的房间模板**在运行时**随机选取并拼接**而成。

- **房间模板（Room Prefab）**：每个房间是一个可复用单元（建议 **Packed Level Actor / 子关卡**，或带固定模块尺寸的房间 BP）。每个模板携带：
  - **连接口（Door Socket）**：入口 / 出口的对接锚点（Scene Component 或标记 Actor），生成器据此把房间首尾相接。
  - **刷怪点标记**：房间内若干 `ManualSpawnPoints` 用的标记 Actor（见 4.3）。
  - **房间类型 Tag**：普通房 / 入口房 / 楼梯房 / Boss 房 / （可选）支线房。
- **每层布局（建议先做线性链）**：`入口房 → 普通房 ×K → 楼梯房`，K = 该层房间数（每层几个，按层递增）。第 3 层末端换成 **Boss 房**。线性链最易控、契合"每层几个房间"；后续可加"支线死胡同房"塞额外捕捉目标。
- **生成算法（每层）**：
  1. 放入口房（固定类型）。
  2. 从普通房模板池随机抽 K 个，按连接口依次对接（统一模块尺寸 + 网格对齐，避免重叠）。
  3. 末端接楼梯房（第 3 层接 Boss 房）。
  4. 对每个房间的刷怪点执行刷怪（4.3），怪池随楼层加难。
- **实现载体**：`UMFDungeonGenerator`（建议 `UWorldSubsystem` 或 GameMode 持有的组件）。提供 `GenerateFloor(int32 FloorIndex)` → 返回房间实例列表 + 楼梯/Boss 位置。下层时销毁上层、生成新层（或用关卡流送加载/卸载子关卡，性能更好但接入成本高——**v2.0 先用 SpawnActor 拼装，关卡流送留优化**）。

> **简化降级位**：若拼接对齐调试成本高，可先退到「固定布局，只随机刷怪」（用户备选项）验证整条流程，再升级到随机拼接。

### 4.2 地牢运行状态机 —— 替代 M1 阶段机【必须】

新增地牢专属阶段枚举（或扩 `EMFGamePhase`）：

```cpp
enum class EMFDungeonPhase : uint8 { Idle, Exploring, AtStairs, BossFight, Victory, Defeat, Extracting };
```

GameMode（`M2_*` 区块）编排：
- **BeginPlay**：`MetaSubsystem.ConsumeLoadout()` 种入背包 → `GenerateFloor(1)` → 刷怪 → `Exploring`。订阅出战宠死亡（复用 v1 的 `HandlePetDied` 链 + 全灭判负）。
- **到达楼梯** → `AtStairs`：弹决策 UI（撤离 / 下层）。
  - **撤离** → `CollectSurvivorsAndSubmit(bVictory=false)` → 结算 → `OpenLevel(MainMenu)`。
  - **下层** → `CurrentFloor++` → 销毁当前层 → `GenerateFloor(CurrentFloor)` → 刷怪 → 回 `Exploring`；若 `CurrentFloor == 3` → 生成 Boss 房 → `BossFight`。
- **Boss 死亡** → `Victory`：`CollectSurvivorsAndSubmit(bVictory=true)`（带复活道具 + 完成奖励）→ `OpenLevel(MainMenu)`。
- **出战宠全灭** → `Defeat`：**直接复用第一循环刚实现的全灭判负**（`GetActivePetActors()` 归零 → `HandleDefeat`）→ `CollectSurvivorsAndSubmit(bVictory=false)` → `OpenLevel(MainMenu)`。
- 运行时状态：`CurrentFloor (1-3)`、当前层房间列表、楼梯 Actor 引用。

### 4.3 房间刷怪 —— 复用 `AMFSpawnAIManager`【必须】

- **直接复用** [AMFSpawnAIManager](../Source/ProjectMF/AI/Public/MFSpawnAIManager.h)：它支持 `ManualSpawnPoints` 选点规则（引用场景标记 Actor）+ 按 `UMFPetConfig` 决定生成种类。
- **接法**：每个房间模板内放刷怪点标记 Actor；生成器在房间实例化后，给该房间挂一个 `AMFSpawnAIManager`（或生成器直接驱动刷怪），`SpawnEntries` 引用**该楼层怪池**对应的 `UMFPetConfig`。
- **怪 = 可捕捉**：小怪用 `AMFPetBase`（已实现 `IMFCatchable`，[MFPetBase.h](../Source/ProjectMF/Catching/Public/MFPetBase.h)），出生阵营 `Team.Enemy`。玩家召唤宠（`Team.Player`）与之异阵营 → 自动战斗（StateTree + 威胁/雷达，全复用）。
- **怪池按层加难**：第 1 层放低难度小怪（教学型），第 2 层中等，第 3 层精英 + Boss。怪池即 6.2 的 10 只可捕捉宠物按层分布。

### 4.4 楼梯 / 撤离决策点【必须】

- **楼梯房**末端放一个 **楼梯 Actor**（进入触发区 / 交互键）。触发后暂停探索、弹出决策 UI：**「撤离带出」** 或 **「下一层」**。
- **撤离** = v1 撤离语义并入楼梯：收集存活宠 → `SubmitExtraction(bVictory=false)` → 结算 → 回主菜单。**撤离不给复活道具**。
- **下层** = 单向前进，不能返回上层（怪与已抓情况不回溯）。
- 第 3 层无楼梯房，直接是 Boss 房（通关是唯一"带奖励出局"的方式）。

### 4.5 第 3 层 Boss 房【必须】

- Boss 房 = 专门设计的固定模板（不参与随机拼接，作为第 3 层终点）。
- **复用 v1 的 Boss 生成**（`M1_SpawnBoss`：SpawnActor + Controller + RunStateTree + 阵营 `BossTeamTags`）。
- 杀 Boss → `Victory`。Boss 战中沿用 v1：全灭判负仍生效（Boss 战里出战宠死光 = 失败）。

### 4.6 捕捉 —— 复用 `GA_CatchPet`，随遇随抓【必须】

- 探索中遇到小怪，用**现有三阶段 QTE 捕捉**（`GA_CatchPet` + `AT_WaitPetTarget` + `AT_MoveBall`，已实现）。
- 抓成功 → `RegisterCaughtPet` 入 `PetSlots`，标记为**本局新抓**（非带入，阵亡即永久消失）。
- 不再有"限时抓宠阶段"——抓宠贯穿整个探索过程，节奏由玩家自己把控（也强化了"贪抓 vs 保命下层"的张力）。

---

## 5. 局外元层系统（沿用 v1 设计，基本不变）

> 全部 greenfield。地牢化对元层几乎无影响——元层只关心"带哪些宠进去 / 带哪些宠出来 / 存档"，不关心局内是竞技场还是地牢。

### 5.1 元层持久化 —— `UMFSaveGame` + `UMFMetaSubsystem`【必须】

跨 `OpenLevel` 状态必须放 `GameInstance` 层。

**`UMFSaveGame : USaveGame`** —— 落盘存档：
```cpp
// 元层持久数据（跨多趟运行）
TArray<FMFPetInstance> MetaPetRoster;   // 局外仓库：所有持有的宠物
TArray<FMFPetInstance> LostPets;        // 墓碑：阵亡的带入宠物，可被复活道具救回
TMap<FName, int32>     MetaConsumables; // 局外道具数量，至少含「复活道具」
// 局内运行快照（中途续局；run 结束后清空）
bool                   bHasActiveRun;   // 是否有未完成的一趟运行可续
FMFRunSnapshot         ActiveRun;       // 当前层/房间/seed/局内背包(含新抓)/带入宠存活态
// 注：v1 的 HighestUnlockedLevelIndex 本循环单地牢不需要；留作未来多地牢扩展
```
> **复用现有 `FMFPetInstance`**（[MFItemTypes.h](../Source/ProjectMF/Inventory/Public/MFItemTypes.h)）：已含 `InstanceID / AIConfigID / Level / Experience / AttributeSnapshot`，本就为序列化设计。
> **⚠️ 前置（additive）**：UE 标准存档序列化只写带 `SaveGame` 修饰符的 `UPROPERTY`。`FMFPetInstance` 现有字段**未标 `SaveGame`**，需逐个补 `UPROPERTY(..., SaveGame)`，否则存档存出空数据。纯增量改动，不动逻辑，与地牢无关。
> **⚠️ `FMFRunSnapshot` 内容强依赖地牢状态模型**（布局 / 当前房间 / seed 重建）——属同事侧地牢的数据。**局外先搭存档基础设施 + 落盘时机 + 元层数据；快照字段等地牢到位再填。**

**`UMFMetaSubsystem : UGameInstanceSubsystem`** —— 活过 `OpenLevel` 的元层大脑：
- 持 `UMFSaveGame*`，封装 `SaveGameToSlot / LoadGameFromSlot`；boot 时读档，无档则**建新档（预置 1-2 只初始宠进 roster）**。
- `PendingLoadout : TArray<FMFPetInstance>` —— 编队 → 入局交接。
- `PendingRunSeed : int32` —— 出征时生成的随机种子 → 地牢生成器（决定布局，可复现）。
- `PendingExtraction : FMFExtractionResult` —— 局内 → 局外交接（存活宠 + 阵亡的带入宠 + 新抓宠 + 是否通关）。
- `BroughtInIDs : TSet<FGuid>` —— 开局种入时登记，结算时据此区分"带入 vs 新抓"。
- API：`SetLoadout()` / `ConsumeLoadout()` / `SubmitExtraction()` / `ReconcileExtraction()`（回主菜单并入存档落盘）/ `ReviveFromGrave(InstanceID)` / `SaveNow()`（统一落盘入口）/ `SaveRunSnapshot()` / `ClearRunSnapshot()`。

**落盘时机（用户拍板）**：关键点位落盘，不每帧存——
- **每个房间清空**（检查点）→ `SaveRunSnapshot()`
- **主动点击保存** → `SaveNow()`
- **宠物阵亡瞬间**立即写盘 → 锁定死亡，**退出续局也改不了死亡，保住永久损失**（解续局 vs 永久损失冲突）
- **结算回主菜单** → `ReconcileExtraction()` 并入元层数据 + `ClearRunSnapshot()` 落盘
- 房间清空 / 阵亡的触发点在地牢侧（🔌 调 `SaveRunSnapshot` / `SaveNow`）；局外提供这些 API。

### 5.2 主菜单（选关简化为单地牢入口）【必须】
- 一张轻量 `MainMenu` 关卡（纯 UI Level）：开始 / 继续 / 退出。
- **开始（新游戏）**：无存档则建新档，**预置 1-2 只初始宠进 roster**（避免空仓库无宝可带）→ 进编队界面。
- **继续**：boot 时 `LoadGameFromSlot`。若 `bHasActiveRun` → **续上未完成的那趟地牢**（用快照恢复 `OpenLevel(地牢图)`，🔌 依赖地牢侧从快照重建）；否则进主菜单/编队，用已有 roster。
- 本循环单地牢：编队 → 出征 `OpenLevel(地牢地图)`。多地牢选择界面**留作未来**。

### 5.3 仓库 + 编队界面（Loadout）【必须】
- 列出 `MetaPetRoster`，玩家勾选最多 N 只（N = 出征上限，建议先 = `MaxPetSlots`）。
- 勾选结果 → `MetaSubsystem.SetLoadout()`。
- 同屏入口：墓碑列表 + "消耗复活道具救回"按钮。
- **本循环不做任何养成**——只读名字/等级/类型，不提供升级/洗练/装备（养成留后续循环）。

### 5.4 局内：启动编排 + 开局种入 + 撤离/结算交接【必须】
- **出征**：编队界面"出征" → 写 `PendingLoadout` + **生成 `PendingRunSeed`**（在出征点生成，跟 loadout 一起过去，可显示/可复现）→ `OpenLevel(地牢图)`。
- **启动编排**（归属：地牢图 GameMode `BeginPlay`，或一个 `InitRun()` 步骤）按序：① 🔌 用 `PendingRunSeed` 调 `UMFDungeonGenerator.GenerateFloor`（同事侧，先 stub）→ ② `ConsumeLoadout()` → `SeedPetsFromInstances()` 灌进 `InventoryComponent.PetSlots`（新增 API；现仅有逐只 `RegisterCaughtPet`），种入宠 `bIsActive=false`，登记 `BroughtInIDs` → ③ 交棒局内 gameplay。
- **撤离 / 结算交接**：把 v1 的 `M1_HandleVictory` / `M1_HandleDefeat` 扩成统一的 `CollectSurvivorsAndSubmit(bVictory)`（🔌 地牢侧）：遍历 `InventoryComponent` 区分"存活/阵亡 × 带入/新抓"，写入 `PendingExtraction`，`ClearRunSnapshot()`，再 `OpenLevel(MainMenu)`。撤离与通关都走它，差别只在 `bVictory` 与复活道具发放。
- **局外侧验证**：上面 ①③ 与 `CollectSurvivorsAndSubmit` 在地牢到位前用占位图 + debug 伪造 `PendingExtraction` 替代，局外可独立测通。

### 5.5 复活道具系统【应该有】
- 通关结算发 1 个复活道具进 `MetaConsumables`。
- 编队界面消耗：`ReviveFromGrave(InstanceID)` 把宠物从 `LostPets` 移回 `MetaPetRoster`，扣 1 道具，落盘。

---

## 6. 内容目标：10 只可捕捉小怪 + 1 Boss

### 6.1 设计原则
- **10 只小怪 = 地牢怪池 = 可带出的宠物**：地牢房间里刷的小怪，就是这 10 只差异化宠物。抓到带出 = 入仓库。按楼层分布难度。
- **复用现有技能套件**做差异化：近战 `Melee`、远程 `Throw / Boulder / BulletCurtain`、移动 `Charge / Jump / GroundSlam`（管线已建，[plan_gas_expansion]/[plan_ranged_attack]）。
- **本循环 1 个 Boss**（第 3 层）；v1 的第 2 Boss 随未来"地牢 2"再做。
- 主题归类挂靠四类伙伴（失主之兽 / 神之残片 / 契约回响 / 诅咒同类），为叙事留钩子。

### 6.2 10 只小怪（草案，按层分布）

| 层 | # | 代号 | 主玩法 | 定位 | 差异点 |
|---|---|---|---|---|---|
| 1 | P1 | 史莱姆猫 SlimeCat | 近战 Melee | 入门均衡 | 教学怪，低门槛 |
| 1 | P2 | 投石蛙 | 远程 Throw | 标准远程 | 直线投射物 |
| 1 | P3 | 撞角兽 | 移动 Charge + 近战 | 冲锋切入 | 高速突进打断 |
| 2 | P4 | 落石鼯 | 远程 Boulder | 范围压制 | 砸点 AOE，怕走位 |
| 2 | P5 | 跳击鼹 | 移动 Jump + 近战 | 越障突袭 | 跳到目标头顶 |
| 2 | P6 | 双刀鼬 | 近战连击 | 高 DPS 脆皮 | 攻速快、血薄 |
| 2 | P7 | 护卫石像 | 近战 + 高防 | 前排肉盾 | 高防低速，扛伤 |
| 3 | P8 | 弹幕蛾 | 远程 BulletCurtain | 多目标 | 扇形弹幕，清场（精英） |
| 3 | P9 | 撼地龟 | 移动 GroundSlam | 控场坦克 | 震地范围控制（精英） |
| 3 | P10 | 余烬之灵 | 远程 + 灼烧区域 | 持续伤害 | 命中留燃烧区（精英，接区域子系统） |

> 实现成本：P1–P3 基本现成 GA 换数据资产；P4–P7 依赖移动技能管线；P10 依赖区域子系统。**排期按依赖排，先上不依赖未完成系统的 6–7 只，移动/区域系怪随其管线落地补齐。**

### 6.3 Boss（第 3 层，本循环 1 个）

| 层 | Boss | 机制核心 | 玩家应对 |
|---|---|---|---|
| 第 3 层 | **骨原徘徊者**（失主之兽巨化） | 近战追击 + 周期性 `GroundSlam` 震地 + （可选）召唤小怪 | 前排顶、远程风筝、走位躲范围 |

> 第 2 Boss「深渊弹幕女王」（`BulletCurtain` + `FallingBoulder` 落石点名）留作**未来地牢 2** 的终点 Boss。

---

## 7. 与现有代码的对接点

| 现有资产 | 第二循环动作 |
|---|---|
| `FMFPetInstance`（[MFItemTypes.h](../Source/ProjectMF/Inventory/Public/MFItemTypes.h)） | **直接复用**作存档单元，无需改结构 |
| `UMFInventoryComponent`（[MFInventoryComponent.h](../Source/ProjectMF/Inventory/Public/MFInventoryComponent.h)） | 新增 `SeedPetsFromInstances()` 开局种入；来源标记走 `MetaSubsystem.BroughtInIDs : TSet<FGuid>` |
| 死亡回传 + 全灭判负（v1 已实现） | **原样复用**：地牢里宠物阵亡进墓碑、出战宠全灭 → `HandleDefeat`，无需重写 |
| `AMFSpawnAIManager`（[MFSpawnAIManager.h](../Source/ProjectMF/AI/Public/MFSpawnAIManager.h)） | **复用**作房间刷怪：`ManualSpawnPoints` 规则指向房间内刷怪点标记 |
| `AMFPetBase` + `IMFCatchable`（[MFPetBase.h](../Source/ProjectMF/Catching/Public/MFPetBase.h)） | 小怪即用它，捕捉全复用 `GA_CatchPet` |
| `AMFGameMode` 的 `M1_*` 区块（[MFGameMode.cpp](../Source/ProjectMF/GameLoop/Private/MFGameMode.cpp)） | 新开 `M2_*` 地牢区块替代竞技场阶段机；复用 `M1_SpawnBoss` / 死亡订阅 / `CollectSurvivorsAndSubmit` |
| `UMFGameLoopConfig`（[MFGameLoopConfig.h](../Source/ProjectMF/GameLoop/Public/MFGameLoopConfig.h)） | 扩展/新建 `UMFDungeonConfig`：层数=3、每层房间数、各层怪池、Boss 配置、房间模板池 |
| —（全新） | `UMFSaveGame`、`UMFMetaSubsystem`、`FMFExtractionResult`、`UMFDungeonGenerator`、房间模板资产、楼梯 Actor、主菜单/编队 2 个 Widget、楼梯决策 UI |

---

## 8. 实现优先级（依赖顺序）

```
A. 地牢局内（先把"能逐层打"跑起来，可在无元层时用调试入口验证）
  ① UMFDungeonGenerator：预制房间随机拼接 + 楼梯/Boss 位（先线性链）   ← 地牢骨架
  ② 房间刷怪：房间模板 + 刷怪点 + AMFSpawnAIManager 接入               ← G5
  ③ M2 运行状态机：逐层 Exploring/AtStairs/下层/BossFight + 复用全灭判负 ← G4/G6 局内闭环
  ④ 楼梯决策点 Actor + 决策 UI（撤离/下层）                           ← G6

B. 局外元层（把地牢串成旅程）
  ⑤ UMFSaveGame + UMFMetaSubsystem（存档读写 + Pending* 交接）        ← 持久层地基
  ⑥ InventoryComponent.SeedPetsFromInstances + BroughtInIDs 来源标记   ← G3 种入
  ⑦ GameMode：BeginPlay 种入 + 统一结算 CollectSurvivorsAndSubmit      ← G3 带出
  ⑧ 主菜单 + 编队 UI + OpenLevel 串联                                 ← G1 流程闭合
  ⑨ 永久损失 + 墓碑 + 复活道具                                        ← G7

C. 内容收尾
  ⑩ 10 只可捕捉小怪数据资产（按层分布）+ 1 Boss + 房间模板池           ← G8
```

> **里程碑切法**：
> - **M-地牢**：①②③④ 做完 = 能进一张地牢图、逐层打、楼梯下层、第 3 层杀 Boss、全灭判负（用调试入口直接进，先不接元层）。
> - **M-元层**：⑤⑥⑦⑧⑨ 做完 = 完整可玩闭环（存档 + 带入带出 + 撤离 + 复活）。
> - **M-内容**：⑩ 把怪和房间铺满。
>
> 建议先 A 后 B：地牢局内可独立于元层验证（调试键直接 OpenLevel 进地牢图），元层再把进出口接上。

---

## 9. 待决问题清单

- **Q1 每层房间数**：每层几个房间？建议第 1 层 3 间、第 2 层 4 间递增（含楼梯房）；可做成 `UMFDungeonConfig` 每层可配。
- **Q2 房间是否必须清怪才能过**：到楼梯是否要求清空房间小怪？建议**不强制**（怪会 aggro，但玩家可绕；房间是捕捉机会而非强制清场），保留"贪抓 vs 速通"的选择。
- **Q3 楼梯解锁条件**：楼梯一进层就可达，还是要走到末端房间？建议**走到末端楼梯房**（线性链天然如此）。
- **Q4 下层是否回血/补给**：下层时出战宠是否回血？建议**不回血**（强化"带伤深入"的损失张力）；或楼梯房放可选补给点留作平衡旋钮。
- **Q5 地牢生成载体**：SpawnActor 拼装 vs 关卡流送（Level Streaming）？建议 **v2.0 先 SpawnActor 拼装**（实现快），性能/内存吃紧再升级关卡流送。
- **Q6 出征上限 N**：编队最多带几只？建议先 = `MaxPetSlots`；想强化取舍可设更小（如 3）。
- **Q7 复活道具掉落量**：每次通关固定 1 个？建议先固定 1。
- **Q8 墓碑容量**：建议本循环先无限，后续再加压（满了最老的彻底消失）。
- **Q9 存档槽**：单槽覆盖即可（本循环不做多存档）。
- **Q10 新抓小怪等级**：地牢抓到默认 Lv.1（现状），本循环不做局内升级，带出即 Lv.1 入库。

### 已拍板决策（2026-06-26）
- **D1 空仓库新游戏**：新存档**预置 1-2 只初始宠**进 roster，保证第一趟有宝可带。
- **D2 落盘时机**：关键点位落盘 —— **每个房间清空（检查点）+ 主动点击保存 + 阵亡瞬间立即存 + 结算**。⇒ 引入**局内运行快照（中途续局）**；快照内容依赖地牢状态模型（同事侧）。
- **D3 续局 vs 永久损失冲突**：靠 **D2 的"阵亡即时存"** 解决——宠物一死立即写盘锁定，退出续局也改不了死亡，保住永久损失灵魂。
- **D4 随机种子**：出征点生成 `PendingRunSeed` 交给地牢生成器（可复现）；**本循环 seed 入快照**（续局要靠它重建同一座地牢），run 结束随快照清空。

---

## 附：一句话总结

第二循环 v2 = **把第一循环的战斗内核装进一座 3 层地牢**：预制房间随机拼接成层、房间刷可捕捉小怪、每层楼梯处做"撤离带出 vs 继续深入"的决策、第 3 层杀 Boss 通关；外面再套一层 `SaveGame + GameInstance 子系统` 承载仓库/墓碑/复活进度，主菜单 + 编队把多次地牢运行串成旅程，配合"永久损失 + 复活道具"的风险规则——交付一个能对外演示、有进有出、有得有失的可玩版本。
