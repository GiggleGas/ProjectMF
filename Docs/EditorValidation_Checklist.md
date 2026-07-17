# 编辑器验证清单（操作版）

> 2026-07-13 ｜ 累积待验：flipbook 基底(S1-S7) + 背包(W1) + 打造(W2) + 投掷回血(S5)。
> 每项 = **验证什么 · 怎么操作 · 预期**。控制台命令用 ` 键打开输入。按阶段顺序（有依赖）。

**常用控制台命令速查**：
| 命令 | 作用 |
|---|---|
| `MFSpawnLoot <ItemID> <数量>` | 脚下生成掉落物 |
| `MFDropTable <掉落表名>` | 脚下 roll 一次掉落表 |
| `MFCraft <配方ID>` | 合成一个配方 |
| `MFUseItem <ItemID>` | 使用一个消耗品 |
| `MFKillNextPet` | 击杀一只出战宠（测死亡流程） |
| `MFSVFlash` / `MFSVCollision` / `MFSVBillboard` | S1 表现组件单测 |
| `MF.SpriteVisual.Debug 1` | billboard 洋红箭头 + 碰撞绿球 |
| `mf.debug.projectile 1` | 投射物画青球 + 数量 |
| `MF.Char.CharacterBaseDebug 1` | 角色碰撞球/朝向箭头 |

---

## 阶段 0 · 前提配置（先配，否则全看不见）

| 配什么 | 怎么操作 |
|---|---|
| DT_Item 每行 `WorldFlipbook` | 打开 DT_Item，逐行给 `WorldFlipbook` 拖入 flipbook 资产（静态图=单帧 flipbook） |
| 三类攻击 `ProjectileFlipbook` | 打开投掷/落石/弹幕的 DataAsset，`ProjectileFlipbook` 填 flipbook |
| BP_CatchBall flipbook | 打开 BP_CatchBall，选基类 `FlipbookComponent`，Details 里配球的 flipbook |
| MF Item Settings | Project Settings → 搜 "MF Item"，`ItemDatabase` 指 DT_Item |

---

## 阶段 1 · flipbook 回归（最优先，动了核心路径）

| 验证项 | 怎么操作 | 预期 |
|---|---|---|
| 角色 billboard | 进 PIE，WASD 走 + 转镜头 | 玩家/宠/怪 sprite 始终正面朝相机、随镜头转 |
| 受击闪红 | `MFKillNextPet`（掉血过程）或让怪打宠 | 挨打瞬间 sprite 闪红→复位 |
| 闪光组件单测 | 控制台 `MFSVFlash` | 主控角色闪蓝 0.5s 自动复位 |
| 治疗闪绿 | 先配好投掷回血（阶段3）再回来，或 GE 喂血 | 回血瞬间闪绿 |
| 碰撞尺寸 | `MF.Char.CharacterBaseDebug 1` 看绿球；再 `MFSVCollision` 看 log | 绿球贴合 sprite；`FitCollision: Radius=` 与启动时 log 的值一致 |
| billboard 组件对齐 | `MF.SpriteVisual.Debug 1` | 角色头顶洋红箭头指向相机，角色朝向不变 |
| 掉落物可见+散开 | `MFSpawnLoot <ItemID> 3` | 掉落物出现、抛物线弹开、面向相机 |
| 掉落物 billboard 观感 | 观察掉落物倾斜 | ⚠️ 现在是「和角色一样的统一倾斜」(不是各自朝向)——**预期变化** |
| 采集物 billboard | 看场景里的树/矿 | 现在也面向相机（以前不会）——树若该正面不转，记下来告诉我 |
| 投射物飞行 | `mf.debug.projectile 1` + 让 AI 远程攻击（进有怪场景/召唤宠对打） | flipbook 面向相机飞、青球同步、命中/超距消失 |
| 落石/弹幕 | 同上，观察落石/弹幕类攻击 | 落石从上方落、弹幕扇形多发，行为同以前 |
| 捕捉球 | 用抓宠键对野宠抓一次 | 球 flipbook 面向相机飞出、命中/收球流程照常 |

---

## 阶段 2 · 背包（W1）

| 验证项 | 怎么操作 | 预期 |
|---|---|---|
| 拾取进包 | `MFSpawnLoot <ItemID> 5` → 走过去按拾取键（空格就近拾取） | 掉落物进背包，格子显图标+数量 |
| 堆叠 | 多次 `MFSpawnLoot` 同一 ItemID 后拾取 | 同种堆叠，超上限开新格 |
| 拖动丢弃 | 打开背包 UI，鼠标拖一个格子往外拖 | 该格物品丢回地面（重新抛一次） |
| 满包弹回 | 把背包塞满后再拾取 | 装不下的部分弹回地面 |
| 鼠标可用 | 进 PIE 看光标 | 鼠标常显、能点格子 |

出问题看 `LogMFInventory`。

---

## 阶段 3 · 打造 + 投掷回血（W2 + S5）

**先配置**：
| 配什么 | 要点 |
|---|---|
| DT_RecipeLibrary | 配方 Recipe_HealSnack 等，Inputs/Output 用 FMFItemKey 下拉选 |
| GE_Heal（蓝图 GE） | ⚠️ **固定值 Modifier 改 `Healing` 元属性**（如 +50），**别用 SetByCaller** |
| DA_HealField（UMFAreaEffectData） | `TargetFilter=AllyOnly` + `Effects=[{Kind=Heal, Magnitude=X}]` + Radius/Duration/TickInterval + 绿圈 VisualActorClass |
| DA_HealSnackThrow（UMFThrowableData） | `FlightFlipbook` + Speed/MaxRange/ArcHeight + `ImpactArea=DA_HealField` |
| DT_Item 治疗零食行 | `ItemType=Consumable`，`ThrowableData=DA_HealSnackThrow` |
| MF Crafting Settings | `RecipeLibrary` 指 DT_RecipeLibrary |

**再验证**：
| 验证项 | 怎么操作 | 预期 |
|---|---|---|
| 合成扣料产出 | 先备好料，`MFCraft Recipe_HealSnack`（或背包合成页点） | 扣料 + 治疗零食入包 |
| 料不够置灰 | 清空料后看合成页 | 该配方置灰不可点 |
| 进投掷瞄准 | 双击背包里治疗零食格子 | log「投掷瞄准」，进瞄准态 |
| 抛物线投掷 | 瞄准态下左键点地面某处 | flipbook 抛物线飞过去 → 落点生成回血场 → 范围内召唤宠回血(闪绿) → 扣 1 个 |
| 取消 | 瞄准态下右键 | 取消，不消耗 |
| 超最远距离 | 左键点很远的地方 | 落在最远处（夹取） |
| 直接喂宠(非投掷) | 配个只有 UseEffect(无 ThrowableData) 的消耗品 → `MFUseItem <ItemID>` | 直接对全体召唤宠加血 |

出问题看 `LogMFInventory`（投掷）+ `LogMFCraft`（合成）。

---

## 阶段 4 · 端到端一局

| 验证项 | 怎么操作 | 预期 |
|---|---|---|
| 完整链路 | 进图 → 打怪/采集拿料 → 拾取 → 合成治疗零食 → 双击投掷喂宠 → 宠战斗 → 触发 Boss | 全链无断点 |
| 自动铺怪 | 进图看有没有怪 | bAutoSpawn 的 SpawnAIManager 自动生成 |
| 稳定性 | 全程玩几分钟 | 无崩溃、无明显掉帧 |

---

## 验证顺序

**阶段 0 → 1 先做**（flipbook 是前提且动了核心路径，绿了地基就稳）→ 阶段 2 背包 → 阶段 3 打造/投掷 → 阶段 4 端到端。

任一项对不上，把**现象 + 相关 log**（`LogMFVisual`/`LogMFLoot`/`LogMFAbility`/`LogMFInventory`/`LogMFCraft`）贴我，我定位改代码。
