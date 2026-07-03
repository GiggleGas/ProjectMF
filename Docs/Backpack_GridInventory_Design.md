# 背包格子管理（饥荒式）设计方案

> 2026-07-03 ｜ Loop2 MVP 任务 A1.5（插在 A1 掉落物之后、A2 目标点之前）｜ 状态：方案待评审，未动代码
> 范围：固定格子数 + 每格堆叠上限 + 同种物品多格堆叠 + 溢出掉地 + 丢弃 + 饥荒式格子 UI。
> 明确不做（用户拍板）：搜打撤（Tarkov）式网格 Tetris——不做物品尺寸/形状/重量/容器嵌套/绑定。

---

## 1. 现状核实（已读代码）

现有 `UMFInventoryComponent` 已有雏形，但**不是饥荒式**，且有缺陷：

| 现状 | 问题 |
|---|---|
| `FMFItemDef.MaxStackSize`（默认 99） | ✅ 堆叠上限字段已存在，直接复用 |
| `MaxResourceSlots`（PlayerConfig 注入） | ✅ 容量上限已有；但当前 `>0` 才生效，`=0` 视为无限 |
| `ResourceSlots` 是**动态紧凑数组**（`Add`/`RemoveAt`） | ✗ 无固定格位——做不了"格子有固定位置/中间可空/可整理" |
| `AddResource` 用 `FindResourceSlotIndex` 只找**一个**同种格 | ✗ 堆满后溢出**只新建一格**，再多直接丢弃 |
| 同上，命中已满格就跳过半满格 | ✗ 同种物品会散落多个半满格（堆叠 bug） |

**结论**：需要重写数据模型（紧凑→固定索引数组）+ 修正多格堆叠 + 补 UI 与丢弃。溢出掉地已由 A1 承接（见 §5）。

## 2. 范围（要 / 不要）

**要（饥荒式核心）**
- 固定 N 个格子，格位固定（格子 3 可空、格子 4 有物）。
- 每格 = 一种物品 × 该物品 `MaxStackSize`。
- 同种物品堆满一格 → 自动开下一个空格。
- 背包占满（所有格满或无空格容纳）→ 捡不下 → 掉回地面（A1 已实现）。
- 丢弃某格 → 物资掉回地面（可再捡）。
- 饥荒式格子网格 UI（Icon + 数量 + 空格）。

**不要**
- 物品尺寸/形状（Tetris）、重量、容器嵌套、装备绑定。
- 拖拽换位 / 堆叠拆分 / 自动排序 → **后置**（见 §7），非饥荒式核心手感的必需项。

## 3. 数据模型改造

### 3.1 固定索引数组

`ResourceSlots` 从"动态紧凑数组"改为"**固定容量索引数组**"：
- `MaxResourceSlots > 0`：BeginPlay 注入后 `ResourceSlots.SetNum(MaxResourceSlots)`，预填空格（`ItemID = None, Count = 0`）。数组长度恒 = 容量，索引 = 格位。
- 空格判定：`Slot.ItemID.IsNone() || Slot.Count <= 0`。
- `FMFInventorySlot` 不加字段（ItemID + Count 够用；SaveGame 修饰符 A1 已补）。

> `MaxResourceSlots == 0` 的"无限"旧语义**废弃**：本作是生存式有限背包，容量必须是正数（PlayerConfig 配，建议 15~20）。=0 时回退旧紧凑逻辑仅作兼容兜底并打警告。

### 3.2 `AddResource` 重写（多格堆叠正确）

```
int32 AddResource(ItemID, Count):
    校验（Def 存在 / 是 Resource 类型）——不变
    Remaining = Count
    // 第一遍：填所有未满的同种格（从前往后）
    for each slot where slot.ItemID == ItemID and slot.Count < MaxStack:
        move = min(MaxStack - slot.Count, Remaining); slot.Count += move; Remaining -= move
    // 第二遍：占空格新建堆
    for each empty slot while Remaining > 0:
        slot.ItemID = ItemID; slot.Count = min(MaxStack, Remaining); Remaining -= slot.Count
    Added = Count - Remaining
    if Added > 0: broadcast OnInventoryChanged
    return Added        // 契约不变：返回实际加入数，满则 < Count
```

### 3.3 `RemoveResource` / 其余

