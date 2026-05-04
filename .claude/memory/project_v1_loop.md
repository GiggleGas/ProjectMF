---
name: 一阶段验证循环规范
description: 第一个可玩验证循环的完整体验需求 + 与当前代码的缺口分析（2026-04基线）
type: project
originSessionId: a95f5ef2-0163-44ce-937c-34b8603b555d
---
# 一阶段验证循环规范

## 循环定义

```
抓野生宠物 → 召唤出战 → 探索发现Boss → 宠物自动战斗
     ↑                                        ↓
     └─── 重新抓宠 ← 宠物全灭 ← 宠物失利逃跑 ←┘
```

**Why:** 验证核心差异化玩法——宠物养成+战斗的手感，在宠物深度养成（M3）之前先跑通最小战斗闭环。  
**How to apply:** 新功能排期时以"能否推进这个循环"为判断标准，无关循环的功能延后。

---

## 体验需求（5个阶段）

### Phase 1 — 抓宠
| # | 需求 | 当前状态 |
|---|---|---|
| 1.1 | 野生宠物在场景中漫游 | ❌ 缺 StateTree 游荡状态 |
| 1.2 | 靠近触发捕捉（3阶段QTE） | ✅ GA_CatchPet 完整 |
| 1.3 | 抓成功后进入 PetSlots | ✅ RegisterCaughtPet 完整 |
| 1.4 | 1-5键召唤宠物到玩家附近 | ✅ GA_SummonPet 完整 |
| 1.5 | 召唤后宠物持续跟随玩家 | ❌ 缺 StateTree 跟随状态 |

### Phase 2 — 探索发现Boss
| # | 需求 | 当前状态 |
|---|---|---|
| 2.1 | Boss Actor存在于地图（体型/外观有别于野生宠物） | ❌ 完全缺失 |
| 2.2 | Boss感知玩家/宠物后切换战斗AI | ❌ 缺 AIPerception + 战斗状态切换 |

### Phase 3 — 战斗
| # | 需求 | 当前状态 |
|---|---|---|
| 3.1 | 宠物自动检测Boss并攻击 | ❌ 缺 GA_PetAttack + 战斗 StateTree |
| 3.2 | Boss自动反击宠物/玩家 | ❌ 缺 GA_BossAttack 或 BT攻击节点 |
| 3.3 | 伤害数值流通（HP双向扣减） | ❌ 缺 GE_Damage + UMFPetAttributeSet |
| 3.4 | 血条可见（宠物+Boss头顶） | ❌ 缺 HP Widget |

### Phase 4 — 失利与逃跑
| # | 需求 | 当前状态 |
|---|---|---|
| 4.1 | 宠物HP低于阈值时脱离战斗跑向玩家 | ❌ 缺 StateTree 逃跑状态 + HP阈值条件 |
| 4.2 | 宠物HP归零时死亡（Actor销毁+实例标记） | ❌ 缺死亡GE + bIsDead 字段 |
| 4.3 | 所有出战宠物死亡时有明确UI提示 | ❌ 缺全灭事件监听 + HUD提示 |

### Phase 5 — 循环回到抓宠
| # | 需求 | 当前状态 |
|---|---|---|
| 5.1 | 死亡宠物从出战名单移除，不可再召唤 | ❌ FMFPetInstance 需加 bIsDead 字段 |
| 5.2 | 地图有足量野生宠物可继续捕捉 | ❌ 需摆放足量野生宠物BP到场景 |

---

## 已完成模块（可直接复用）

| 模块 | 文件 | 说明 |
|---|---|---|
| 玩家移动/相机 | `MFCharacter.cpp` | 完整，含Q/E旋转 |
| GAS框架 | `MFAttributeSetBase`、`MFGameplayAbilityBase` | HP/MaxHP/MoveSpeed已定义 |
| 背包数据层 | `MFInventoryComponent`、`MFItemDatabase` | 资源格子+宠物名单完整 |
| 宠物数据结构 | `FMFPetInstance`（序列化/还原） | 完整，需加 bIsDead |
| 抓宠流程 | `GA_CatchPet`、`AT_WaitPetTarget`、`AT_MoveBall` | 三阶段完整 |
| 召唤/召回 | `GA_SummonPet`、`InventoryComponent::SummonPet` | NavMesh定位+召唤/召回完整 |
| AI基类 | `AMFAICharacter`、`AMFPetBase` | Mass接口+IMFCatchable完整 |

---

## 实现优先级（依赖顺序）

```
① GE_Damage + UMFPetAttributeSet       ← 所有战斗的基础，先做
② AI StateTree（4状态）                ← 游荡/跟随/战斗/逃跑
③ GA_PetAttack + GA_BossAttack         ← 依赖①②
④ Boss Actor Blueprint                 ← 依赖①③
⑤ 死亡系统（GE_Death + bIsDead）       ← 依赖①
⑥ 血条UI + 全灭提示                    ← 依赖①⑤
⑦ 野生宠物BP摆场景 + Boss摆位          ← 收尾
```

### StateTree需要的4个状态
- **Wander**：野生宠物无目标时随机游荡
- **Follow**：召唤后跟随玩家（距离保持）
- **Combat**：检测到敌人，移动到攻击范围内并触发 GA_PetAttack
- **Flee**：HP < FleeThreshold（如30%），脱离战斗跑向玩家

### UMFPetAttributeSet 需要的字段
- `Attack`（攻击力，用于 GE_Damage 计算）
- `Defense`（防御力，用于伤害减免）
- `FleeThreshold`（逃跑HP百分比阈值，默认0.3）

### FMFPetInstance 需要的新字段
```cpp
UPROPERTY(BlueprintReadOnly, Category = "Pet")
bool bIsDead = false;  // 死亡后不可召唤，循环时需要去抓新宠物
```

---

## 不在此循环范围内的功能（暂不做）

- 资源采集（M1）：验证循环不依赖资源，延后不影响
- 宠物深度养成（M3）：品质/洗属性/升级
- 建造系统（M6）
- 背包/物品UI
- QTE视觉反馈（抓宠时的UI）

这些在循环验证手感通过后再介入。
