# 掉落物系统（Loot）设计方案

> 2026-07-02 ｜ Loop2 MVP 任务 A1 ｜ 状态：**C++ 已实现并编译通过（§6 步骤 1–4）**；剩 §3.7 编辑器内容配置（BP_LootPickup / ItemDatabase 条目 / LT 掉落表资产）
> 范围：通用掉落表 + 怪物死亡掉落 + 采集点产出 + 地面掉落物拾取进背包。
> 不含（属暂缓/其他任务）：资源来源标记（见 §5.4）、派宠劳作采集、局外产出、任务系统交互底层（A2）。

---

## 1. 现状对接点（已核实）

| 现有资产 | 位置 | 对接方式 |
|---|---|---|
| `UMFItemDatabase` + `FMFItemDef` | `Inventory/` | 物品定义已就绪，ItemID 规范 `Item.Resource.*`；掉落表只引用 ItemID，不重复定义物品 |
| `UMFInventoryComponent::AddResource(ItemID, Count)` | `Inventory/` | 拾取入包的唯一入口，返回实际添加数（背包满时部分添加） |
| `UMFAttributeSetBase::OnDeath` → `AMFCharacterBase::HandleDeath()`（虚函数） | `Character/` | 死亡掉落挂点；`AMFAICharacter` 目前**未 override**，正好新增 |
| `UMFAIConfig`（UPrimaryDataAsset，per AI 类型） | `AI/` | 掉落表挂这里，同一 Config 复用的 AI 共享掉落 |
| `AMFSceneActorBase` | `Scene/` | 注释已预留"树木/矿石等可采集物派生子类追加碰撞与交互（TODO）"——采集点落地这个 TODO |
| `GA_Pick`（按住→`State.Picking` tag，无产出逻辑） | `GAS/` | 采集读条复用此壳，补产出 |
| `AMFSpawnAIManager` 散点摆放 | `AI/` | 采集点摆放沿用（A3 任务） |
| 阵营体系（`MF.Team.*` loose tags + `AreHostile`） | — | 用于"玩家侧宠物死亡不掉落"过滤 |

单机项目，不做网络复制。

---

## 2. 架构总览

```
                         ┌─ 怪物死亡: AMFAICharacter::HandleDeath (override)
  UMFLootTable ◄─ 配置 ──┤
  (DataAsset)            └─ 采集点:   AMFGatherNode (SceneActorBase 派生)
       │
       ▼ RollTable(Table) → TArray<FMFLootResult>
  UMFLootSubsystem (UWorldSubsystem)
       │ SpawnLoot(Results, Location)
       ▼
  AMFLootPickup ×N (地面掉落物, 散开落地)
       │ overlap → 吸附飞向玩家 → AddResource()
       ▼
  UMFInventoryComponent.ResourceSlots → OnInventoryChanged → UI
```

**单管线原则**：怪掉落、采集产出、（未来）任务奖励/Boss 掉落，全部走
`RollTable → SpawnLoot → 地面拾取物 → 自动吸附入包` 这一条路径。采集不做"直接进背包"分支——自动拾取让脚下产物瞬间入包，体验等价，但代码只有一条管线。

---

## 3. 组件规格

### 3.1 `FMFLootEntry` / `EMFLootRollMode` — `Loot/Public/MFLootTypes.h`（新建目录，仿 `Meta/`）

```cpp
/** 掉落表中的一条目：独立判定概率 + 数量区间。 */
USTRUCT(BlueprintType)
struct FMFLootEntry
{
    FName  ItemID;        // 引用 UMFItemDatabase，规范 Item.Resource.*
    float  Chance = 1.f;  // 0~1，每条独立 roll
    int32  CountMin = 1;
    int32  CountMax = 1;  // 命中后数量在 [Min,Max] 均匀取整
};

/** roll 结果（ItemID + 实际数量），Subsystem 输出、SpawnLoot 输入。 */
USTRUCT(BlueprintType)
struct FMFLootResult { FName ItemID; int32 Count; };
```

MVP 只做**每条独立判定**（饥荒式：肉 100%×2、皮 50%×1）。互斥权重组（N 选 1）留枚举扩展位 `EMFLootRollMode`，本期不实现。

### 3.2 `UMFLootTable` — `Loot/Public/MFLootTable.h`

