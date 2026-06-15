# ProjectMF — GAS 技能扩展 · 任务清单

> 配套设计文档：[GAS_AbilitySystem_Summary_and_Plan.md](GAS_AbilitySystem_Summary_and_Plan.md)
> 用途：跟踪扩展实现进度。完成一项把 `[ ]` 改成 `[x]`，状态列同步更新。
> 状态图例：⬜ 待办　🟡 进行中　✅ 完成　⏸️ 阻塞/挂起

总计约 **19–26 人天**。关键路径：P1 → P2 → P3（区域/Combo）。P0a 与 P0b 可并行。

---

## P0a · 地基：伤害管线（3–4d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [x] B1 | `MoveSpeed` → `MaxWalkSpeed` 同步（ASC 属性变化回调 + init 后同步一次） | `MFCharacterBase.cpp` | — | ✅ |
| [x] B2 | 加 `IncomingDamageMultiplier` / `OutgoingDamageMultiplier` 属性（默认 1，夹紧 `[0,+∞)`） | `MFCombatAttributeSet` | — | ✅ |
| [x] B3 | 加 `Healing` meta + `PostGE` 对称回复管线（复用 `OnHealthChanged`）+ 治疗闪绿（`ReactToHeal`/`FlashSpriteColor`） | `MFAttributeSetBase`、`MFCharacterBase` | — | ✅ |
| [x] B4 | 伤害公式接易伤：`max(Damage − Defense, 1) × IncomingMult` | `MFAttributeSetBase` PostGE | B2 | ✅ |
| [x] B5 | 出伤接增伤：近战+远程 `ApplyDamageToTarget` 乘 `OutgoingMult` | `GA_AIAttackBase` / `GA_AIRangedAttackBase` | B2 | ✅ |
| [x] B6 | 标签层次化 + C++ 定义：Ability 树（Player/Pet/.../Move）、`MF.GameplayState.*`（旧 State 改名 + 新 Stunned/Slowed/Blinded）、`MF.Effect.*`(9+父类)；GA 构造 SetAssetTags；redirects | `MFGameplayTags`、各 GA、`DefaultGameplayTags.ini` | — | ✅ |
| [ ] B7 | 眩晕禁动（`State.Stunned` 标签事件 → `DisableMovement`/恢复） | `MFCharacterBase.cpp` | B6 | ⬜ |
| [ ] B8 | 致盲目标失效钩子（持 `State.Blinded` 时目标获取返回空） | `MFThreatComponent` 等 | B6 | ⬜ |
| [x] B9 | 属性初始化配置化（扁平）：`FMFAttributeInitData` 取代 `DefaultInitEffect`(GE)，代码 `SetNumericAttributeBase` 直接 init | `MFAttributeInitData`(新) + `MFCharacterBase` + Player/AI Config + 2 apply 点 | — | ✅ |

## P0b · 地基：技能基类（2d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [x] A1 | 新建 `UMFPlayerGameplayAbility` / `UMFPetGameplayAbility`（抽象，继承 `UMFGameplayAbilityBase`，纯骨架无门禁） | 新建 2 头文件 | — | ✅ |
| [x] A2 | reparent：Pick/Catch/Summon→Player；AIAttack/AIRanged→Pet | 5 个 GA 头文件 | A1 | ✅ |
| [ ] A3 | 通用门禁 `ActivationBlockedTags`（Dead / Stunned）入父类构造 | 两基类 .cpp | A1、B6 | ⬜ |
| [x] C0 | 新建 `UMFGameplayEffectBase`（`EffectTag` 身份标签） | 新建文件 | — | ✅ |

## P1 · GE 体系（2–3d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [ ] C1 | 10 个 GE 蓝图：Burn/Freeze/Root/Slow/Stun/Vulnerable/DamageUp/Heal/Regen/Blind（继承 `UMFGameplayEffectBase`，带 `Effect.*` 标签） | Content/GameplayEffect | P0a、P0b | ⬜ |

## P2 · 区域子系统（3–4d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [x] E0 | 抽 `UMFCombatStatics`（ApplyDamage + ApplyOnHitEffects + SpawnAreaEffect 静态入口）；近战/远程 GA 改调它 | `MFCombatStatics` | — | ✅ |
| [x] E1 | `UMFAreaEffectData`（半径/时长/间隔/过滤/DamageGE+倍率/Effects） | 新建数据资产 | P1 | ✅ |
| [x] E2 | `UMFAreaEffectSubsystem`：注册/Tick/overlap/过滤/伤害+效果/自销毁（周期重刷）+ `mf.debug.spawnarea`/`mf.debug.area` | 新建子系统 | E1 | ✅ |
| — | 设计调整：弃用 `Kind=Damage`；区域伤害走 `DamageGE+DamageMultiplier`（按来源 Attack 缩放）；区域生成统一走 `SpawnAreaEffect`（带来源） | — | — | ✅ |

