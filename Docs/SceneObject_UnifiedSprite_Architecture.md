# 场景物体统一构建（2D Sprite 表现层）架构方案

> 2026-07-10 立项 ｜ 状态：方案待评审，未改代码
> 命题（2026-07-10 用户）：**以后所有场景中的物体都通过统一系统构建**（承接"全 sprite、废弃 ISM"方向）。
> 定稿理解：不是"一个 subsystem 装万物"，而是**统一的 2D sprite 表现层规范 + 一份共享表现能力代码，宿主按「是否有独立 gameplay / 是否海量」分两类**。

---

## 1. 核心判断：统一"表现层"，不是统一"管理器"

「所有物体走同一个 subsystem」是错的——角色/宠/Boss 有 GAS/AI/碰撞/交互/PaperZD 状态机，**必须是独立 Actor**，塞进池会毁掉这些。投射物 subsystem 只适合"海量、无独立逻辑的飞行视觉"。

正确的统一是**两个维度**：
1. **表现统一**：所有场景物体都是 2D sprite，共享同一套 billboard / 闪光 / 碰撞自适应 / 朝向逻辑（**一份代码**，不再各写各的）。
2. **宿主分层**：按「有无独立 gameplay」「是否海量」选宿主——重实体走 Actor，海量视觉走 subsystem 池。

## 2. 现状盘点（统一的起点）

| 物体 | 当前基类 / 载体 | 表现能力现状 |
|---|---|---|
| 玩家 / AI / 宠 / Boss | `AMFCharacterBase`（Flipbook+PaperZD） | **全**：billboard、受击/治疗闪光、碰撞自适应、方向性动画 |
| 掉落物 | `AMFSceneActorBase` + 自加 `PaperSpriteComponent` | 自己重造了 billboard + 抛物线 |
| 采集节点 | `AMFSceneActorBase` | 基类太薄，几乎裸 |
| 区域视觉（火圈/回血场） | `AMFSceneActorBase`（AreaEffectSubsystem 生成） | 基本够用 |
| 投射物 / 弹幕 / 落石 | 投射物 subsystem（ISM，将改 sprite 池） | 无 billboard（mesh） |
| **捕捉球** | `AMFCatchBallActor`（**StaticMesh**） | **异类**，未 sprite 化 |

**问题**：表现能力（billboard/闪光/碰撞自适应）只在 `AMFCharacterBase` 里，`AMFSceneActorBase` 享受不到 → 掉落物重造轮子；sprite 载体 `PaperSprite`/`PaperFlipbook` 混用；捕捉球还是 mesh。

## 3. 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│ 第0层  载体规范：统一 PaperFlipbook（单帧 flipbook = 静态图）  │
├─────────────────────────────────────────────────────────────┤
│ 第1层  UMFSpriteVisualComponent（新）——共享表现能力，一份代码 │
│        billboard｜受击/治疗闪光｜碰撞自适应｜朝向/方向性        │
├──────────────────────────────┬──────────────────────────────┤
│ 第2层-A  实体宿主（Actor）    │ 第2层-B  海量视觉宿主（池）    │
│  有 GAS/AI/交互，非海量        │  无独立逻辑，可能成百上千      │
│  · CharacterBase（玩家/宠/怪） │  · 投射物 subsystem（sprite 池）│
│  · SceneActorBase（掉落/采集/  │    弹幕/投射物/投掷物/落石     │
│    捕捉球/宝箱/区域视觉）       │                              │
│  ——都挂第1层组件               │  ——池内联第1层能力的轻量版     │
└──────────────────────────────┴──────────────────────────────┘
```

### 3.0 载体规范：统一 `PaperFlipbook`

淘汰散用的 `PaperSpriteComponent`，统一到 `PaperFlipbookComponent`：**单帧 flipbook 就是静态 sprite**，零额外成本，换来 billboard/闪光/朝向逻辑只写一份、掉落物和角色走同一条渲染路。（投射物 sprite 池也用 flipbook，将来弹药想加飞行动画/旋转直接配。）

### 3.1 第1层：`UMFSpriteVisualComponent`（把表现能力从 CharacterBase 抽出来）

现在这些散在 `AMFCharacterBase`（+ 掉落物重造）里，抽成一个可挂任意 Actor 的组件：

```cpp
UCLASS(ClassGroup=MF, meta=(BlueprintSpawnableComponent))
class UMFSpriteVisualComponent : public UActorComponent
{
    // 每帧让目标 flipbook 面向相机（原 CharacterBase::UpdateBillboard）
    void TickBillboard();
    // 受击闪红 / 治疗闪绿（原 FlashSpriteColor + 定时复位）
    void FlashColor(const FLinearColor& Color, float Duration);
    // 从 flipbook 首帧尺寸自适应碰撞（原 UpdateCollisionFromFlipbook）
    void FitCollisionToFlipbook(float Scale);

