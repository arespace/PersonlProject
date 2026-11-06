# Content 新建规范（标准流程与规范）

> 状态：现行 · 最后更新：2026-11-06 · 取代关系：无
> 生成日期：2026-11-06
> 配套文档：[Content资产整理方案.md](../方案/Content资产整理方案.md)（存量整理，一次性执行）；本文档管「以后新建怎么建」，长期有效。
> 所有规范与整理方案的目标结构一致，不另起一套。文中引用的类名、路径、标签均来自项目实际代码/资产。

---

## 0. 先回答：新建内容按什么流程走

一句话流程（完整链路见第 5 节）：

```
明确归属目录与命名 → C++ 类（如需）→ 重启编辑器 → 蓝图子类 → 数据表行
→ GameplayTag / GE → 蓝图配置 → 完整编译 → PIE 验证 → 代码审查 → git 提交
```

不同新内容类型的差异只在「从哪一步开始」：

| 新内容 | 起点 | 主要落点 |
|---|---|---|
| 新功能（系统级） | C++ 组件/子系统 | 主玩法模块 + EventBus 事件 + 蓝图组件配置 |
| 新角色 | DT_Characters 加行 | 网格/动画查表 + `Abilities/PlayerAbilities` 专属 GA |
| 新武器 | DT_Items + DT_WeaponData 各加行 | 外观走 `WeaponMeshMap`，技能走 `WeaponAbilityClass` |
| 新 Boss | `PP_BossCharacter` 蓝图子类 | 阶段阈值 + `SearchForTargetAbilityClass` + 场地门 |
| 新敌人 | `PP_EnemyCharacter` 蓝图子类 | `Abilities/EnemyAbilities/{Melee,Ranged}` + 动画 |
| 新 UI | C++ Widget 基类（如需） | `UI/<领域>/` + UIManager 注册 + HUD 层级配置 |
| 新拾取物/机关 | C++ Actor（如需） | `GameObjects/{Pickups,Interactables,Boss}` + IPP_Interactable |
| 纯美术/音效导入 | 导入对话框 | 只进 `UI/Textures/Imported/` 或对应角色目录，命名规范化后再引用 |

---

## 1. 目录规范：放哪、怎么分

### 1.1 总原则：按功能聚合，内部按类型分

**判据（按顺序问）**：

1. **这个资产属于某个功能/角色吗？** 属于 → 进该功能/角色的目录（`Characters/PlayerCharacter/Animations`、`GameObjects/Boss`）。
2. **是跨功能共享的吗？** 是 → 进类型目录的共享层（`AbilitySystem/GameplayEffects` 根、`Animations/Notifies`、`UI/Components`）。
3. **是第三方导入的素材吗？** 是 → 一律进 `UI/Textures/Imported/<包名>/`，不改名只搬家，禁止散落到功能目录里。
4. **是临时测试资产吗？** 是 → 测试地图放 `Maps/Dev/`，实验性资产文件名带 `_Test` 后缀，验证完两周内退役进 `_Archive/`。

**两个反模式**：

- 不要「按类型建顶层」再往里塞功能资产（`Materials/` 只放一个 M_HiddenWeapon 就是这个错误）；
- 不要「按系统建顶层」放类型混杂的东西（`Player/` 里放 `BP_EnemyController` 就是这个错误）。

### 1.2 目标目录树（以整理方案为准）

目标树、各目录的「保持 / 改名 / 并入」标注，**唯一标准**是 [《Content资产整理方案》§2.2 目标目录树](../方案/Content资产整理方案.md)。本文不再复制第二份树，避免两处漂移；目录结构调整只改那一份。

新内容落位时只需记住三条判据（详见 1.1）：按功能聚合、跨功能共享进类型根目录、第三方素材进 `UI/Textures/Imported/<包名>/`。目录是否允许新建，看 1.3 的准入规则。

### 1.3 新目录的准入规则

- **顶层目录（PersonalProject 下第一级）禁止新增**。当前 14 个顶层已覆盖全部类型；确有新类型（如音效目录 `Audio/`）先在本规范登记再建。
- **二级目录按功能命名**，英文、复数、不缩写：`Pickups` 对，`pk`、`Pickup2` 错。
- **不要为「只有 1 个资产」建目录**——先放最近的聚合目录，超过 3 个同类再拆子目录。

