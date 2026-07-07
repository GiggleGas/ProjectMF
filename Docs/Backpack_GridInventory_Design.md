# 背包格子管理（饥荒式）设计方案

> 2026-07-03 立项 ｜ 2026-07-06 对齐物品重构、拆分逻辑/UI ｜ Loop2 MVP 任务 A1.5
> 状态：逻辑层已实现 + 编译通过（2026-07-06）；**本轮做 UI 层**（格子 widget + HUD 挂载 + 丢弃交互）。
> 范围：固定格子数 + 每格堆叠上限 + 同种物品多格堆叠 + 溢出留地 + 丢弃 + 饥荒式格子 UI。
> 明确不做（用户拍板）：搜打撤（Tarkov）式网格 Tetris——不做物品尺寸/形状/重量/容器嵌套/绑定。

---

## 1. 现状核实（物品重构后，2026-07-06）

`UMFInventoryComponent` 已有资源接口，但**不是饥荒式**，且有堆叠缺陷：

| 现状 | 说明 |
|---|---|
| `FMFItemDef.MaxStackSize`（默认 99） | ✅ 堆叠上限字段已存在，`UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID)` 查 |
| `MaxResourceSlots`（PlayerConfig 注入，int32） | ✅ 容量上限已有；`=0` 当前视为无限，本方案改为必须正数 |
| `FMFInventorySlot`：`int32 ItemID` + `int32 Count`（均 SaveGame） | ✅ 已 int32 化 + 加 SaveGame（物品重构 + A1 已完成） |
| `ResourceSlots` 是**动态紧凑数组**（`Add`/`RemoveAt`） | ✗ 无固定格位——做不了"格子有固定位置/中间可空/可整理" |
| `AddResource` 用 `FindResourceSlotIndex` 只找**一个**同种格 | ✗ 堆满后溢出只新建一格，再多直接丢弃 |
| 同上，命中已满格就跳过半满格 | ✗ 同种物品会散落多个半满格（堆叠 bug） |
| `AddResource` 校验 `ItemType != Resource` 拒绝 | ✗ 消耗品（打造 A1.8 产出）进不了包，需放开 |
| 拾取：`AMFLootPickup::PickUp()` 虚空销毁（无背包） | 本轮接背包：改为 `AddResource` 成功再销毁 |
| `OnInventoryChanged` 广播已有 | ✅ UI 订阅它刷新 |

**结论**：重写数据模型（紧凑→固定索引数组）+ 修正多格堆叠 + 放开消耗品 + 拾取接入背包。

---

## 2. 逻辑层（本轮）

### 2.1 数据模型：紧凑 → 固定索引数组

`ResourceSlots` 改为**固定容量索引数组**：
- BeginPlay 注入 `MaxResourceSlots` 后 `ResourceSlots.SetNum(MaxResourceSlots)`，预填空格（`ItemID=0, Count=0`）。数组长度恒 = 容量，下标 = 格位。
- 空格判定：`Slot.ItemID <= 0 || Slot.Count <= 0`。
- `FMFInventorySlot` 结构不加字段。
- `MaxResourceSlots` 必须正数（PlayerConfig 配，建议 **20**）；`<=0` 视为配置错误，打警告并退化为不限格（仅兜底）。

### 2.2 `AddResource(int32 ItemID, int32 Count)` 重写

```
校验：ItemID>0 && Count>0；Def = FindItem(GetItemTable(), ItemID) 存在；
      Def.ItemType 属于「可叠加类」（Resource 或 Consumable）——放开消耗品
MaxStack = Def.MaxStackSize
Remaining = Count
// 第一遍：填所有未满的同种格（从前往后）
for slot in ResourceSlots where slot.ItemID==ItemID && slot.Count<MaxStack:
    move = min(MaxStack - slot.Count, Remaining); slot.Count += move; Remaining -= move
// 第二遍：占空格新建堆
for slot in ResourceSlots where slot is empty, while Remaining>0:
    slot.ItemID = ItemID; slot.Count = min(MaxStack, Remaining); Remaining -= slot.Count
Added = Count - Remaining
if Added>0: OnInventoryChanged.Broadcast()
return Added        // 契约不变：返回实际加入数，背包满则 < Count
```

### 2.3 `RemoveResource` / 查询 / 丢弃

- `RemoveResource(ItemID, Count)`：从后往前扣同种格；扣空的格**置空**（`ItemID=0, Count=0`）而非 `RemoveAt`，保持固定长度与格位。数量不足返回 false。
- `GetResourceCount` / `HasResource`：遍历求和，空格 Count=0 无影响——**无需改**。
- 新增 `bool DropSlot(int32 SlotIndex)`：把该格全部内容经 `UMFLootSubsystem::SpawnLoot` 生成地面掉落物（落玩家脚下，可再捡），然后置空该格。背包满时腾地方的必要出口。

### 2.4 拾取入包（替代旧的虚空/自动吸附）