```cpp
UCLASS(BlueprintType)
class UMFLootTable : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) TArray<FMFLootEntry> Entries;
};
```

命名规范：`LT_SlimeCat` / `LT_Tree_Oak` / `LT_Boss_BonePlain`。刻意保持薄——嵌套表、条件掉落（首杀/时段）都不做，等有真实需求再加。

### 3.3 `AMFLootPickup` — `Loot/Public/MFLootPickup.h`，派生自 `AMFSceneActorBase`

地面掉落物 Actor，一个实例 = 一种 ItemID × Count。

- **组件**：基类 Flipbook（视觉，BP 里配通用"掉落袋"动画或按物品换 Sprite=Icon）+ 新增 `USphereComponent`（拾取感应，默认半径 ~80cm，OverlapOnlyPawn）。
- **状态机**：`Dropping`（出生小抛物线散开，~0.4s，纯表现）→ `Idle`（待拾取）→ `Magnet`（玩家进入感应半径且背包可收 → 插值飞向玩家，~0.3s 加速）→ 到达 → `AddResource()` → 按实际添加数处理：
  - 全部加入 → `Destroy()`。
  - 部分/零加入（背包满）→ Count 扣除已加部分，回落原地进入 1s 冷却再允许吸附（防每帧重试）。
- **配置字段**：`ItemID`、`Count`、`MagnetRadius`、`MagnetDuration`、`Lifetime`（默认 300s 自动消失，0=永久；防止满地remains）。
- **触发者限定**：MVP 仅玩家 Pawn 触发吸附（`AMFCharacter` 判定）。宠物拾取（劳作）留扩展位。

### 3.4 `UMFLootSubsystem` — `Loot/Public/MFLootSubsystem.h`，`UWorldSubsystem`

```cpp
/** roll 一张表（纯函数，可单测）。 */
TArray<FMFLootResult> RollTable(const UMFLootTable* Table) const;

/** 在 Location 生成掉落物：每个 Result 一个 Pickup，落点在 ScatterRadius 内随机散开。 */
void SpawnLoot(const TArray<FMFLootResult>& Results, const FVector& Location);

/** 组合便捷入口：Roll + Spawn。死亡/采集挂点只调这一个。 */
void DropFromTable(const UMFLootTable* Table, const FVector& Location);
```

- 同 ItemID 的多个数量默认合并成一个 Pickup（减少 Actor 数）；`ScatterRadius`（默认 ~100cm）内随机偏移落点。
- Pickup 类、散开半径等默认值放 `UMFLootSettings : UDeveloperSettings(config=Game)`（沿用 `UMFTelegraphSettings` 模式），BP 可在 Settings 里换 Pickup 蓝图类。
- **调试 exec**（仿 `MFKillNextPet` 模式）：
  - `MFSpawnLoot <ItemID> <Count>` — 脚下生成掉落物；
  - `MFDropTable <TableAssetName>` — 脚下按表 roll 一次（验概率）。

### 3.5 怪物死亡掉落挂线

- `UMFAIConfig` 新增字段：`UPROPERTY(EditDefaultsOnly, Category="Loot") TObjectPtr<UMFLootTable> LootTable;`（空 = 不掉落，纯增量）。
- `AMFAICharacter` **override `HandleDeath()`**：`Super::HandleDeath()` 后，若 `AIConfig->LootTable` 有效 → `LootSubsystem->DropFromTable(Table, GetActorLocation())`。
- **过滤规则**（防误掉）：
  - ASC 带 `MF.Team.Player` 阵营 tag（玩家召唤宠）→ 不掉落。召唤宠走 `AMFPetBase` 濒死→真死链，不经过野怪死亡表现，但 `HandleDeath` 基类挂点是共享的，显式过滤兜底。
  - 被捕捉的野宠：`GA_CatchPet` 直接销毁 Actor 不走 `OnDeath` → 天然不掉落，无需处理（已确认）。
- 死亡即掉（`HandleDeath` 时机），不等尸体销毁——与现有死亡表现（StateTree 播死亡动画）互不干扰。

### 3.6 `AMFGatherNode` 采集点 — `Loot/Public/MFGatherNode.h`，派生自 `AMFSceneActorBase`

落地 SceneActorBase 注释里的 TODO：