### 1.4 特殊资产的归属约定

| 资产 | 放哪 | 原因 |
|---|---|---|
| 玩家新技能蒙太奇 | `Animations/AlsKwang(Monages)` 或角色 Animations 目录 | **禁止放 ALS 插件目录**（重装插件即丢）；重定向后放 Retargeted |
| 角色/皮肤变体 | 网格进 `DT_Characters.CharacterMesh` 查表；动画共用 AlsKwang/AlsShinbi 一套 | 同角色多皮肤共用 IK Rig + 重定向 AnimBP，新皮肤只加表行 |
| 武器外观网格 | `Characters/PlayerCharacter/Materials` 旁新增 `Weapons/`（第一个武器资产落地时建） | 与 WeaponMeshMap 配置就近 |
| 共享 GE/GA（敌我通用） | `GameplayEffects` 根 / `Abilities` 根 | 现行约定：根目录即共享层 |
| 第三方 UI 素材 | `UI/Textures/Imported/<包名>/` | 整包可替换、不污染项目资产 |
| 测试残留 | `_Archive/`（退役）或 `Maps/Dev/`（还在用） | 不留在正式目录 |

---

## 2. 命名规范

### 2.1 资产前缀（通用部分以整理方案为准）

`BP_`、`WBP_`、`ABP_`、`AB_`、`AM_`、`A_`、`BS_`、`GA_PP_`、`GE_PP_`、`GC_`、`DT_<域>_<对象>`、`IA_` / `IMC_`、`AN_`、`M_` / `SM_` / `T_`、`IKR_` / `RTG_` 的规则与示例，见 [《Content资产整理方案》§2.3 命名约定](../方案/Content资产整理方案.md)。**两份冲突时以整理方案为准。**

整理方案未列、但本规范新增的前缀：

| 前缀 | 类型 | 示例 |
|---|---|---|
| `MI_` | 材质实例（参数化统一走 MI） | `MI_Weapon_Iron_Base` |
| `SK_` | 骨骼网格（导入时统一） | `SK_Weapon_Longsword` |
| `CT_` | 曲线表 | `CT_PlayerDamage` |
| `NS_` | Niagara 系统 | `NS_HitSpark` |
| `AIS_` | ALS 动画实例子类 | `AIS_Als_Kwang` |

新增前缀必须先加到本表，再在整理方案 §2.3 同步，避免两处不一致。

### 2.2 C++ 侧对应命名（与资产前缀一一映射）

| C++ | 资产 | 示例 |
|---|---|---|
| `APP_Xxx` | `BP_Xxx` | `APP_BossCharacter` → `BP_BossCharacter` |
| `UPP_Xxx`（类） | — | `UPP_EnemyRespawnManager` |
| `UPP_XxxWidget` | `WBP_Xxx` | `UPP_SettlementWidget` → `WBP_Settlement` |
| `FPP_Xxx` | USTRUCT | `FPP_WeaponMeshVisual` |
| `EPP_Xxx` | UENUM | `EPP_SlotType` |
| `PPTags.Xxx.Yyy` | GameplayTag | `PPTags.SetByCaller.StaminaCost`、`PPTags.Status.Invulnerable` |

### 2.3 硬规则

1. 禁止纯数字、单字母文件名（`1.uasset`、`s.uasset`）；只有 `UI/Textures/Imported/` 内豁免。
2. 同一动作/功能只保留一个权威资产；旧版本进 `_Archive`，不留双轨。
3. 目录名英文复数（`Animations`、`Pickups`），修正历史拼写（`Pags`、`Word`、`AbillityActions`）。
4. 技能蒙太奇 Slot 一律 `Locomotion.Default`——命名里不用体现 Slot，但资产内容必须遵守（见 3.3）。
5. 新建 C++ 类名确定后**改名成本极高**（牵动 CoreRedirects + 蓝图父类），命名前先对照本表。

---

## 3. 各类资产的内容要求（建的时候必须配什么）

### 3.1 GameplayAbility（GA）