拾取仍是玩家空格键就近交互（`MFCharacter::HandleCarryOrRevive` 已接），本轮把"虚空销毁"换成"入包"：
- `AMFLootPickup` 提供 `int32 TryPickUpInto(UMFInventoryComponent* Inv)`：调 `Inv->AddResource(ItemID, Count)`，返回实际入包数：
  - **全部入包 → Destroy**。
  - **部分 / 完全没入（背包满）→ 扣掉已入部分，剩余「重新抛一次散落」**（`BounceOut()`：从当前位置重新抛到附近随机落点、重播散开动画），而**不是静止不动**。给玩家明确反馈：按了空格 → 东西弹起又落下 → "背包满了，装不下弹回来"，避免"按键没反应"的困惑。
- `MFCharacter` 拾取分支改为调用它。当前每个掉落物 Count=1，要么进包销毁、要么满了弹一次。

### 2.5 事件

复用现有 `OnInventoryChanged`（任意格变化时广播，全量）。UI 订阅它整体刷新 N 个格子（量小，无需增量）。

### 2.6 调用点核查（实现时）

- `MFCharacter` 拾取分支：虚空 `PickUp()` → `TryPickUpInto`。
- `MF.Inventory.Debug` 遍历显示：跳过空格（ItemID<=0）。
- grep `GetResourceSlots()` 所有调用点，确认无"紧凑无空格"隐含假设（当前仅 debug 打印）。

---

## 3. UI 层（本轮）

**原则**：逻辑全在 C++（仿现有 `UMFPetSlotWidget` / `UMFMainHUDWidget` 模式）；蓝图只在 Designer 摆同名 BindWidget 控件 + 填几个 Details 配置，**不连任何蓝图节点**。

### 3.1 `UMFItemSlotWidget : UUserWidget`（单个格子，仿 `UMFPetSlotWidget`）

- **BindWidget**（蓝图 Designer 摆同名控件）：
  - `UImage* Icon` — 物品图标。
  - `UTextBlock* CountText` — 数量文本。
- **BindWidgetOptional**：`UImage* Background`（格子底框，空/满都常驻显示——这是"空位置"的视觉载体）。
- **C++ 接口**：
  - `void SetSlot(int32 SlotIndex, int32 ItemID, int32 Count)` — 填充：
    - **空格（`ItemID<=0`）→ 隐藏 Icon + CountText，但格子本身（Designer 摆的底框/背景）保留显示**，即呈现为"空位"。
    - 有物 → `UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID)` 取 `FMFItemDef.Icon` 设到 `Icon` 并显示；`Count>1` 显示数字、否则隐藏（可配总显示）。
    - 末尾调 `OnSlotVisualUpdated(bEmpty)`（`BlueprintImplementableEvent`）——让蓝图定制空/满外观（如空格淡灰底、满格高亮框），不写 C++ 也能区分空满。
  - 记录 `SlotIndex`（丢弃用）。
- **丢弃交互**：override `NativeOnMouseButtonDown` 检测**右键** → 广播 `OnSlotRightClicked(SlotIndex)` 委托（不误触；左键留给将来拖拽）。逻辑本身不在格子里做，交给容器。

### 3.2 `UMFBackpackWidget : UUserWidget`（格子容器，仿 HUD 的 PetSlot 部分）

- **BindWidget**：`UUniformGridPanel* SlotGrid` — 格子网格容器。
- **EditDefaultsOnly**（蓝图 Details 配）：
  - `TSubclassOf<UMFItemSlotWidget> ItemSlotClass` — 指 WBP_ItemSlot。
  - `int32 Columns = 10` — 每行列数（20 格 = 10×2）。
- **C++ 接口**：
  - `void InitBackpack(UMFInventoryComponent* Inv)` — 记 `BoundInventory`、订阅 `OnInventoryChanged`、`BuildSlots()` 首建、`RefreshSlots()` 填充。由 `UMFMainHUDWidget::InitPlayerHUD` 调用。
- **C++ 内部**：
  - `BuildSlots()` — 按 `GetResourceSlots().Num()`（= 固定容量 N，含空格）生成 **N 个** `ItemSlotWidget` 缓存进 `TArray`，`AddChildToUniformGrid(w, index/Columns, index%Columns)`。**只首次建一次**。
    - ⚠️ 与 `PetSlotWidget` 的关键区别：宠物卡槽按花名册**动态数量**生成（无空位）；背包**固定画满 N 格**，空格也画出来（"默认空位置展示"）。这是固定索引数组（逻辑层）在 UI 上的直接体现。
  - `RefreshSlots()` — 遍历 `GetResourceSlots()` 逐格 `SetSlot`（**不重建**，固定格子平滑刷新）。绑 `OnInventoryChanged`。
  - `HandleSlotRightClicked(int32 SlotIndex)` — 订阅格子委托 → `BoundInventory->DropSlot(SlotIndex)`（掉回脚下，逻辑层已实现）。
  - `NativeDestruct` 取消订阅。

### 3.3 挂载到 HUD

- `UMFMainHUDWidget` 加 `BindWidgetOptional` 的 `UMFBackpackWidget* BackpackWidget`。
- `InitPlayerHUD(Player)` 里：拿到 `InventoryComponent` 后 `if (BackpackWidget) BackpackWidget->InitBackpack(Inv);`（与现有 `RefreshPetSlots` 并列，复用同一个 Inventory）。
- 旧 WBP_MainHUD 不放 BackpackWidget 也不报错（`BossReadyBanner` 同款约定）。

