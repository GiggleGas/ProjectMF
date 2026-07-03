# 任务系统（Quest）设计方案

> 2026-07-03 ｜ Loop2 MVP 任务 A1.7（插在 A1.5 背包之后、A2 目标点之前）｜ 状态：方案待评审，未动代码
> 范围：任务定义配置（唯一 ID）+ 任务列表 UI + 进度追踪 + 完成事件。三种任务类型：收集 N 物资 / 击杀 N 生物 / 收集 N 宠物。
> 定位：**统一的目标追踪层**。目标点（A2）是任务的第四种"到点交互"形态，落地时上报本系统；撤离点 / 打 Boss 等消费者只订阅本系统的"全部完成"事件。

---

## 1. 层次定位（Quest ⊃ Objective）

```
                 UMFQuestSubsystem  ← 唯一的"本图目标是否完成"权威
                 追踪进度 / 判完成 / 广播事件 / 驱动任务列表 UI
        ┌──────────────┬──────────────┬──────────────┬───────────────┐
   CollectItem     KillCreature    CollectPet     InteractPoint(A2)
   背包事件         击杀事件         抓宠事件        目标点Marker上报
   (本任务做)       (本任务做)       (本任务做)       (A2 落地时接)
                              │
                     OnAllQuestsCompleted → 撤离点 / Boss入口 / 献祭（消费者订阅）
```

**与 A2 的关系（架构收敛）**：原 A2 计划的独立 `UMFObjectiveSubsystem` 计数职责**并入本系统**——目标点 Marker 完成时调 `QuestSubsystem::NotifyInteractPoint(QuestID)`，不再另起一个计数子系统。A2 方案相应简化为"Marker（世界交互）+ 上报 Quest"。这样"目标完成"只有一个权威，撤离/Boss 只订阅一处。

## 2. 现状对接点（已核实）

| 任务类型 | 进度来源 | 现状 |
|---|---|---|
| 收集 N 物资 | `UMFInventoryComponent::GetResourceCount(ItemID)` + `OnInventoryChanged` | ✅ 已有，订阅重算即可 |
| 击杀 N 生物 | 生物死亡广播 | ✗ **需新增** `OnCreatureKilled` 委托（`AMFAICharacter::HandleDeath` 里发） |
| 收集 N 宠物 | 抓宠成功广播 | ✗ **需新增** `OnPetCaught` 委托（`RegisterCaughtPet` 成功末尾发；现有 `OnPetRosterChanged` 召回/死亡也触发，不精确） |

新增两个委托是纯增量、零改动现有逻辑。

## 3. 配置方案（核心）

### 3.1 载体：DataTable，RowName = 唯一任务 ID

沿用项目 `DT_AIRegistry` 的 DataTable 模式。建 **`DT_QuestLibrary`**（行类型 `FMFQuestDef : FTableRowBase`）：
- **每行一个任务定义，RowName 即全局唯一 QuestID**（UE DataTable 强制行名唯一，天然去重）。
- 命名规范：`Quest_CollectWood_10` / `Quest_KillSlime_5` / `Quest_CatchAnyPet_3`。

### 3.2 任务类型枚举

```cpp
UENUM(BlueprintType)
enum class EMFQuestType : uint8
{
    CollectItem    UMETA(DisplayName = "收集物资"),   // 背包中某物资达到 N
    KillCreature   UMETA(DisplayName = "击杀生物"),   // 击杀某类生物 N 只
    CollectPet     UMETA(DisplayName = "收集宠物"),   // 本局抓到宠物 N 只
    InteractPoint  UMETA(DisplayName = "到点交互"),   // 预留：A2 目标点，本期不实现
};
```

### 3.3 行结构 `FMFQuestDef`

```cpp
USTRUCT(BlueprintType)
struct FMFQuestDef : public FTableRowBase
{
    /** 任务类型。 */
    UPROPERTY(EditDefaultsOnly) EMFQuestType Type = EMFQuestType::CollectItem;

    /**
     * 目标对象 ID，语义按 Type 解释；None = "任意"。
     *   CollectItem  → 物资 ItemID（UMFItemDatabase，如 Item.Resource.Wood）
     *   KillCreature → 生物类型 AIConfigID（DT_AIRegistry 的 RowKey；None = 任意敌方生物）
     *   CollectPet   → 宠物 AIConfigID（None = 任意宠物）
     */
    UPROPERTY(EditDefaultsOnly) FName TargetID;

    /** 需要的数量 N。 */
    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 1)) int32 RequiredCount = 1;

    /** 任务名（UI 显示）。 */
    UPROPERTY(EditDefaultsOnly) FText DisplayName;

    /** 任务描述（UI，可含 {0}=当前 {1}=目标 占位，运行时填充）。 */
    UPROPERTY(EditDefaultsOnly) FText Description;

    // --- 预留（MVP 不实现，占位便于将来加而不改结构）---
    /** 完成奖励（掉落表复用），MVP 留空。 */
    UPROPERTY(EditDefaultsOnly) TObjectPtr<class UMFLootTable> RewardTable;
};
```