| 必配项 | 要求 | 依据 |
|---|---|---|
| Ability Tags | 在 GA 本体配 AbilityTags；角色专属 GA 的激活标签与 `CharacterAbilityMap` 授予链一致；输入走 `TryActivateAbilitiesByTag`，**不要**再绑独立 InputAction | 标签驱动输入路线 |
| NetExecutionPolicy | 监听类（`GA_PP_ListenForKillScored` 型）**Server Only**，否则客户端预测刷 `ClientActivateAbilityFailed` | 已验证根因 |
| EndAbility | 所有分支（含蒙太奇中断、死亡打断）保证 EndAbility；蒙太奇回调不可靠时按死亡 GA 先例加兜底 Timer（时长 +0.1s） | 死亡复活根因 |
| 消耗 | 走 `UPP_GameplayAbility` 基类 override `CheckCost/ApplyCost`，GE 用带 Data Tag 的 SetByCaller（如 `PPTags.SetByCaller.StaminaCost`）；**禁止**旧式无标签 `SetByCallerFloat` | 耐力方案 |
| Slot | 蒙太奇 Slot = `Locomotion.Default`；`DefaultGroup.DefaultSlot` 会丢姿势 | 已验证根因 |
| 授出去重 | 同一个 GA 只能在 `BP_PP_PlayerCharacter.StartupAbilities` **或** `BP_CrashGameMode.CharacterAbilityMap` 一处授予；两处同授 = 双实例双倍耗体力 | 已验证根因 |
| 角色差异化 | 角色专属 GA：从 StartupAbilities 移除、只进 CharacterAbilityMap；GA 槽位与 C++ 基类一一对应 | 角色差异化路线 |

### 3.2 GameplayEffect（GE）

| 必配项 | 要求 |
|---|---|
| DurationType | Instant（伤害/消耗/加血）、Infinite（持续回复/ drains）、Duration（临时状态）；按语义选，不凭感觉 |
| Stacking | 需要叠加时显式配 StackLimitType/StackDurationRefreshPolicy；默认不叠就不留空配置 |
| Modifier | 伤害/消耗用 `PP_AttributeSet.Xxx Add (Base)`；**回满属性禁止用 Add 型 GE**（每次 +100 叠加，已验证），直接代码赋值 |
| SetByCaller | 必须 `SetByCaller` + Data Tag（如 `PPTags.SetByCaller.StaminaCost`），Magnitude 用 Tag 读取 |
| UE5.7 组件 | Grant Tags / Ongoing Tag Requirements **不再是 GE 本体字段**，用 Components 添加（Grant Tags to Target Actor / Require Tags to Apply\|Continue This Effect） |
| 命名 | `GE_<功能>_<对象>`，放 Owner 目录（Player/Enemy）或根（共享） |

### 3.3 蒙太奇与重定向动画

- Slot 必须 `Locomotion.Default`；自定义蒙太奇放项目目录（`Animations/Als*/Montages` 或角色 Animations），**永不**放 ALS 插件目录。
- 外部动画重定向到 Kwang/Shinbi：用 `AlsAnimationModifier_CreateLayeringCurves` 勾选 **Add Slot Curves** 烘焙 6 条 Layer 权重曲线；Retargeter 的 Root/Pelvis Motion 显式指定骨骼；导出加 `_Kwang/_Shinbi` 后缀。
- 重定向修复后**必须重新批量导出覆盖旧动画**，否则游戏仍用旧数据。

### 3.4 数据表

| 表 | 规则 |
|---|---|
| `DT_Items` vs `DT_WeaponData` | 相互独立但 **RowName 必须一致**；武器同时出现在两表，ItemID = RowName |
| `WeaponMeshMap`（PP_PlayerCharacter） | Key = 武器 ID = `DT_WeaponData` RowName = `DT_Items` ItemID，**三方一致**；每把武器独立配网格 + 相对变换 |
| 全局表（物品/任务/角色） | 注册进 `UPP_DataManager` + `DefaultGame.ini` 对应段（`[/Script/PersonalProjectCore.PP_DataManager]`）；任务表缺行会 warn、物品表不 warn——新增表时确认 warn 策略 |
| 实例级表（商人商品） | 留在组件实例，不进 DataManager |
| `DT_Characters` | 新角色加行：RowName 即 `CharacterAbilityMap` 的 Key；CharacterMesh/AnimBP 空值回落 `SKM_Als`/`AB_Als`（网格路径不是骨架路径） |
| RowName | 一旦被引用（蓝图/表/代码）**永不改名**——RowName 改名不走 CoreRedirects |

