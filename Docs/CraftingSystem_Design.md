# 打造合成系统（Crafting）设计方案

> 2026-07-06 ｜ 月底玩家测试版 W2 任务（替换原任务系统）｜ 状态：方案待评审，未动代码
> 范围：配方配置（唯一 ID）+ 合成逻辑 + 合成 UI（背包内一页）+ 产出宠物消耗品/食物 + 喂宠使用（回血/临时增益）。
> 产出用途（2026-07-06 用户拍板）：**宠物消耗品/食物**——采集物→合成食物/药→对宠物使用回血或临时 buff。装备/战斗道具留后。

---

## 1. 定位

验证核心后勤循环：**采集资源 → 打造宠物道具 → 喂宠续航/变强 → 打 Boss**。这比任务系统更贴游戏 Pillar 2（玩家是后勤，靠制作让宠物变强），也让"采集到底有什么用"形成内在闭环。

## 2. 现状对接点（已核实）

| 现有资产 | 对接方式 |
|---|---|
| 背包（A1.5）`ResourceSlots` + `AddResource`/`RemoveResource` | 合成扣料走 `RemoveResource`，产出入包走 `AddResource`。**依赖 A1.5 先完成** |
| `FMFItemDef.ItemType` 已有 `Consumable` 枚举（预留） | 产出的食物 = `Consumable` 类物品，进 ItemDatabase |
| `FMFItemDef` + `MaxStackSize` | 消耗品也能叠加进背包格子（格子只存 ItemID+Count，不区分类型） |
| GAS：`GE_Heal`（濒死/治疗已有回血）/ `OutgoingDamageMultiplier` + `Data_OutgoingDamageMult` SetByCaller | 消耗品的"回血/临时增伤"直接复用现有 GE，零新战斗数值 |
| `InventoryComponent::GetActivePetActors()` | 喂食目标=场上召唤宠，已有获取接口 |

