# 零预算开发路线 · 免费开源资源 × 里程碑映射

> 状态：背景资料 · 最后更新：2026-10-16 · 取代关系：无（资源与里程碑映射）
> **日期**：2026-10-16
> **项目**：PersonalProject（UE 5.7 / C++ / GAS / UnLua / LAN 联机）
> **目标形态**：PvE 魂类刷怪为主，后期加联机共斗/PvP
> **范围**：把全部可用的**免费/开源**项目、插件、引擎自带能力、免费素材/服务，按里程碑嵌入完整开发流程
> **状态**：路线待评审；M1（战斗核心）已有独立 spec，本文不重复设计，只做引用与资源映射

---

## 0. 与既有文档的关系

| 文档 | 关系 |
|---|---|
| [Architecture.md](../架构/Architecture.md) | 架构宪法：五模块单向依赖、事件总线、Core 接口、UIGateway。本路线所有落地改动必须遵守 |
| [superpowers/specs/2026-10-09-soulslike-combat-core-design.md](../superpowers/specs/2026-10-09-soulslike-combat-core-design.md) | **M1 战斗内核的权威设计**（已确认，待实施）。本文 M1 章节只引用，不覆盖其决策 |

已确认、本文必须服从的 M1 决策：

1. 通用战斗组件**自研抽成 `Plugins/ActionCombatCore/` 插件**（ActionState / LockOn / InvulnerabilityWindow），不引入第三方战斗框架
2. **锁定自研**，只接管 control rotation，保留现有 SpringArm——**不引入 ALS 相机模块**
3. 翻滚动画"GASP 评估 3 天，不成立立即转 Mixamo 垫步"
4. M1 阶段**敌人 AI 不换 StateTree/行为树**，现有 SearchForTarget 够用（AI 重构在 M2 单独立项）
5. 先做 Development 打包打通（修复 #6 技能静默丢失），再开始魂类开发

---

## 1. 免费资源总表（2026-10-16 核实许可与 5.7 状态）

### 1.1 A 类：作为插件装进项目（全部源码、免费）