### 3.5 Widget（WBP）

- C++ 基类 `BindWidget` 的控件名与 WBP 内控件**同名**，否则绑定静默为空；
- 常驻 HUD 顶层 Widget 设 **HitTestInvisible**（Visible 会拦 Slate 鼠标导致视角不转）；
- Widget 层级由蓝图 **OwningLayer** 配置，输入模式由 UIManager 栈管理；`CreateWidget` 的 OwningObject 只能是 UWorld/APlayerController/UserWidget/GameInstance（**不能传 AHUD**）；
- 面板类用 `CreatePanelWidget/ShowPanel` 模板；`Hide*` 是导航语义，不做对称 Hide 合并。

### 3.6 C++ 类（新建时）

- **模块归属**：接口/全局结构/标签下沉 `PersonalProjectCore`；库存任务链按既有模块依赖（Core ← Inventory ← Quest ← 主玩法 ← UI），不新增模块；全局管理器用 ActorComponent 挂 GameMode/GameInstance，不写单例。
- 核心头文件带模块 `*_API` 宏（PPTags.h 全量带 `PERSONALPROJECTCORE_API`）。
- 构造函数**禁止** `ConstructorHelpers/FClassFinder/StaticLoadClass` 加载「引用后加载模块类的蓝图」；跨模块资产运行时 `LoadClass<T>()`（UE5.7 需显式模板参数）。
- `UCLASS(Abstract)` 不许纯虚函数（C2259）；用「默认实现 + 注释要求 override」。
- 私有 UPROPERTY 需 Blueprint 可见时加 `meta=(AllowPrivateAccess="true")`。
- 接口 `BlueprintNativeEvent` 只能用 `Execute_Xxx(Object, ...)` 调用。
- 跨系统通信走 EventBus（`UPP_EventBus`）+ GameplayTag 事件（命名 `Event.<系统>.<动作>`，如 `Event.Boss.Dead`）；禁止 Actor 互持引用直调；UI 只经接口 + 事件总线。
- C++ 类改名/换模块必须同步：CoreRedirects（类/结构体/枚举三类全量比对）+ `DefaultGame.ini` 段名（**CoreRedirects 不作用于 ini 段名**）。

---

## 4. 完整链路：从需求到可提交（问题 3）

### 4.1 步骤表

| # | 步骤 | 做什么 | **必须重启编辑器？** | 易漏点 |
|---|---|---|---|---|
| 1 | 归属判定 | 按 1.1 判据定目录 + 按 2.x 定名；对照本规范，不开新顶层 | 否 | 名字定了再建，建完改成本高 |
| 2 | C++ 类（如需） | 按模块归属新建；接口下沉 Core；编译通过 | **是**（新增 UCLASS 后需重启才出现在蓝图面板） | 加了 UPROPERTY/默认组件/Server RPC/USTRUCT 时 Live Coding 无效，**关编辑器完整编译** |
| 3 | 蓝图子类 | 右键 C++ 类 → Create Child Blueprint（角色类必须 Child Blueprint，不能 Duplicate）；**C++ 类编译报错时绝不保存蓝图** | — | 组件/类解析失败时保存蓝图会固化空引用，不可回滚 |
| 4 | 数据表行 | DT_Items/DT_WeaponData/DT_Characters 按需加行；RowName 对齐（3.4） | 否 | RowName 被引用后不可改；武器三方 Key 一致性 |
| 5 | GameplayTag / GE | 新标签进 PPTags.h（走 pp-gas-tag-convention skill）；GE 按 3.2 配置 | 加标签 = 改 C++，需重启 | UE5.7 GE 组件式配置；SetByCaller 带 Data Tag |
| 6 | 蓝图配置 | GA/GE/组件字段、CharacterAbilityMap、WeaponMeshMap、UI OwningLayer | 否 | **授予去重**（StartupAbilities vs CharacterAbilityMap）；Boss 必配 SearchForTargetAbilityClass |
| 7 | 完整编译 | 关编辑器 → Build.bat（走 pp-build-precheck skill） | **是** | 只改 cpp 函数体才可 Ctrl+Alt+F11 热更；函数内 static 不随热更重置 |
| 8 | PIE 验证 | 按内容类型跑验证清单（战斗图/大厅/交互/UI/输入/日志） | 建议重启后验 | 日志四查：`Failed to load` / `Unable to travel` / `Failed to find` / `Redirector` 警告 |
| 9 | 代码审查 | pp-code-review skill；蓝图配置项走 pp-blueprint-config-checklist skill | 否 | 重点查授权去重、RowName 对齐、网络复制 |
| 10 | 提交 | `git add` 具体文件（含 .uasset）；一个功能一批提交（批次完成统一提交是项目约定） | 否 | LFS 已接管 Content 路径，首次提交后确认 `git lfs status` |