## P3 · Combo 子系统（2–3d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [ ] F1 | `UMFComboSubsystem` + combo 数据表 `(TagA,TagB)→ResultGE` | 新建子系统 + DataTable | P2 | ⬜ |
| [ ] F2 | 区域子系统结算完目标效果标签后调用 Combo | `MFAreaEffectSubsystem` | F1 | ⬜ |

## P4 · 投掷区域化（1–2d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [x] D1 | `AreaOnResolve`（可选）加到**远程基类** `UMFRangedAttackDataBase`（投掷/落石通用） | `MFRangedAttackDataBase.h` | P2 | ✅ |
| [x] D2 | 远程基类加 `SpawnResolveArea(Location)`；投掷(命中/最大射程)+落石(落地)落点调它生成区域；弹幕不接 | `GA_AIRangedAttackBase`/`GA_ThrowProjectile`/`GA_FallingBoulder` | D1 | ✅ |

### 区域表现（P2 延伸，已完成）
| [x] E3 | `AMFSceneActorBase`：可播 PaperZD/Flipbook 的轻量场景 Actor 基类（无移动/GAS/碰撞，不自管寿命），供区域表现+将来树木/矿石复用 | `Scene/MFSceneActorBase` | — | ✅ |
| [x] E4 | `UMFAreaEffectData` 加 `VisualActorClass`+`VisualBaseRadius`；子系统 RegisterArea 生成视觉+按半径缩放，区域结束/Cancel 销毁 | `MFAreaEffectData`/`MFAreaEffectSubsystem` | E3 | ✅ |

## P5 · 移动技能（4–5d）

| 任务 | 描述 | 涉及 | 依赖 | 状态 |
|------|------|------|------|------|
| [ ] G0 | `UMFMovementAbilityBase`（继承 Pet 基类）+ 击退/sweep/落点区域工具 | 新建基类 | P0b | ⬜ |
| [ ] G1 | `GA_Charge` + `UMFChargeData`（突进 + 沿途命中 + 击退/控制） | 新建 | G0 | ⬜ |
| [ ] G2 | `GA_Jump` + `UMFJumpData`（抛物线跳 + 落地可选 AOE） | 新建 | G0 | ⬜ |
| [ ] G3 | `GA_GroundSlam` + `UMFGroundSlamData`（径向 AOE + 击退 + 落点注册区域） | 新建 | G0、P2 | ⬜ |

## P6 · 整合联调（2–3d）

| 任务 | 描述 | 依赖 | 状态 |
|------|------|------|------|
| [ ] H1 | AI StateTree 接线、玩家输入绑定 | P1–P5 | ⬜ |
| [ ] H2 | 数值调优、debug 可视化、回归一阶段循环 | H1 | ⬜ |

---

## 里程碑

| 里程碑 | 包含 | 周期 | 交付点 | 状态 |
|--------|------|------|--------|------|
| M1 | P0a + P0b + P1 | ~1.5 周 | 地基打通、基类重构完、10 个 GE 可单独验证 | ⬜ |
| M2 | P2 + P3 + P4 | ~2 周 | 区域+Combo 跑通、投掷落地生成区域、燃烧+缠绕触发额外 GE | ⬜ |
| M3 | P5 + P6 | ~1.5 周 | 三个移动技能可用、全面联调 | ⬜ |

## 后续迭代 TODO（本批不做）

- [ ] 同种效果叠层（如燃烧叠 N 层）
- [ ] 多元（>2）combo
- [ ] 区域表现（Decal / Niagara）
- [ ] 区域/combo 性能与数量级评估
- [ ] 门禁细则（待策划案更新后补充到 Pet/Player 基类）
- [ ] **养成 / 等级系统**：线性「基值 + 每级成长」，`DT_CharacterStats`(按 AIConfigID)，`ApplyAttributeInitData` 加 Level 入参查表现算；宠物快照改存 Level+XP。详见设计文档第四部分。
- [ ] **伤害飘字对象池**：当前 `UMFDamageNumberWidget` 每个数字 CreateWidget→RemoveFromParent，数字密集时创建/销毁 + GC 开销大，改 widget 池复用。