一个 `Type + TargetID + RequiredCount` 三元组即可表达全部三种任务，无需按类型拆多张表。

### 3.4 配置示例（DT_QuestLibrary）

| RowName（=QuestID） | Type | TargetID | RequiredCount | DisplayName |
|---|---|---|---|---|
| `Quest_CollectWood_10` | CollectItem | `Item.Resource.Wood` | 10 | 收集木材 |
| `Quest_CollectMeat_5` | CollectItem | `Item.Resource.Meat` | 5 | 囤积肉食 |
| `Quest_KillSlime_5` | KillCreature | `Pet_SlimeCat` | 5 | 清剿史莱姆 |
| `Quest_KillAny_8` | KillCreature | *(None)* | 8 | 肃清野兽 |
| `Quest_CatchAnyPet_3` | CollectPet | *(None)* | 3 | 收服伙伴 |
| `Quest_CatchFox_1` | CollectPet | `Pet_FireFox` | 1 | 捕获火狐 |

## 4. 运行时

### 4.1 进度结构

```cpp
USTRUCT(BlueprintType)
struct FMFQuestProgress
{
    UPROPERTY(BlueprintReadOnly) FName QuestID;
    UPROPERTY(BlueprintReadOnly) int32 CurrentCount = 0;
    UPROPERTY(BlueprintReadOnly) bool  bCompleted = false;
};
```

### 4.2 `UMFQuestSubsystem`（WorldSubsystem）

```cpp
// 激活（进图编排 / exec 调用）——从 DT 读定义建进度条目并绑定事件源
void ActivateQuest(FName QuestID);
void ActivateQuests(const TArray<FName>& QuestIDs);

// 目标点（A2）上报入口——InteractPoint 型任务推进
void NotifyInteractPoint(FName QuestID);

// 查询
UFUNCTION(BlueprintPure) TArray<FMFQuestProgress> GetActiveQuests() const;
UFUNCTION(BlueprintPure) bool GetProgress(FName QuestID, FMFQuestProgress& Out) const;
UFUNCTION(BlueprintPure) bool AreAllCompleted() const;   // 无激活任务时返回 false（同 A2 边界约定）

// 事件（UI / 消费者订阅）
FOnMFQuestProgress       OnQuestProgress;      // (QuestID, Current, Required)
FOnMFQuestCompleted      OnQuestCompleted;     // (QuestID)
FOnMFAllQuestsCompleted  OnAllQuestsCompleted;
```

**配置载体**：`UMFQuestSettings : UDeveloperSettings(config=Game)` 持 `TSoftObjectPtr<UDataTable> QuestLibrary`（指向 DT_QuestLibrary），沿用 `UMFTelegraphSettings` / `UMFLootSettings` 模式。

**进度推进逻辑**：
- **CollectItem**：订阅玩家 `OnInventoryChanged` → `Current = min(GetResourceCount(TargetID), Required)`；达标锁 `bCompleted`（**达成即锁定，之后丢弃/消耗不回退**，见 §6）。
- **KillCreature**：订阅 `OnCreatureKilled(DeadAI)` → 若 `TargetID` 为 None 或匹配死者 AIConfigID → `Current++`。
- **CollectPet**：订阅 `OnPetCaught(Instance)` → 若 `TargetID` 为 None 或匹配 → `Current++`。
- **InteractPoint**：由 A2 Marker 调 `NotifyInteractPoint` → `Current++`。

**玩家延迟绑定**：WorldSubsystem 初始化早于玩家 Pawn，`ActivateQuest` 时若玩家背包未就绪，延迟到玩家就绪再订阅（仿 `AMFGameMode::M1_SubscribeToInventoryChanges` 的取玩家方式）。

### 4.3 需新增的两个事件源（增量）