### 4.2 三个最容易漏的检查点

1. **步骤 2→3 之间的重启**：不重启就找不到新 C++ 类（面板无条目），会有人顺手 Duplicate 一个旧蓝图改——直接违反 3.6。
2. **步骤 6 的授予去重**：新技能忘删 StartupAbilities 里的旧条目，测试时「技能偶尔双倍耗体力」，极难定位。
3. **步骤 8 的日志检查**：功能「看起来能用」但 `Failed to load` 已埋雷，打包后才炸。PIE 结束必看 Output Log。

---

## 5. 新建内容自检清单（照着勾）

**命名与归属**
- [ ] 资产落在 1.2 目录树的既有位置，没有新建顶层目录
- [ ] 前缀符合 2.1 表；GA 带 `PP_`、GE 带 `PP_` 或功能前缀、Widget 是 `WBP_`
- [ ] 无纯数字/单字母名（Imported 目录内豁免）
- [ ] C++ 类名/模块归属符合 3.6；标签已加进 PPTags.h

**GAS 正确性**
- [ ] GA：AbilityTags 已配；监听类 Server Only；所有分支 EndAbility；蒙太奇 Slot = Locomotion.Default
- [ ] GA 只在一处授予（StartupAbilities XOR CharacterAbilityMap），已去重
- [ ] GE：Duration/Stacking 按语义；SetByCaller 带 Data Tag；Grant/Require Tags 用 UE5.7 Components 方式
- [ ] 属性回满用代码赋值，不是 Add 型 GE

**数据一致性**
- [ ] 武器：DT_Items RowName = DT_WeaponData RowName = WeaponMeshMap Key = ItemID
- [ ] 全局表已注册 DataManager 且 DefaultGame.ini 段名正确
- [ ] DT_Characters 新行：网格/动画路径正确（SKM_Als 兜底），CharacterAbilityMap Key = RowName

**蓝图与网络**
- [ ] 角色类是 Child Blueprint（非 Duplicate）；C++ 编译错误时未保存任何蓝图
- [ ] 服务端逻辑有 HasAuthority 守卫；没调 GetPlayerState(0)
- [ ] 外观变更走 OnRep 回调（参考 OnRep_WeaponSlots 模式）；知道 SetMaterial/SetStaticMesh 不复制
- [ ] HUD 常驻 Widget = HitTestInvisible；CreateWidget OwningObject 合法

**验证与提交**
- [ ] 编辑器完整重启 + 编译通过（非 Live Coding 补丁状态）
- [ ] PIE：主战斗图 + 大厅双图验证；Output Log 无 `Failed to load` / `Failed to find` / `Redirector` 警告
- [ ] pp-code-review / pp-blueprint-config-checklist 已跑
- [ ] git add 具体文件；`git lfs status` 确认大文件走 LFS

---

## 6. 你可能漏掉的维度（问题 4）

按「新内容生命周期」排列，每条是本项目真实踩过或极可能踩的坑：

