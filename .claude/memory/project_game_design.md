---
name: ProjectMF Game Design Overview
description: Core game concept, features, and development roadmap for ProjectMF survival-pet game
type: project
originSessionId: e43d0336-4bdb-48ad-8901-810ebcb3f7cd
---
# ProjectMF — 类饥荒+帕鲁宠物养成游戏

## 核心概念
- 2D角色（PaperZD）in 3D场景（UE5）
- 玩法类似帕鲁：生存建造 + 抓宠
- 宠物深度养成类似梦幻西游：品质、洗属性、升级、技能、装备、染色、变异、幻化
- 玩家自身成长集中在工具类（采集速度、建造速度等）
- 游玩循环：出城探索/抓宠 → 采集资源 → 回城养宠/建造 → 探索更远地图

## 当前已完成（截至2026-04）
- AMFCharacterBase：PaperZD billboard渲染（2D精灵in 3D）
- AMFCharacter：玩家角色、相机弹簧臂、相机旋转（Q/E）、移动、Pick交互桩
- AMFAICharacter：AI角色基类，Mass Entity接口框架
- MFCamera/MFPlayerController/MFPlayerAnimInstance：已完成
- UE5 Enhanced Input系统接入

## 技术栈
- Unreal Engine 5
- Paper2D + PaperZD（2D角色动画）
- Mass Entity System（大规模AI）
- Enhanced Input System

## 里程碑（2026-05调整版）
- M1 (2026-05): **验证循环** — 3分钟抓宠→Boss战→胜负判定，5种Pet+1Boss+必要UI
- M2 (2026-07): 资源采集 + 背包
- M3 (2026-09): 宠物深度养成——升级/技能/洗属性/装备
- M4 (2026-10): 宠物外观系统——染色/变异/幻化
- M5 (2026-11): 生存机制 + 玩家成长
- M6 (2027-01): 建造系统
- M7 (2027-02): 世界扩展/多区域
- Beta (2027-Q2)

**调整原因：** M1 目标从资源采集改为验证循环，优先验证宠物抓取+战斗的核心手感；资源采集/背包移至M2，不影响战斗闭环验证。

## 近期优先级（M1验证循环）
1. GE_Damage + UMFPetAttributeSet（战斗数值基础）
2. AI StateTree 4状态（游荡/跟随/战斗/逃跑）
3. GA_PetAttack + GA_BossAttack（依赖①②）
4. Boss Actor Blueprint（5种Pet+1Boss外观/数值）
5. 死亡系统 + 血条UI + 胜负判定HUD

**Why:** M1目标是验证循环手感，资源/背包/养成系统等M2+内容暂不介入。
**How to apply:** 排期新功能时以"是否推进抓宠→Boss战→胜负判定循环"为优先判断标准。