- **组件**：基类 Flipbook（树/矿石外观，可按剩余次数换阶段动画）+ `UBoxComponent`（阻挡碰撞）+ 交互范围检测。
- **字段**：`LootTable`（每次采集 roll 一次）、`MaxHarvestCount`（耗尽次数，如树×3）、`HarvestDuration`（单次读条秒数，如 2s）、`bDestroyOnDepleted`（耗尽销毁 or 留残桩）。
- **交互流程（MVP 最简，不等 A2 交互底层）**：
  1. 玩家在范围内按住采集键 → 现有输入链激活 `GA_Pick`（不改输入）。
  2. Node 自身 Tick 检测：范围内存在带 `MF.GameplayState.Picking` tag 的玩家 → 累计进度；松开（tag 消失）进度清零（或保留，可配）。
  3. 进度满 `HarvestDuration` → `DropFromTable(LootTable, 自身位置)` → 掉落物落脚下被自动吸走 → `MaxHarvestCount--`，耗尽后销毁/换残桩外观。
- 检测方向选 **Node 检测玩家 tag** 而非改 GA_Pick 加目标逻辑：GA_Pick 保持"动画状态壳"职责不变，零改动；将来 A2 交互底层或派宠劳作接管时，只换"谁在触发 Gather"，Node 接口不动。为未来预留 `Gather(AActor* Instigator)` 公开入口。

### 3.7 内容配置（验证用）

- `DA_ItemDatabase` 补 3~4 个测试资源：`Item.Resource.Meat / Wood / Stone / SlimeGel`（Icon 可先占位）。
- 掉落表 3 张：`LT_TestPet`（肉100%×1-2 + 胶50%×1）、`LT_Tree`（木100%×2-3）、`LT_Rock`（石100%×1-2）。
- 测试宠/怪的 `UMFAIConfig` 挂 `LT_TestPet`；测试图摆 2 个 `AMFGatherNode`。

---

## 4. 顺手补的前置（纯增量）

- `FMFInventorySlot::ItemID / Count` 补 `SaveGame` 修饰符——B1（元层 P1 存档）序列化 ResourceSlots 的前置，与 P0 给 `FMFPetInstance` 补 SaveGame 同理，本期一并做掉。

## 5. 明确不做 / 推迟

1. **互斥权重组 / 嵌套表 / 条件掉落**：留 `EMFLootRollMode` 枚举位，MVP 只有独立判定。
2. **ISM/Niagara 轻量化渲染**（仿 ProjectileSubsystem）：MVP 掉落物同屏几十个，Actor 方案足够；数量成为瓶颈时再迁移，Subsystem API 不变。
3. **宠物/劳作拾取**：`Gather(Instigator)` 与吸附触发者判定留扩展位，S3 劳作讨论后接入。
4. **资源来源标记（局内新获 vs 带入）**：不在背包格加字段。结算需要的"来源审计"由元层入局时快照 diff 实现（P5 `ReconcileExtraction` 职责）——掉落系统对此零感知，避免为待讨论的规则提前造结构。

---

## 6. 实现顺序（5 步，每步可独立提交/验证）

| 步 | 内容 | 验证方式 |
|---|---|---|
| 1 | `Loot/` 目录 + Types + `UMFLootTable` + `UMFAIConfig.LootTable` 字段 + `FMFInventorySlot` 补 SaveGame | 编译过，编辑器能建 LT 资产 |
| 2 | `UMFLootSubsystem`（Roll+Spawn）+ `AMFLootPickup`（散开/吸附/入包/背包满回落）+ `UMFLootSettings` + exec | `MFSpawnLoot Item.Resource.Meat 3` → 捡起 → 背包 UI +3 |
| 3 | `AMFAICharacter::HandleDeath` override + 阵营过滤 | 打死测试宠掉肉；玩家召唤宠濒死→真死不掉 |
| 4 | `AMFGatherNode` + GA_Pick tag 检测读条 | 按住采集键 2s，树掉木头×3 次后耗尽 |
| 5 | 内容配置（§3.7）+ 概率验证（`MFDropTable` 刷 100 次看分布） | 全链路：进图→打怪/采集→背包累积 |

**验收标准**：打死一只野宠掉 1-2 块肉自动吸入背包；一棵树可采 3 次每次掉木头；背包满时掉落物留在地上；玩家召唤宠死亡不产生任何掉落。