### 3.4 数据流

```
InventoryComponent.OnInventoryChanged（AddResource/RemoveResource/DropSlot 都广播）
   → UMFBackpackWidget::RefreshSlots
   → 逐格 UMFItemSlotWidget::SetSlot(index, itemID, count)
   → Icon(FMFItemDef.Icon via GetItemTable) + CountText；空格清空
```

### 3.5 呈现方式

常驻 HUD 底部格子栏（饥荒式），20 格 = 10 列 × 2 行 UniformGrid。无开合输入态（MVP 最简）；将来要开合面板再包一层。

### 3.6 蓝图侧要配的（纯配置，无节点）

1. **WBP_ItemSlot**（父类 `UMFItemSlotWidget`）：Designer 放 `Image` 命名 `Icon` + `TextBlock` 命名 `CountText`，调格子外观（边框/背景/大小）。
2. **WBP_Backpack**（父类 `UMFBackpackWidget`）：Designer 放 `UniformGridPanel` 命名 `SlotGrid`；Details 设 `ItemSlotClass=WBP_ItemSlot`、`Columns=10`。
3. **WBP_MainHUD**：放 WBP_Backpack 命名 `BackpackWidget`（匹配 BindWidgetOptional），摆底部。
4. **DT_Item** 每个物品配 `Icon`（UI 图标，之前 WorldSprite 是场景外观，Icon 是背包/UI 用，两者分开）。

### 3.7 实现顺序（3 步 C++ + 1 步编辑器）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `UMFItemSlotWidget`（SetSlot + 右键委托） | 编译过 |
| 2 | `UMFBackpackWidget`（InitBackpack/BuildSlots/RefreshSlots/订阅/丢弃） | 编译过 |
| 3 | `UMFMainHUDWidget` 加 `BackpackWidget` + InitPlayerHUD 初始化 | 编译过 |
| 4 | （编辑器）WBP_ItemSlot / WBP_Backpack / 挂 WBP_MainHUD / DT_Item 配 Icon | 见验收 |

**UI 层验收**：捡物 → 底部格子实时显示 Icon + 数量；同种超单格上限 → 显示两格；右键格子 → 物品掉回脚下（DropSlot）且该格变空；背包满弹回时格子不变（逻辑层已处理）。

---

## 4. 背包满行为（饥荒式）

- 拾取时背包满 → 掉落物**重新抛一次散落**（`BounceOut`，弹起重新落地），不销毁、不静止——给玩家"捡了但装不下弹回来"的明确反馈，避免"按键没反应"的困惑。玩家腾地方（丢弃/消耗）后再来捡。
- `DropSlot` 是主动腾地方的出口。
- 与"未撤离=丢"的生存风险呼应（元层阶段才接）。

## 5. 决策点（默认已选，可否决）

| 点 | 默认 | 理由 |
|---|---|---|
| 消耗品是否现在放开进包 | **放开**（Resource + Consumable 都可叠加进格）✅ 已确认 | 打造 A1.8 产出消耗品，背包本就该装；一步到位免得 A1.8 再改 |
| 背包容量 `MaxResourceSlots` | **20** ✅ 已确认 | 够玩一局采集，格子网格显示不挤 |
| 背包满 / 部分入包 | **重新抛一次散落**（弹起重落，非静止留地）✅ 已确认 | 明确反馈"捡了装不下弹回来"，避免按键没反应的困惑 |
| 拖拽换位 / 堆叠拆分 / 排序 | **后置 A1.5b** | 依赖 UMG DragDrop，工作量偏重，乐趣纵切用不到 |

## 6. 与 B1 元层存档的关系

背包数据模型（固定数组含空格）**定型后再做 B1**——B1 序列化 `ResourceSlots` 直接存整个固定数组（`FMFInventorySlot` 的 SaveGame 已具备）。这是 A1.5 排在 B1 前的额外理由：避免存档结构先建后改。

## 7. 实现顺序（本轮 = 逻辑层）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `ResourceSlots` 固定数组化（BeginPlay SetNum 预填空格）+ 空格判定 | `MF.Inventory.Debug` 显示固定 N 格含空格 |
| 2 | `AddResource` 重写（多格堆叠 + 放开 Consumable）+ `RemoveResource` 置空 | `MFSpawnLoot` 超单格上限自动开第二格；扣空不塌缩 |
| 3 | `DropSlot` + 拾取 `TryPickUpInto`（虚空 → 入包）| 空格键捡物进背包；背包满留地；丢弃掉回脚下 |

**本轮验收（无 UI，靠 `MF.Inventory.Debug`）**：捡同种物资超单格 `MaxStackSize` 自动占第二格；背包所有格占满后继续捡 → 掉落物留地面；`MFSpawnLoot 消耗品ID` 也能进包（消耗品放开）；`RemoveResource` 扣空的格变空且格位不塌缩。UI 层留下一轮。