- `AMFAICharacter`：加 `FOnCreatureKilled OnCreatureKilled`（静态/GameMode 级全局委托，或经 QuestSubsystem 直接调）。在 `HandleDeath` 里，**非玩家阵营**（不含 `Team.Player`）死亡时广播，携带自身 AIConfigID。击杀者归属 MVP 不追究（野外死亡≈玩家宠所为）。
- `UMFInventoryComponent`：加 `FOnPetCaught OnPetCaught`，`RegisterCaughtPet` 成功返回前广播新实例。

## 5. UI — 任务列表

- 新建 `UMFQuestLogWidget : UUserWidget`（BindWidget + C++ 驱动，仿 `UMFMainHUDWidget`）：
  - 一个列表容器，每个活动任务一行 `UMFQuestEntryWidget`：DisplayName + "当前/目标" + 完成勾。
  - 订阅 `OnQuestProgress` / `OnQuestCompleted` 刷新对应行。
- 挂载：`UMFMainHUDWidget` 加 `BindWidgetOptional` 的 `QuestLogPanel`（旧 WBP 不放不报错，同 `BossReadyBanner` 约定）。MVP 常驻 HUD 一角。

## 6. 决策点（方案默认已选，可否决）

| 点 | 默认选择 | 理由 |
|---|---|---|
| 收集类"N 个"语义 | **当前持有量达 N 即完成并锁定** | 简单可验证；配合撤离生存（得真攒够）；锁定后丢弃不回退，避免抖动 |
| 击杀归属 | **任意非玩家阵营生物死亡即计数**，不判击杀者 | 野外死亡基本是玩家宠所为；避免为 MVP 造伤害归属链 |
| 收集宠物语义 | **本局抓取次数累计**（非"当前持有"） | 抓宠是核心动词，累计更贴"收集"；撤离前损失不回退进度 |
| 任务失败/放弃 | **不做** | MVP 只有完成态；失败/超时留后置 |

## 7. 激活来源

- **MVP**：exec 手动激活（`MFActivateQuest <QuestID>`）跑通链路；正式"进图激活哪些任务"由 B3 启动编排 / 关卡配置调 `ActivateQuests`，接口本期做好、调用方后补。
- 进图激活集的配置（哪张图配哪几个任务）留到大图（A3）/编排（B3）时定，本期不硬编码。

## 8. 调试

- 新增 `LogMFQuest` 日志类别。
- exec（`AMFCharacter`，惯例同 `MFSpawnLoot`）：
  - `MFActivateQuest <QuestID>` — 激活一个任务；
  - `MFQuestStatus` — 打印所有活动任务进度；
  - `MFCompleteQuest <QuestID>` — 直接完成（快速验证下游消费者）。

## 9. 实现顺序（3 步 C++ + 1 步编辑器）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `Quest/` 目录 + `FMFQuestDef`/`EMFQuestType`/`FMFQuestProgress` + `UMFQuestSettings` + `UMFQuestSubsystem`（激活/查询/事件骨架）+ `LogMFQuest` | 编译过 |
| 2 | 三种进度推进 + 新增 `OnCreatureKilled`/`OnPetCaught` 两委托 + 延迟绑定玩家 + exec ×3 | `MFActivateQuest Quest_CollectWood_10` 后捡木头→进度涨→完成广播 |
| 3 | `UMFQuestLogWidget` + `UMFQuestEntryWidget` + HUD 挂载 | 任务列表实时显示进度/完成 |
| 4 | （编辑器）DT_QuestLibrary 填 §3.4 示例行 + WBP_QuestLog；MF Quest Settings 指定 DT | 见验收 |

**验收标准**：激活"收集木材×10"→ 捡木头 UI 进度 3/10→…→10/10 完成；激活"击杀史莱姆×5"→ 打死 5 只史莱姆逐个推进；激活"收集宠物×3"→ 抓 3 只任意宠完成；三任务全完成 → 日志广播 `OnAllQuestsCompleted`；`MFQuestStatus` 输出正确。

---

## 附：对 A2 目标点方案的影响

A2 落地时按本方案调整：目标点 Marker 不再挂独立 `UMFObjectiveSubsystem`，改为完成时调 `QuestSubsystem::NotifyInteractPoint(QuestID)`（每个 Marker 配一个所属 QuestID）。A2 的"只广播事件、出口解耦"精神不变——只是完成事件的汇聚点从独立 Objective 子系统改为统一的 Quest 子系统。`ObjectiveSystem_Design.md` 需相应标注此收敛。