| 资源 | 许可 | 5.7 状态 | 承担职责 | 引入里程碑 |
|---|---|---|---|---|
| **ActionCombatCore**（自研） | 自有，可独立开源 | 随项目 | 输入缓冲/行动预检/无敌帧窗口/锁定 | M1（按 spec） |
| **Advanced Sessions**（[mordentral](https://github.com/mordentral/AdvancedSessionsPlugin)） | MIT | 5.6 分支，源码自编译 5.7 | Steam/EOS 会话搜索、邀请、大厅属性 | M7 |
| **CommonSession + CommonUser**（从 Lyra 提取的源码插件） | Epic 免费，随 Lyra 分发 | 跟随 5.7 | 会话/登录/平台抽象，替代裸 OnlineSubsystem | M7（二选一/并存） |
| Motion Warping | 引擎自带 | ✅ | 攻击吸附、翻滚贴向、处决对齐 | M1（spec 指定，必选） |
| GameplayInsights | 引擎自带 | ✅ | GAS 标签/GE/技能时间线调试 | M1 |
| StateTree + AIModule | 引擎自带 | ✅ | M2 敌人 AI 骨架、感知 | M2 |
| Niagara / MetaSounds | 引擎自带 | ✅ | 特效 / 音频与 BGM 钩子 | M5、M8 贯穿 |
| Enhanced Input / Control Rig（IK Rig+IK Retargeter）/ Common UI / World Partition+Data Layer | 引擎自带 | ✅ | 输入 / 动画重定向 / 手柄 UI / 大地图分区 | 各自里程碑 |

### 1.2 B 类：只当字典读、可移植代码但**不合并**的免费开源工程

> 统一放仓库外 `Reference/`（加入 .gitignore），禁止把其模块/命名空间并入五模块依赖图。

| 工程 | 许可 | 引擎版本 | 抄什么 |
|---|---|---|---|
| [Lyra Starter Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)（[GitHub 镜像](https://forums.unrealengine.com/t/lyra-starter-game-source-code-now-on-github/767395)） | Epic 免费 | 5.7 同步 | GAS 多人装备/武器切换、队伍与友伤过滤、伤害管线、Common UI 菜单规范、EOS 接入、Settings 体系 |
| [GASP 游戏动画示例](https://www.unrealengine.com/tech-blog/explore-the-updates-to-the-game-animation-sample-project-in-ue-5-7) | Epic 免费 | 5.7 专版 | Motion Matching + Chooser + Pose Search、Smart Object、Motion Warping 官方范例 |
| [tomlooman/ActionRoguelike](https://github.com/tomlooman/ActionRoguelike) | MIT | 主分支 5.6（clone 时选匹配分支） | **契合度最高**：共斗第三人称、多怪攻击名额（Token）协调、Boss 多阶段、死亡掉货币+拾取、Checkpoint 存档、交互组件、掉落管线 |
| [tranek/GASDocumentation](https://github.com/tranek/GASDocumentation) | MIT | 文档随 5.x（示例 5.3，按分支取） | GAS 网络同步、Cost/Cooldown GE、MMC、AbilityTask 权威写法 |
| Aura（Stephen Ulibarri 课程配套，GitHub 公开） | 免费 | UE5.x | GAS RPG 全流程：属性初始化、伤害公式、HitReact、属性条 UI |
| [Souls 式输入缓冲 on GAS](https://www.quodsoler.com/blog/building-souls-style-input-buffering-on-top-of-gas) | 免费文章 | — | 连招缓冲窗口实现（spec 阶段 2 接线参考） |
| [SoulslikeEnemyCombat 公开文档](https://hakanerunsal.com/docs/SoulslikeCombatDocs) | 文档免费（**插件收费，不买**） | 5.6/5.7/5.8 | M2 敌人 AI 七系统设计蓝图：走位/动作评分/反应/多怪分工角色/感知记忆/近战扫描/Targeting |
| [ALS Refactored](https://github.com/Sixze/ALS-Refactored) | MIT | 4.17 target 5.7 | **本项目不采用**（见 §1.4），仅作为移动/相机实现的对照阅读材料 |

### 1.3 C 类：免费内容资产

| 来源 | 内容 | 使用约束 |
|---|---|---|
| [Mixamo](https://www.mixamo.com/)（Adobe 免费账号） | 角色/攻击/受击/翻滚/处决动画，经 IK Retargeter 重定向 | 仅限游戏内使用，不可单独转售素材；spec 已定为翻滚动画主来源（GASP 评估失败后） |
| Paragon 资产（Fab 免费） | 角色/动画/特效，项目已大量使用（Greystone 145 动画） | 限 Unreal 引擎项目 |
| Megascans/Quixel（Fab 免费） | 扫描材质/模型，M6 关卡 | 限 Unreal 引擎项目 |
| Fab 每月限免 | 动作包、特效包、音效包定期淘取 | 各资产逐一看许可 |
| 引擎自带内容 | StarterContent、Niagara 示例、MetaSounds 预设 | 自由使用 |

### 1.4 D 类：后端/服务（M7 再决策，现在不装）

| 方案 | 性质 | 适用 |
|---|---|---|
| **EOS**（Epic Online Services） | 免费额度大，Lyra 全套示范 | 账号、大厅、中继 P2P、成就，上线快 |
| [**Nakama**](https://heroiclabs.com/nakama/)（[UE 客户端文档](https://heroiclabs.com/docs/nakama/client-libraries/unreal/index.html)） | Apache-2.0 开源自建（Docker） | 账号/匹配/排行榜/**云存档（血遗云端化）**/服务端权威逻辑，不想被平台绑定时 |
| FMOD 免费档 | 预算 20 万美元内免授权 | 仅当 MetaSounds 无法满足时后补，现在不引入 |

### 1.5 明确不采用的免费/付费方案及原因

| 方案 | 原因 |
|---|---|
| ALS Refactored（整体引入） | M1 spec 已决定锁定自研、保留 SpringArm；整体移动重构风险高、收益与当前阶段不匹配。仅阅读对照 |
| GCS / ACF / ACS 等全栈战斗框架（无论免费付费） | 与自研 GAS 层（AttributeSet/Execution/MontageEventAbility/事件总线）双轨并行，技术债大于收益 |
| SoulslikeEnemyCombat 付费插件 | 预算为零；其公开文档已足够支撑 M2 自研 |
| 任何 Fab 第三方"免费下载"盗版站（如 3dcghub 之流） | 版本旧、可能带恶意代码、核心模块不可控；只用 Fab 官方与 GitHub 官方仓库 |

---

## 2. 里程碑路线与资源嵌入

```
M0 准备期 → M1 战斗内核(已有spec) → M2 敌人AI → M3 魂循环(掉魂/篝火)
→ M4 刷怪成长 → M5 Boss战 → M6 关卡节奏 → M7 联机共斗/PvP → M8 演出(贯穿)
```

每个里程碑的完成定义统一为：**编译通过 → PIE 验证 → LAN 双端（listen+client）验证 → Development 打包验证（承接 #6 教训）→ 单独提交 Git**，与批次 A 的既有习惯一致。

### M0｜准备期（2–3 天，只搭环境）

1. 仓库外建 `Reference/` 并 clone：Lyra（5.7）、GASP（5.7）、ActionRoguelike（选匹配分支）、GASDocumentation；**只读**
2. `Reference/` 加入 `.gitignore`；`Plugins/` 下每个第三方插件根目录保留 LICENSE 副本
3. 动画管线一次性定死：目标骨骼（Greystone/Paragon 骨架）与 Mixamo 源骨骼的 IK Rig/IK Retargeter 资产建好并验证一个动画——此决策影响后续全部动画资产
4. 按 M1 spec 前置项打通 Development 打包、修复 #6
5. 建好 M1 spec 的插件分支与 `ActionCombatCore` 空骨架（仅编译验证）

### M1｜战斗手感内核（约 4.5 周）

**权威设计以 [魂类战斗核心 spec](../superpowers/specs/2026-10-09-soulslike-combat-core-design.md) 为准**（伤害链路统一 → Stamina/Poise → 翻滚无敌帧 → 锁定 → 韧性格挡弹反，四阶段）。免费资源在其中的位置：

| spec 任务 | 免费资源 |
|---|---|
| 三套伤害链路收敛到 ExecCalc | GASDocumentation 的 GE/Execution 章节；Lyra 伤害管线对照 |
| Stamina 的 Cost GE + 周期回复 + 回复停顿 | GASDocumentation 的 Cost/Cooldown + MMC 章节；Aura 属性初始化对照 |
| 输入缓冲 | Quod Soler 输入缓冲文章；ActionRoguelike 的交互/能力缓冲写法 |
| 翻滚动画 | GASP 评估（3 天止损）→ Mixamo + IK Retargeter；位移用引擎自带 ApplyRootMotionConstantForce + Motion Warping |
| 无敌帧/锁定组件 | 自研进 ActionCombatCore（spec 已定，不抄 ALS） |
| 攻击前后摇 Section + 吸附 | Greystone 现成 Attack_* 素材；Motion Warping 官方范例看 GASP |
| 调试 | GameplayInsights（spec 已指定） |

### M2｜敌人 AI（2–3 周）

1. **StateTree**（5.7 自带）重写敌人行为：巡逻 → 警觉（感叹号提示）→ 逼近走位 → 按距离/权重选攻击 Ability → 后撤；替换 SearchForTarget 雏形
2. 攻击仍走 GAS Ability + 蒙太奇前摇，复用 MontageEventAbility 骨架，网络架构零分歧
3. **多怪轮流出手**：移植 ActionRoguelike 的 Attack Token 名额协调组件思路（组件化，与本项目架构契合）
4. 走位/评分/感知/记忆系统：照 SoulslikeEnemyCombat 公开文档的 7 系统自研，其 ActionSets/Profiles 用本项目 DataAsset 风格重建
5. 群体避让用 CharacterMovement 自带 RVO；近战判定继续用现有 `UPP_MeleeAttack` swept trace
6. **验收**：3 只怪轮流出招不群殴；每招有可翻滚躲避的清晰前摇；LAN 双端行为一致

### M3｜魂循环：死亡掉魂 + 篝火（1–2 周）

| 子系统 | 落地方式与参考 |
|---|---|
| Souls 货币 | Inventory 组件照金币模式新增（属性/接口/`FPP_SoulsUpdatedEvent`/UI），走事件总线与 Core 接口，不给 UI 引具体类 |
| 血遗（掉地→捡回→二次死亡清空） | 照 ActionRoguelike 的 credits-pickup-on-death 流程：死亡点 spawn 拾取 Actor，持久化进 `UPP_LocalSaveGame` |
| 篝火 | 新 Actor 实现 `IPP_Interactable`：回满状态、重置区域敌人（配合 M6 Data Layer）、存档、加点入口；复活点改用篝火（替换 `bShouldRespawn` 语义） |
| 存档 | 参照 ActionRoguelike 的 SaveGame 子系统模式扩展现有 LocalSaveGame |
| 传送 | 复用大厅 ServerTravel 经验 |

**验收**：死亡掉魂 → 捡尸取回；途中二次死亡魂清空；篝火休息后敌人重置、状态回满、进度落盘。

### M4｜刷怪与成长（2 周）

1. **LootTable 数据表**进 DataManager（权重/数量/稀有度），EnemyCharacter 死亡按表掉落，替换单一 GoldDropAmount；管线参考 ActionRoguelike PickupSpawn
2. **属性加点**：篝火处消耗 Souls 升级（生命/精力/力量/敏捷…），缩放公式写进 DamageExecution，MMC 写法查 GASDocumentation
3. **装备强化/词条**：扩展 `FPP_ItemInfo`，强化等级作用于装备 GE 数值
4. 装备管理（实例化/切换/快捷栏）读 **Lyra EquipmentManager + QuickBar**，但落地保持本项目 Inventory 组件 + 接口模式
5. 刷怪点配置 + 篝火重置规则，形成"刷材料 → 强化 → 推更难区域"闭环
6. **验收**：一轮完整刷怪循环（刷怪→掉落→强化→数值可感知变强→篝火重置）

### M5｜Boss 战（1–2 周）

1. EnemyCharacter 子类 + 多阶段 Ability（血量阈值转阶段，监听复用现有 AttributeChangeTask）；编排参考 ActionRoguelike Boss
2. 雾门交互（IPP_Interactable 新实现）、处决/背刺窗口（复用 M1 的 Riposte 标签）
3. 顶部 Boss 血条、阶段切换演出经 IPP_UIGateway 推送
4. 特效/音效：Niagara + MetaSounds；处决动画走 Mixamo
5. **验收**：单人 + 双人 LAN 下阶段切换同步、雾门封锁、处决窗口正确

### M6｜关卡节奏（内容向，2 周+）

1. World Partition + Data Layer 做分区与篝火重置范围
2. 篝火间距、捷径单向门/电梯、埋伏点、难度梯度分区
3. 环境素材走 Megascans；敌人交互点/埋伏位可参考 GASP Smart Object 用法
4. **验收**：一个完整"篝火 → 关卡段 → Boss 雾门"垂直切片

### M7｜联机共斗/PvP（3 周+）

1. 读 Lyra 队伍系统与友伤过滤，在 DamageExecution 的 GEContext 攻击方判定上加队伍规则
2. 会话层选型（先小规模验证再定）：
   - 轻量：Advanced Sessions（Steam/EOS 会话与邀请）
   - 正规：从 Lyra `Plugins/Online/` 提取 CommonSession + CommonUser 源码拷入本项目 Plugins/ 编译
3. 共斗：召唤/密码匹配复用 SessionManager + 会话层；PvP：入侵标记 + 队伍反转
4. GAS 复制问题查 GASDocumentation 网络章节；共斗流程对照 ActionRoguelike Listen Server
5. 需要账号/云存档/反作弊时再引入 Nakama（Docker 自建）或 EOS，二者 M7 初做 ADR 决策
6. **验收**：Internet/LAN 召唤共斗 Boss、入侵 PvP 各跑通完整一局；血遗等持久化在断线后行为正确

### M8｜演出打磨（贯穿每个里程碑）

命中顿帧（短时 TimeDilation，自写）、镜头震动、低血量后处理+心跳音、弹反火花、处决慢镜；伤害数字已有。每项半天成本，随当批战斗改动一起做，不立大批次。

---

## 3. 引入纪律（保护五模块解耦成果）

1. **只装源码插件**，不装纯二进制插件（5.7 新版本被弃更风险）；每个 A 类插件独立 POC 分支，编译+LAN 双端验证通过再合主线
2. **B 类工程只读不合并**：移植代码必须重写进对应模块归属（Core / 主玩法 / UI），禁止把第三方模块名、命名空间带入依赖图；依赖方向保持 Architecture.md §2 的单向无环
3. **不引入与 GAS 竞争的战斗框架**；战斗机制本体始终是自研 GAS + 五模块架构
4. 第三方插件若引入新的跨模块 native Tag，同样必须带模块 API 导出宏；其类若发生重定位，CoreRedirects 进 DefaultEngine.ini，遵守 Architecture.md §9 的全部历史教训
5. 接入插件后检查：79+ 条 CoreRedirects 不受影响、Config 段名规则、删类前 Glob Content 查资产引用、重定向修复前不保存蓝图
6. 跨系统通知一律新增 EventBus 事件结构体（在 `PP_EventTypes.h` 追加）；UI 一律走 Core 接口，禁止重新引入主玩法具体类

## 4. 资产许可红线（商业化前复核）

- Mixamo：仅限游戏内使用，**素材本身不可转售/单独分发**
- Paragon / Megascans：**限 Unreal 引擎项目**使用
- Fab 限免：逐资产保留授权截图/页面存档
- GitHub 源码：MIT/Apache-2.0 需在产品致谢或插件目录保留 LICENSE 与版权声明
- 个人开发阶段均无影响，但每引入一项在本表登记，避免商业化时回溯无据

## 5. 决策待办（需要在对应里程碑前拍板）

| 编号 | 决策点 | 截止时间 |
|---|---|---|
| D1 | GASP 动画集是否满足翻滚/战斗需求（3 天评估，失败转 Mixamo） | M1 阶段 2 |
| D2 | M7 会话层：Advanced Sessions vs Lyra CommonSession | M7 启动时 |
| D3 | M7 后端：EOS vs Nakama 自建 vs 纯 LAN 不做线上 | M7 启动时 |
| D4 | M3 Souls 与金币是合并为统一货币还是并存 | M3 设计时 |

---

## 6. 一句话总结

**自研 `ActionCombatCore` 出战斗手感（M1 已有 spec）、StateTree 出 AI、ActionRoguelike 出魂循环与多怪协调的抄作业样本、Lyra 出联机与装备标准答案、GASDocumentation 出网络同步字典、GASP/Mixamo/Paragon/Megascans 出动画与素材、EOS 或 Nakama 出后期线上服务——全程零采购，且每一份外部代码都以"只读参考或源码插件"的方式接受五模块架构与引入纪律约束。**