**⚠️ 需处理的现有约束**：`AddResource` 当前有 `if (Def->ItemType != EMFItemType::Resource) return 0`（[MFInventoryComponent.cpp:102](../Source/ProjectMF/Inventory/Private/MFInventoryComponent.cpp#L102)）——消耗品进不了包。放开为接受 `Resource` + `Consumable`（都是可叠加物品），或在 A1.5 背包重构时泛化为 `AddStackableItem`。列为打造的前置改动。

## 3. 配置方案

### 3.1 配方：DataTable，RowName = 唯一配方 ID

建 `DT_RecipeLibrary`（行 `FMFRecipeDef : FTableRowBase`），沿用 Quest/AIRegistry 的 DataTable 模式，RowName 即唯一配方 ID（`Recipe_HealSnack` / `Recipe_PowerSnack`）。

```cpp
USTRUCT(BlueprintType)
struct FMFItemCount   // 复用于配方输入
{
    UPROPERTY(EditDefaultsOnly) FName ItemID;
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=1)) int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FMFRecipeDef : public FTableRowBase
{
    /** 需要消耗的资源（全部满足才可合成）。 */
    UPROPERTY(EditDefaultsOnly) TArray<FMFItemCount> Inputs;

    /** 产出物品 ID（引用 ItemDatabase，通常 Consumable 类）。 */
    UPROPERTY(EditDefaultsOnly) FName OutputItemID;

    UPROPERTY(EditDefaultsOnly, meta=(ClampMin=1)) int32 OutputCount = 1;

    /** UI 显示名（留空回退产出物 DisplayName）。 */
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
};
```

### 3.2 消耗品的"使用效果"：ItemDef 加一字段

消耗品也是 `FMFItemDef`（ItemType=Consumable），加一个**仅消耗品用**的字段：

```cpp
// FMFItemDef 追加：
/** 使用时对目标宠物施加的 GE（仅 Consumable 有意义）。空 = 无效果。 */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable")
TSubclassOf<UGameplayEffect> UseEffect;
```

消耗品定义集中在 ItemDatabase，无需另建表。

### 3.3 配置示例

**DT_RecipeLibrary**：
| RowName（=配方 ID） | Inputs | OutputItemID | OutputCount |
|---|---|---|---|
| `Recipe_HealSnack` | Meat×2 | `Item.Consumable.HealSnack` | 1 |
| `Recipe_PowerSnack` | Meat×1 + Wood×2 | `Item.Consumable.PowerSnack` | 1 |

**ItemDatabase 消耗品条目**：
| ItemID | ItemType | UseEffect |
|---|---|---|
| `Item.Consumable.HealSnack` | Consumable | GE_Heal（回血） |
| `Item.Consumable.PowerSnack` | Consumable | GE_AttackUp（临时增伤，时限 GE） |

## 4. 运行时

### 4.1 `UMFCraftingComponent`（挂玩家，仿 InventoryComponent 注入模式）

```cpp
// 配置（PlayerConfig 注入 RecipeLibrary DataTable）
UPROPERTY() TObjectPtr<UDataTable> RecipeLibrary;

// 查询
UFUNCTION(BlueprintPure) bool CanCraft(FName RecipeID) const;   // 背包料够？
UFUNCTION(BlueprintPure) TArray<FName> GetAllRecipes() const;

// 执行：扣料（RemoveResource）+ 产出入包（AddResource）
UFUNCTION(BlueprintCallable) bool Craft(FName RecipeID);

// 背包/合成状态变化时广播，UI 刷新可合成高亮
FOnCraftableChanged OnCraftableChanged;   // 订阅 Inventory.OnInventoryChanged 转发
```

合成逻辑极轻：`CanCraft` 遍历 Inputs 查 `HasResource`；`Craft` 先校验再逐项 `RemoveResource` + `AddResource(OutputItemID, OutputCount)`。

### 4.2 消耗品使用（喂宠）

- 入口：背包 UI 里消耗品格子 → 点击"使用"（或右键）。
- 目标：**对场上所有召唤宠施加 `UseEffect`**（`GetActivePetActors()` 遍历 ApplyGE）——省去选择目标交互（MVP 推荐）；扣 1 个消耗品。
- 效果复用现有 GE：回血用 `GE_Heal`，临时增伤用带时限的 `GE_AttackUp`（改 `OutgoingDamageMultiplier`）。

### 4.3 合成 UI

- `UMFCraftingWidget`：列出 `GetAllRecipes()`，每行显示配方名 + 输入料 + 产出；`CanCraft` 为真则高亮可点，点击调 `Craft`。
- 嵌入背包 UI 一页（`BindWidgetOptional`），随背包开合，不加新输入。

## 5. 决策点（默认已选，可否决）

| 点 | 默认 | 理由 |
|---|---|---|
| 喂食目标 | **全体召唤宠**（不选单只） | 省交互；战斗中喂食不打断操作 |
| 产出效果种类 | **回血 + 临时增伤两种** | 覆盖"续航"和"变强"，足够验证后勤循环 |
| 合成场所 | **背包内合成页**，不摆合成台 | 省一个 Actor + 摆放，MVP 随身合成 |
| 使用触发 | **背包内点击消耗品即用** | 无新输入键 |

## 6. 实现顺序（3 步 C++ + 1 步编辑器）

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | `FMFRecipeDef`/`FMFItemCount` + `DT_RecipeLibrary` + `FMFItemDef` 加 `UseEffect` + **AddResource 放开 Consumable** + `LogMFCraft` | 编译过；消耗品能进背包 |
| 2 | `UMFCraftingComponent`（CanCraft/Craft）+ PlayerConfig 注入 RecipeLibrary + exec `MFCraft <RecipeID>` | `MFCraft Recipe_HealSnack` 扣料产出食物 |
| 3 | 消耗品使用（对全体召唤宠 ApplyGE）+ `UMFCraftingWidget` 合成页 + 背包嵌入 | 合成→喂宠→宠物回血/增伤 |
| 4 | （编辑器）DT 配方 + ItemDatabase 消耗品条目 + GE_Heal/GE_AttackUp | 见验收 |

**验收标准**：采集够料后打开背包合成页 → 料够的配方高亮 → 合成"治疗零食"扣料并入包 → 点击使用 → 场上召唤宠回血；"力量零食"→ 宠物临时增伤（时限后消失）；料不够时配方置灰不可合成。

## 7. 依赖与排期

- **依赖 A1.5 背包先完成**（合成扣料/产出都走背包接口）→ 打造排 W2，背包 W1，顺序正确。
- **任务系统（A1.7）移出月底测试版**，推迟到测试后与 B 组元层一起做完整 MVP——首轮测试用打造循环 + Boss 作目标，暂不需要任务引导（可辅以口头/文字引导）。