- `RemoveResource`：从后往前扣同种格；扣空的格**置空**（`ItemID=None,Count=0`）而非 `RemoveAt`（保持固定长度与格位）。
- `GetResourceCount` / `HasResource`：遍历求和，空格 Count=0 无影响——**无需改**。
- 新增 `bool DropSlot(int32 SlotIndex)`：把该格全部内容生成地面 `AMFLootPickup`（复用 `UMFLootSubsystem::SpawnLoot`，落玩家脚下可再捡）后置空该格。这是"背包满时腾地方"的必要出口。

### 3.4 调用点核查

- `AMFLootPickup::TryGiveTo`（A1）只用 `AddResource` 返回值，**不遍历 slots** → 不受影响。
- `PrintInventoryDebug` / `MF.Inventory.Debug` 遍历显示 → 跳过空格即可。
- 实现时 grep `GetResourceSlots()` 全部调用点，确认无"数组紧凑无空格"的隐含假设。

## 4. UI（饥荒式格子网格）

- 新建 `UMFBackpackWidget : UUserWidget`（仿 `UMFMainHUDWidget` 的 BindWidget + C++ 驱动模式）：
  - 一个 `UUniformGridPanel`（或 WrapBox），按 `MaxResourceSlots` 生成 N 个 `UMFItemSlotWidget`。
  - 每格 Widget：Icon（`FMFItemDef.Icon`）+ 数量文本；空格显示空底。
  - 订阅 `OnInventoryChanged` → 刷新全部格子（MVP 全量刷新，量小无需增量）。
- **呈现方式（MVP）**：常驻 HUD 底部一条格子栏（饥荒式），或按键（如 `B`）开合的面板。推荐**常驻底部栏**——最贴饥荒、无需额外输入态管理。
- 挂载：`UMFMainHUDWidget` 加 `BindWidgetOptional` 的 `BackpackPanel`，旧 WBP 不放也不报错（`BossReadyBanner` 同款约定）。

## 5. 溢出与 A1 对接（已闭合）

A1 的 `AMFLootPickup::TryGiveTo` 已消费 `AddResource` 返回值：`Added < Count` 时扣除已入部分、原地回落冷却重试——即**背包满 → 掉落物留在地面**，与饥荒一致。本任务只需保证重写后的 `AddResource` 在满时返回真实可容纳数（§3.2 已满足），A1 侧零改动。

## 6. 与 B1 元层存档的关系

背包数据模型（固定数组含空格）**定型后再做 B1**——B1 序列化 `ResourceSlots` 直接存整个固定数组。这也是把 A1.5 排在 B1 之前的额外理由：避免存档结构先建后改。（`FMFInventorySlot` 的 SaveGame 修饰符 A1 已补。）

## 7. 分档

**MVP（本任务 A1.5）**：§3 数据层重构 + §4 网格 UI + `DropSlot` 丢弃 + §5 溢出（已完成）。
**后置（A1.5b，polish，不阻塞 MVP）**：拖拽换位 / 堆叠拆分（右键分半）/ 一键整理排序——依赖 UMG DragDrop，工作量偏重，"乐趣纵切"不需要它。

## 8. 实现顺序（3 步 C++ + 1 步编辑器）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `ResourceSlots` 固定数组化 + `AddResource` 重写 + `RemoveResource` 置空 + `DropSlot` + 调用点核查 | `MF.Inventory.Debug` 看多格堆叠正确；`MFSpawnLoot` 超单格上限自动开第二格 |
| 2 | `UMFBackpackWidget` + `UMFItemSlotWidget` + 订阅刷新 | 捡物资 UI 实时显示格子/数量/空格 |
| 3 | 丢弃交互（格子 UI 按钮或按键）→ `DropSlot` → 地面 Pickup | 丢一格 → 掉回地面可再捡 |
| 4 | （编辑器）WBP_Backpack + WBP_ItemSlot 外观；MaxResourceSlots 配 15 | 见验收 |

**验收标准**：捡同种物资超过单格 `MaxStackSize` 自动占第二格；背包所有格占满后继续捡 → 掉落物留地面；丢弃某格 → 物资掉回脚下可再捡；UI 正确显示每格 Icon/数量/空格；`RemoveResource`（消耗/制作用）扣空的格变空且不塌缩格位。