    // 相机朝向来源：通过委托/接口注入，解耦"谁提供相机"
    //   玩家 → CameraComponent 前向；AI/场景物 → PlayerCameraManager
    TFunction<bool(FVector&)> CameraForwardProvider;

    UPROPERTY(EditAnywhere) float BillboardYawOffset = -90.f;
    UPROPERTY() TObjectPtr<UPaperFlipbookComponent> Target;
};
```

- `AMFCharacterBase` / `AMFSceneActorBase` 都改成"持有并驱动这个组件"，删掉各自重复的 billboard/闪光/碰撞代码 → **一份实现，全场景共享**。
- 相机朝向来源用委托注入：不同宿主提供不同相机源，组件本身不关心。

### 3.2 第2层-A：实体宿主（Actor + 第1层组件）

有独立 gameplay 的都留 Actor，只是表现走第1层组件：

- **角色系** `AMFCharacterBase`：玩家/AI/宠/Boss。原有 GAS/AI/移动不动，billboard/闪光交给组件。
- **场景交互物** `AMFSceneActorBase` **补齐**：把它从"太薄"升级为"持第1层组件 + 可选碰撞/交互"。掉落物删掉自造 billboard、改用组件；采集节点、宝箱同理。
- **捕捉球** `AMFCatchBallActor`：从 StaticMesh **迁到 `AMFSceneActorBase`**，抛掷弧线复用掉落物那套（sprite + 抛物线）。

### 3.3 第2层-B：海量视觉宿主（投射物 sprite 池）

承接已定稿的《纯 Sprite 投射物子系统》方案：subsystem + `PaperFlipbookComponent` 池 + 共享 tick + 弹道矢量化。弹幕/投射物/投掷物/落石走这里，**不做 Actor**。池内联第1层能力的轻量版（billboard 直接在池 tick 里算，不挂组件——海量下省组件开销）。

## 4. 决策规则（新增任何场景物体时查这张表）

| 物体举例 | 有 GAS/AI/交互? | 可能海量(≥百)? | 宿主 |
|---|---|---|---|
| 玩家 / 宠 / 怪 / Boss | 是 | 否 | Actor · `AMFCharacterBase` + 组件 |
| 掉落物 / 采集节点 / 捕捉球 / 宝箱 / 传送点 | 交互，无 AI | 否 | Actor · `AMFSceneActorBase` + 组件 |
| 区域视觉（火圈 / 回血场） | 否 | 中 | AreaEffectSubsystem（现有，走 SceneActorBase） |
| 投射物 / 弹幕 / 落石 / 玩家投掷物 | 否 | 是 | 投射物 subsystem（sprite 池） |

一句话规则：**有独立逻辑 → Actor；纯海量飞行视觉 → 池；两者都挂/复用同一套 sprite 表现能力。**

## 5. 实施计划（2026-07-10 定稿：现在整体重构，连续推进）

用户拍板**现在一次性推进**（非渐进穿插）。阶段间的依赖顺序不变——仍按下表先后连续做完，每步做完自检回归，不分散到后续迭代。

| 步 | 内容 | 依赖 / 回归重点 |
|---|---|---|
| **S1 表现组件** | 抽 `UMFSpriteVisualComponent`（billboard / 闪光 / 碰撞自适应 / 朝向，相机源用委托注入） | 无依赖，先建。单元自测 billboard 数学与原 `CharacterBase` 一致 |
| **S2 角色系接入** | `AMFCharacterBase` 改为持有并驱动组件，删自身重复的 billboard/闪光/碰撞代码 | 依赖 S1。**回归**：玩家/宠/怪 表现、受击闪红、治疗闪绿、碰撞尺寸全不变 |
| **S3 场景物接入** | `AMFSceneActorBase` 升级为持组件；掉落物删自造 billboard/抛物线改用组件；采集节点接入 | 依赖 S1。**回归**：掉落物散开抛物线 + billboard 不变 |
| **S4 投射物 sprite 池** | 投射物 subsystem ISM→`PaperFlipbook` 池 + 弹道矢量化；三类远程攻击（投掷/落石/弹幕）迁 sprite；池内联组件的 billboard 轻量版 | 依赖 S1（复用 billboard 算法）。**回归**：现有 AI 远程攻击行为不变 |
| **S5 玩家投掷消耗品** | 接《纯 Sprite 投射物子系统》§4：`UMFThrowableData` + `FMFItemDef` 换 `ThrowableData` + 抛物线投掷 + 落点回血场 | 依赖 S4 |
| **S6 捕捉球去 mesh** | `AMFCatchBallActor` StaticMesh → `AMFSceneActorBase`（sprite + 抛物线 + 组件） | 依赖 S1、S3。**回归**：抓宠投掷/命中/收球流程不变 |
| **S7 载体收尾** | 全项目残留 `PaperSpriteComponent` 收敛到 `PaperFlipbook`（单帧）；清干净 mesh/ISM 死代码 | 最后做，影响面收敛 |

> **工期提示（需知情）**：S1–S7 是纯架构/技术债重构，本身不直接产出月底玩家测试版内容，会占用当前 W3「大地图 + 整合」的时间（见 `PlayerTest_July_Plan.md`）。收益是此后所有场景物体开发都在统一地基上、不再重造。若测试版工期吃紧，可考虑先做 S4+S5（当前投掷需求，本就要做），S1–S3、S6–S7 的纯重构压到测试后——但用户已选"现在整体推进"，此处仅作风险标注。

## 6. 优缺点 / 风险

**优点**
- 表现能力一份代码：billboard/闪光/碰撞不再散在多处、掉落物不再重造。
- 全场景 sprite，风格统一；新物体照决策表选宿主，不再拍脑袋。
- 与"全 sprite 投射物"方向一致，投掷/弹幕/掉落/角色共享表现层。

**缺点 / 代价**
- 抽组件要动 `AMFCharacterBase`（最核心的类），P1 回归面大。
- 海量视觉（池）与实体（Actor）两套宿主并存——是**必要的**分层，但读代码要理解"为什么不统一成一个"。

**风险点**
1. **P1 重构 `AMFCharacterBase`**（最高）：billboard/闪光/碰撞是玩家和所有 AI 共用的表现，抽错会全场景表现异常。→ 纯行为不变重构，抽完逐一对比玩家/宠/怪表现。
2. **相机源解耦**：组件要同时服务"玩家相机"和"AI 用 PlayerCameraManager"，委托注入没接好会 billboard 朝向错乱。→ P1 先只接玩家、回归通过再接 AI。
3. **载体切换（P4）**：`PaperSprite`→`PaperFlipbook` 单帧，个别资产 pivot/尺寸可能要重设。→ 放最后、影响面最小时做。

## 7. 决策点（2026-07-10 已拍板）

| 点 | 决策 |
|---|---|
| 载体 | ✅ **统一 `PaperFlipbook`**（单帧=静态，一套代码） |
| 迁移节奏 | ✅ **现在整体重构，连续推进**（S1–S7，见 §5） |
| 表现共享方式 | ✅ **抽独立组件 `UMFSpriteVisualComponent`**（组合，非继承合并） |
