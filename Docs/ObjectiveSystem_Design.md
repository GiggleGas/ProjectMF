# 目标点/交互系统（Objective）设计方案

> 2026-07-02 ｜ Loop2 MVP 任务 A2 ｜ 状态：方案待评审，未动代码
> ⚠️ **2026-07-03 收敛**：新增 A1.7 任务系统（`QuestSystem_Design.md`）后，本方案的独立 `UMFObjectiveSubsystem` **计数职责并入 `UMFQuestSubsystem`**——目标点 = 任务的 `InteractPoint` 类型，Marker 完成时调 `QuestSubsystem::NotifyInteractPoint(所属 QuestID)`，不再另起 Objective 子系统。下文的 Subsystem/事件段按此理解为"上报 Quest"；Marker（2D 世界交互 + 读条）本体设计不变。
> 范围：目标点 Actor +「到点操作 N 处」交互 → 上报 Quest。
> 设计红线（2026-07-02 评审拍板）：**只广播事件，不含出口/解锁/结算语义**——撤离点、打 Boss 入口、献祭都订阅统一的 Quest `OnAllQuestsCompleted`，规则层保持薄。

---

## 1. 现状对接点（已核实）

| 现有资产 | 对接方式 |
|---|---|
| `GA_Pick` + `MF.GameplayState.Picking` tag 检测（`AMFGatherNode` 已建立此模式） | 目标点交互复用同款：到点按住采集键读条，**GA 零改动、零新输入**，玩家一套操作通吃采集/目标 |
| `AMFSceneActorBase`（2D Flipbook/PaperZD） | 目标点 Actor 派生自它——与场景 Actor 一致用 2D 图片表现（既定要求） |
| `AMFGameMode` M1 区块（竞技场循环，Loop2 将整体替换） | **不动、不接**。目标事件自持在 WorldSubsystem 上，避免绑死进将被替换的 M1 |
| `UMFMainHUDWidget` 的 `BindWidgetOptional` 模式（`BossReadyBanner` 先例） | HUD 加可选绑定 `ObjectiveText`（"目标 2/3"），旧 WBP 不加也不报错 |
| `MFLog` / `UFUNCTION(Exec)` on `AMFCharacter` 惯例 | 新增 `LogMFObjective` 类别 + 调试命令 |
| `UMFLootSubsystem`（WorldSubsystem 惯例） | 同模式新建 `UMFObjectiveSubsystem` |

## 2. 架构总览

```
AMFObjectiveMarker ×N (关卡手摆, SceneActorBase 派生, 2D 外观)
    │ BeginPlay 自注册
    ▼
UMFObjectiveSubsystem (WorldSubsystem — 注册池 / 激活 / 计数)
    │ 完成一处 → OnObjectiveProgress(完成数, 总数)
    │ 全部完成 → OnAllObjectivesCompleted
    ▼                          ▼
HUD "目标 2/3"          未来消费者（C1 撤离点 / Boss 入口 / 献祭 / 昼夜）
                        各自订阅，Subsystem 对它们零感知
```

## 3. 组件规格（新建 `Objective/` 目录，仿 `Loot/`）

### 3.1 `AMFObjectiveMarker` — `Objective/Public/MFObjectiveMarker.h`，派生 `AMFSceneActorBase`

**状态机**：`Dormant`（隐藏、不可交互）→ `Active`（显示、可交互）→ `Completed`（换外观/消失，终态）。

**完成方式**（枚举 `EMFObjectiveTriggerMode`，逐 Marker 配置）：
- `Interact`（默认）：玩家进 `InteractRadius` 按住采集键，读条 `InteractDuration` 秒 → 完成。检测方式与 `AMFGatherNode` 相同（Tick 查玩家距离 + `State.Picking` tag），读条期间画 debug 进度条（同款）。
- `Presence`：玩家进入 `PresenceRadius` 即完成（探索型目标，如"抵达某处"）。

**字段**：`TriggerMode`、`InteractDuration=2s`、`InteractRadius=180`、`PresenceRadius=150`、`bKeepProgressOnInterrupt=false`、`bAutoActivate=true`。

**BP 钩子**：`OnActivated`（显示标记外观/头顶图标）、`OnCompletedVisual`（换完成外观或播动画后自灭）。Dormant/Active 的显隐由 C++ 统一处理（SetActorHiddenInGame + 碰撞开关），BP 只管外观细节。

**引导表现（MVP）**：Active 时头顶显示一个 2D 悬浮标记——直接用基类 Flipbook 配一个循环动画（如旗帜/光柱贴图）即可，无需额外组件。屏幕边缘方向指示器**后置**（polish，不进 MVP）。

### 3.2 `UMFObjectiveSubsystem` — `Objective/Public/MFObjectiveSubsystem.h`，`UWorldSubsystem`