1. **重定向器卫生**：移动/改名后旧位置留 Redirector。规范：每次操作完当场 Fix Up Redirectors + Refresh Asset Registry + Save All；季度全项目搜 `ObjectRedirector` 清零（当前已知 5 个，见整理方案 M5）。长期不清理会让「资产在哪」失去唯一答案。
2. **git LFS 与提交粒度**：.uasset 二进制不可 diff，出问题只能整文件回滚。提交时一个功能一批（项目约定），避免「10 个功能混一个 commit」后无法部分回退；换机器 clone 后必须 `git lfs pull`，否则蓝图全挂。
3. **World Partition 外部 Actor**：LobbyMap 的 Actor 存在 `Content/__ExternalActors__/PersonalProject/Maps/LobbyMap/`，与地图路径强绑定。**地图永不移动**；新建地图时就要定好名，因为 HLOD/外部 Actor 从第一天开始积累。
4. **资产重复与引用核实**：Duplicate 会复制整套内部引用；删「看起来没用」的资产前必须 Reference Viewer + 全项目字符串扫描（GA_PP_Fifth 教训：C++ 零引用 ≠ 资产零引用）。退役走四步：确认 0 引用 → 移 `_Archive/` → 观察两周 → 删除。
5. **命名冲突**：不同目录同名资产会让 CoreRedirects、字符串扫描、「谁引用了它」全部失准。新建前先在 Content Browser 全局搜一遍名字；这也是禁止纯数字名的原因。
6. **跨模块引用纪律**：C++ 模块依赖单向（Core ← Inventory ← Quest ← 主玩法 ← UI）；新功能默认落现有 5 模块，不新增模块；UI 对业务类的引用保持清零状态（接口 + EventBus）。建新系统时先想「事件怎么发」而不是「谁调谁」。
7. **蓝图硬引用阻碍扩展**：蓝图里直接拖引用（如写死 GA 类、写死网格路径）会让「按表驱动」的扩展点失效。能用 `TSubclassOf`、DataTable 行、GameplayTag 驱动的，就不硬引。CharacterAbilityMap、DT_Characters、WeaponMeshMap 是三个现成的表驱动范例。
8. **共享资产的归属**：跨敌我用的 GA/GE 放共享层（Abilities 根 / GE 根）；改共享资产必须回归两侧（玩家侧 PIE + 敌人侧 PIE 都验）。避免「为了方便」把共享资产塞进 Player 目录——下一次敌人要用时又要搬。
9. **废弃资产的退役流程**：没有流程就会重现 `GE_Heal_Health`（与 AddHealth 双轨并存）。退役 = 移 `_Archive/`（不删，保留引用关系可查）→ 一个月无恢复 → 彻底删除。**删除类（C++ UCLASS）前先查 Content 里是否有资产以它为父类**。
10. **防止再乱的三道闸**：① 新建即按本规范落位（本文件）；② 每完成一个功能批次，把产生的杂项当场归位，不留「下次再整理」；③ 季度跑一次整理方案的扫描脚本（目录分布 + 重定向器 + 无引用资产），偏差超 10% 触发一次小整理。
11. **网络复制的隐形边界**：SetMaterial/SetStaticMesh 仅服务端可见；篝火类关卡 Actor 无 owning connection，Server RPC 会被丢弃（要走 PC 转发）。新交互物先想清楚「它有没有 Owner」。
12. **构造函数加载红线**：新 C++ 类的构造函数里手痒加 `ConstructorHelpers::FClassFinder` 是最高频复发点——只允许运行时 `LoadClass<T>()`。构造函数引用错模块 = 编辑器启动即崩，且日志极难读懂。
13. **Config 与代码的同步**：新增全局数据表、新增 Config 类时，`DefaultGame.ini` 的段名/路径当天同步；CoreRedirects 不救 ini。改完 ini 重启编辑器验证表路径非空（DataManager 日志 Item/Quest/Character 三项）。
14. **备份红线不变**：Content 已入 git + LFS，但「组件/类解析失败时绝不保存蓝图」与「关键节点前 _Backups 兜底」两条仍然有效——git 保护的是历史，救不了「当场固化空引用」。

---

## 附：本规范与存量整理的衔接

- 整理方案执行完毕前，新内容照本规范落位即可，**不要**为对齐旧资产而暂停新功能开发。
- **目标目录树与通用命名前缀以《Content资产整理方案》为唯一标准**；本规范只维护新建操作流程、各资产类型的必配项与自检清单。两份文档出现不一致时，目录树/前缀以整理方案为准，流程与必配项以本规范为准。
- 本规范 2.1 新增的前缀（`MI_`/`SK_`/`CT_`/`NS_`/`AIS_`）需同步进整理方案 §2.3。