```cpp
// 注册（Marker BeginPlay/EndPlay 自调用）
void RegisterMarker(AMFObjectiveMarker* Marker);
void UnregisterMarker(AMFObjectiveMarker* Marker);

// Marker 完成时回调 → 推进计数 + 广播
void NotifyCompleted(AMFObjectiveMarker* Marker);

// 进图编排接口（预留给 B3 启动编排/GameLoopConfig；MVP 可不调用）
// 把注册池全部置 Dormant，随机激活其中 Count 个。Count<=0 或超池 = 全激活。
UFUNCTION(BlueprintCallable) void ActivateRandomSubset(int32 Count);

// 查询
UFUNCTION(BlueprintPure) int32 GetActiveTotal() const;      // 激活总数（含已完成）
UFUNCTION(BlueprintPure) int32 GetCompletedCount() const;
UFUNCTION(BlueprintPure) bool  AreAllCompleted() const;     // 无激活目标时返回 false（见 §5）

// 事件（DYNAMIC_MULTICAST，UI/BP/未来消费者订阅）
UPROPERTY(BlueprintAssignable) FOnMFObjectiveProgress      OnObjectiveProgress;     // (Completed, Total)
UPROPERTY(BlueprintAssignable) FOnMFAllObjectivesCompleted OnAllObjectivesCompleted;
```

**明确不做**：出口解锁、阶段切换、结算、存档——全是消费者的事。Subsystem 只报事实。

### 3.3 激活策略（对齐"进图标记几处目标"的设计意图）

- **MVP 默认**：Marker `bAutoActivate=true` 自激活——图里摆几个就是几个目标，零编排依赖，立即可玩。
- **将来**：进图编排（B3/M6）调 `ActivateRandomSubset(M)`——池里摆 8 个、每局随机标 3 个，重玩性用摆放密度换。接口本期做好，调用方后补。

### 3.4 HUD 计数

`UMFMainHUDWidget` 加 `BindWidgetOptional` 的 `ObjectiveText`：`InitPlayerHUD` 里订阅 `OnObjectiveProgress` → 刷新"目标 2/3"；全部完成刷新为"目标完成"（后续 C1 撤离点激活提示由消费者自己接）。WBP 未放该控件时静默跳过（`BossReadyBanner` 同款约定）。

### 3.5 调试

- 新增 `LogMFObjective` 日志类别。
- exec（`AMFCharacter`，惯例同 `MFSpawnLoot`）：
  - `MFCompleteNextObjective` — 直接完成一个未完成的激活目标（快速跑通链路）；
  - `MFObjectiveStatus` — 打印激活/完成计数与各 Marker 状态。

## 4. 与未来消费者的对接（本期不实现，只留缝）

| 消费者 | 接法 |
|---|---|
| C1 撤离点 | 自身配置 `bRequireObjectives`；订阅 `OnAllObjectivesCompleted` → 激活可交互 |
| 打 Boss 入口 / Boss 献祭降生 | 同上订阅；或读 `AreAllCompleted()` 轮询 |
| Loop2 GameMode（替换 M1 后） | 编排期调 `ActivateRandomSubset`，结算期读计数 |
| 任务类型扩展（采够量/护送/净化） | 新 Marker 子类覆写"何时调 NotifyCompleted"，Subsystem 与事件不变 |

## 5. 边界情况约定

- **0 个激活目标**：`AreAllCompleted()` 返回 **false**，不广播 `OnAllObjectivesCompleted`——空图不误触发"全完成"；无目标图的撤离点应配 `bRequireObjectives=false`（消费者决策）。
- **同时在多个 Interact Marker 半径内按住采集**：各自独立读条（都在读）。MVP 接受；摆放时目标点间距 > 2×InteractRadius 即可规避。
- **与 GatherNode 同位置**：同一个"按住采集"会同时推进两者读条——摆放规避（目标点别压采集点），不做代码互斥。
- **重复进图**：目标状态是纯运行时（WorldSubsystem 随关卡生灭），OpenLevel 天然重置，不存档。
- **Completed 终态**：Presence 重复进入、Interact 再读条均忽略。

## 6. 实现顺序（3 步 C++ + 1 步编辑器）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `Objective/` 目录 + Marker（状态机/自注册）+ Subsystem（注册池/计数/事件）+ LogMFObjective | 编译过 |
| 2 | Interact 读条 + Presence 触发 + 完成流转 + debug 进度条 + `ActivateRandomSubset` | exec 后见事件日志 |
| 3 | HUD `ObjectiveText` + exec ×2 | `MFCompleteNextObjective` 推进 HUD 计数 |
| 4 | （编辑器）BP_ObjectiveMarker 配 2D 标记外观；测试图摆 3 个（2 Interact + 1 Presence） | 见验收 |

**验收标准**：进图 HUD 显示"目标 0/3"；走到 Presence 点即变 1/3；两处按住采集键读条 2s 各推进一格；全完成时 HUD 变"目标完成"且日志广播 `OnAllObjectivesCompleted`；`MFObjectiveStatus` 输出正确。
