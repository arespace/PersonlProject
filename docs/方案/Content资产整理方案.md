# Content/PersonalProject 资产整理方案

> 生成日期：2026-11-06
> 依据：对 `Content/PersonalProject` 下 **781 个文件（778 个 .uasset + 3 个 .umap，约 100 MB）** 的实际扫描，包括目录/命名统计、资产间引用扫描（二进制字符串）、`Source/**` 与 `Config/**` 的硬编码路径检索。
> 本文件只做分析与给出步骤，**没有移动、重命名或保存任何资产，也没有修改代码**。

---

## 0. 结论先行

真正混乱的只有 3 处，不是整个目录：

| 序号 | 混乱点 | 体量 | 占比 |
|---|---|---|---|
| 1 | `UI/Button/UI`：第三方 UI 素材被嵌套成 `UI/Button/UI/Button` 四层，且 244 个素材里 **220 个全项目无外部引用** | 244 | 31% |
| 2 | `Characters/PlayerCharacter/Animations`：13 个蒙太奇/序列平铺，存在 5 组重复或废弃版本（同一动作 2~3 份） | 25 | 3% |
| 3 | 6 个「只有 1~3 个文件」的顶层小目录（`Player`、`Game`、`Notifies`、`Word`、`Materials`、`FX`），且 `Player` 里装的是 `BP_EnemyController` | 13 | 2% |

反而是体量最大的 `Animations/AlsKwang` + `Animations/AlsShinbi`（各 172 个）**结构已经是对的**：按「动画蓝图 / 混合空间 / 蒙太奇 / 序列」四类分好了，只是根目录散落了 4~5 个技能蒙太奇。这部分不要拆。

建议按 4 个批次执行：

- **批次 0 清理**：删 redirector、归档无引用资产（零搬迁风险）
- **批次 1 目录收口**：小目录归位、UI 分类细化（零代码、零 ini）
- **批次 2 大范围迁移**：UI 素材包整体搬家、角色动画目录重整（中风险，需逐批验证）
- **批次 3 暂缓**：需要同步改 C++ / Config 的搬迁，本次不做，只在第 4 节列出条件

`Content/PersonalProject` 当前**已在 git 跟踪范围内**（`.gitignore:43` 放行，`.gitattributes:11` 走 LFS），有版本回滚兜底；但仍要按 5.1 做一次物理备份再动手。

---

## 1. 现状盘点

### 1.1 顶层目录分布

| 顶层目录 | 文件数 | 占比 | 现状判断 |
|---|---:|---:|---|
| `Animations` | 348 | 44.7% | 体量大但结构合格（见 1.4 M1） |
| `UI` | 285 | 36.6% | 真正的重灾区（见 1.4 M2） |
| `Characters` | 52 | 6.7% | 中等，动画子目录乱 |
| `AbilitySystem` | 41 | 5.3% | 结构合格，仅根目录散落 6 个 GE |
| `Input` | 19 | 2.4% | 结构合格，目录名拼写错误 |
| `Data` | 10 | 1.3% | 结构合格，混了 1 个废弃表 |
| `GameObjects` | 9 | 1.2% | 平铺，可加二级分类 |
| `Maps` | 4 | 0.5% | 不能动（见第 4 节） |
| `Player` | 3 | 0.4% | 职责错位 |
| `FX` | 3 | 0.4% | 太小，不必拆 |
| `Game` | 3 | 0.4% | 不能动（ini 引用） |
| `Notifies` | 2 | 0.3% | 应并入 Animations |
| `Word` | 1 | 0.1% | 拼写错误（World），应并入 GameObjects |
| `Materials` | 1 | 0.1% | 单文件目录，应并入角色目录 |

合计 781。

### 1.2 资产类型分布

| 类型（按命名前缀） | 数量 | 说明 |
|---|---:|---|
| 动画序列 `A_` | 254 | 几乎全部是 ALS 重定向产物（Kwang / Shinbi 各 128） |
| 无规范前缀 | 197 | 其中 193 个在 `UI/Button/UI`，其余是散落的动画/地图附属资产 |
| 纯数字命名 | 45 | 全部在 `UI/Button/UI`（`1.uasset`、`12.uasset`…） |
| 动画蓝图 `AB_` | 44 | ALS 每角色 21 个 + 外壳 1 个 |
| 蒙太奇 `AM_` | 37 | 玩家/敌人技能与受击 |
| 贴图 `T_` | 36 | 装备图标 |
| Widget `WBP_` | 33 | 项目自有 UI |
| 蓝图 `BP_` | 29 | 角色/机关/框架 |
| GameplayEffect `GE_` | 20 | GAS |
| GameplayAbility `GA_` | 18 | GAS |
| 混合空间 `BS_` | 18 | ALS 每角色 9 个 |
| 数据表 `DT_` | 10 | 全局表 + 对话表 |
| 输入动作 `IA_` | 15 | |
| 输入映射 `IMC_` | 4 | |
| 地图 `.umap` | 3 | PPMap / LobbyMap / Test_ALS |
| GameplayCue `GC_` | 2 | |
| 动画通知 `AN_` | 2 | |
| `ABP_` | 2 | 敌人动画蓝图 |
| Retargeter `RTG_` | 2 | |
| IK Rig `IKR_` | 2 | |
| 动画实例 `AIS_` | 2 | |
| 材质 `M_` | 2 | |
| `BPW_` / `SM_` / `CT_` / `NS_` | 各 1 | 前缀各自为政 |

### 1.3 「混得最厉害」的具体位置

```
Content/PersonalProject/
├─ UI/
│  └─ Button/
│     └─ UI/                 ← 244 个素材，四层嵌套，目录名与父目录重名
│        ├─ 1.uasset 2 3 4   ← 根目录直接放贴图，纯数字名
│        ├─ Button/          ← 104 个：button_01、s、x、w68、chat_06…
│        ├─ login/           ← 48 个：0、1、2、login_button_w1…w7
│        ├─ Icon/            ← 29 个：1、12、23…
│        ├─ Equip/           ← 36 个：T_ICON_*、T_UI_*（这批有规范）
│        ├─ common/          ← 16 个：Circle、Circle2、ExampleArrow…
│        └─ Main/            ← 7 个：0-0、1-0、fw05…
├─ Characters/PlayerCharacter/Animations/   ← 13 个蒙太奇/序列平铺 + 重复版本
└─ Player/                   ← 3 个文件，其中 BP_EnemyController 不属于这里
```

### 1.4 混乱点与证据

**M1｜`Animations` 体量大但不是问题，问题是根目录散落**
`AlsKwang`（172 个）= 根 5 + 动画蓝图 21 + 混合空间 9 + 蒙太奇 9 + 序列 128；`AlsShinbi` 完全镜像。结构本身正确。
真正的问题：
- `Animations/AlsKwang/` 根目录直接放了 `Primary_AKwang`、`Primary_BKwang`、`PP_Primary_B_AlsKwang` 三个蒙太奇/序列；
- `Animations/AlsKwang/Sequences/` 里又混进了 `PP_Primary_A_AlsKwang`（技能蒙太奇不该在 Sequences）；
- `AlsShinbi` 同样有 4 个。

**M2｜`UI/Button/UI` 是导入素材包，不是项目资产**
- 244 个文件，占全部资产 31%；
- 命名完全来自素材包：`1/2/3/4`、`s`、`x`、`w68`、`button_25_1_on`；
- 目录 `UI/Button/UI` 与父目录 `UI/Button` 语义重复；
- 引用扫描结果：**只有 24 个被外部引用**（`WBP_Inventory` 用 8 个、`WBP_Shop`/`WBP_ShopItem` 用 4 个、`WBP_QuestListWidget`/`WBP_EquipSlot`/`WBP_ItemSlot` 各 1 个、`DT_Items` 用 9 个图标），其余 220 个无外部引用。

**M3｜小目录职责错位**
- `Player/` 里放 `BP_EnemyController`（敌人 AI 控制器），而 `Characters/EnemyCharacter/` 里才有敌人本体；
- `UI/LobbyInterface/Core/` 里放 `BP_LobbyGameMode`（GameMode 属于框架层，不属于 UI）；
- `Data/` 根目录放全局表，`Data/NPC/` 又放 NPC 对话表，`Data/Login/` 放登录表——三套规则并存；
- `Notifies/`、`Word/`、`Materials/`、`FX/` 都是 1~3 个文件的顶层目录，每个都要多一次点击。

**M4｜命名不一致（可直接改的拼写错误）**

| 现状 | 问题 | 建议 |
|---|---|---|
| `Input/AbillityActions/` | Abillity 拼写错误 | `Input/AbilityActions/` |
| `UI/LobbyInterface/Pags/` | Pags 应为 Pages | `UI/Lobby/Pages/` |
| `Word/Gatherables/` | Word 应为 World | 并入 `GameObjects/` |
| `GA_PP_Attack_Ramged` | Ramged → Ranged | `GA_PP_Attack_Ranged` |
| `GE_PlyaerDamage` | Plyaer → Player | `GE_PP_PlayerDamage` |
| `AM_Attack_Melle_1..6` | Melle → Melee | `AM_Attack_Melee_1..6` |
| `GA_Secondary` / `GA_Tertiary` | 缺 `PP_` 前缀 | `GA_PP_Secondary` / `GA_PP_Tertiary` |
| `NPC_LI` / `NPC_Zhang` | 一个全大写、一个首字母大写 | `NPC_Li` / `NPC_Zhang` |
| `BPW_CustomButton` | 前缀与 `WBP_` 体系不一致 | `WBP_CustomButton` |
| `DT_DialogueNPC_Merchant` | 词序不规范 | `DT_NPC_Dialogue_Merchant` |
| `PP_Primary_A_Als` vs `PP_Primary_A_AlsKwang` | 同一个 `Als` 既表示「ALS 骨架」又像角色后缀 | 新资产统一为 `AM_<角色>_<技能>` |

**M5｜5 个 redirector 残留（其中 2 个是坏的）**

| 路径 | 指向 | 状态 |
|---|---|---|
| `UI/LobbyInterface/BP_LobbyGameMode` | `UI/LobbyInterface/Core/BP_LobbyGameMode` | 正常重定向，可 Fix Up 后删除 |
| `Characters/NPC/NPC_A` | `Characters/NPC/NPC_LI` | 正常重定向 |
| `Characters/NPC/NPC_B` | `Characters/NPC/NPC_Zhang` | 正常重定向 |
| `Animations/AlsKwang/AB_Als_ViewKwang` | **指向自身** | 坏重定向，全项目 0 引用 |
| `Animations/AlsShinbi/AB_Als_ViewShinbi` | **指向自身** | 坏重定向，全项目 0 引用 |

另有一个明显的空蓝图：`UI/LobbyInterface/NewBlueprint.uasset`（UserWidget 空蓝图，0 引用）。

**M6｜疑似废弃 / POC 残留（引用数已核实）**

| 资产 | 项目内引用数 | 判断 |
|---|---:|---|
| `AM_Death_Player` | 0 | 废弃；死亡实际用 `Melee/AM_MeleePlayer_Death` |
| `Melee/AM_Death_MeleePlayer` | 0 | 同上 |
| `Melee/AM_MeleePlayer_HitReact` | 0 | 废弃；`GC_HitReact` 用 `AM_Player_HitReact` |
| `Melee/Attack_A_Med_MSA` + `_Montage` | 只互相引用 | 废弃；`GA_PP_Primary` 用 `Attack_A_Slow` / `Attack_B_Med_MSA` |
| `PP_Primary_A_Montage`、`PP_Primary_B_Montage` | 0 | 废弃；技能用 `Retargeted/PP_Primary_A_Als` |
| `AM_SecondaryAbility` + `Q_Tracker_Cut` | 0 / 只被前者引用 | 废弃 |
| `DT_Dialogue_Merchant` | 0 | 废弃；`BP_NPC_Merchant` 用 `DT_DialogueNPC_Merchant` |
| `GE_Equip_IronSword` | 0 | 废弃（且放在 `Consumable/` 分类错误） |
| `GC_BurstImpact` | 0 | 废弃 |
| `Game/BP_Als_GameMode` + `BP_Als_Character_Child` + `IMC_PP_AlsTest` + `Maps/Test_ALS` | 仅互相引用 | ALS 模板 POC 四件套，确认不用后再归档 |

**M7｜分类原则冲突**
当前同层级混用两套标准：
- 按**类型**：`Animations`、`UI`、`Data`、`Input`、`Materials`、`Notifies`
- 按**系统**：`AbilitySystem`、`GameObjects`、`Characters`、`Game`、`Player`

结果是同一个功能被切到两个地方（例如 `BP_LobbyGameMode` 在 `UI/`，`BP_CrashGameMode` 在 `Game/`；`BP_EnemyController` 在 `Player/`，敌人本体在 `Characters/`）。

---

## 2. 目标结构

### 2.1 设计原则

1. **顶层保持「类型优先」**：UE 的引用、烘焙、Content Browser 都按资产类型工作，按系统分顶层会让「找一张贴图」要翻遍所有系统目录。
2. **二级按领域/系统**：`UI/Shop`、`Characters/PlayerCharacter`、`AbilitySystem/Abilities/Player`。
3. **只在必要时新增目录**，不重命名现有顶层目录（`Animations` 保持复数，避免 348 个资产整体改路径）。
4. **UI 目录只放 UI 资产**；GameMode / PlayerController / PlayerState 属于框架层，不进 UI。
5. **一个资产只有一个权威版本**，废弃版本进 `_Archive`，不留在正式目录里。

### 2.2 目标目录树

```
Content/PersonalProject/
├─ AbilitySystem/                     [保持现状]
│  ├─ Abilities/
│  │  ├─ PlayerAbilities/
│  │  └─ EnemyAbilities/{Melee,Ranged}
│  ├─ GameplayEffects/{Player,Enemy,Consumable}
│  └─ GameplayCues/                   ← 路径被 DefaultGame.ini 引用，不动
│
├─ Animations/
│  ├─ AlsKwang/                       [保持] 172 个，已按 4 类分好
│  │  ├─ AnimationBlueprints/  BlendSpaces/  Montages/  Sequences/
│  ├─ AlsShinbi/                      [保持] 同上
│  ├─ Retarget/                       ← 由 AlsRetarget 改名（IKR_*/RTG_*）
│  └─ Notifies/                       ← 由顶层 Notifies 并入（AN_*）
│
├─ Characters/
│  ├─ PlayerCharacter/
│  │  ├─ BP_PP_PlayerCharacter / BP_Als_Character_Child
│  │  ├─ Animations/
│  │  │  ├─ Montages/                 ← 由 Animations/Melee 并入
│  │  │  ├─ Source/                   ← 原始序列（Primary_A/B）
│  │  │  └─ Retargeted/               [保持]
│  │  └─ Materials/                   ← M_HiddenWeapon
│  ├─ EnemyCharacter/                 [保持] BP_* + Animation/
│  └─ NPC/                            [保持] 3 个蓝图
│
├─ Data/                              [保持] 全局表在根（ini 引用）
│  ├─ DT_Items / DT_Task / DT_Characters / DT_WeaponData / DT_ShopInventory
│  ├─ NPC/                            ← 对话表
│  └─ Login/
│
├─ GameObjects/
│  ├─ Interactables/                  ← BP_Bonfire、BP_Gatherable_Wood
│  ├─ Pickups/                        ← BP_HealthPickup、BP_ManaPickup、BP_SoulPickupActor
│  ├─ Projectiles/                    ← BP_PP_Projectile、BP_PP_PlayerProjectile、Mesh/
│  └─ Boss/                           ← BP_BossArenaGate
│
├─ Input/
│  ├─ AbilityActions/                 ← 由 AbillityActions 改名
│  ├─ MovementActions/
│  └─ UI/
│
├─ Maps/                              [保持] 绝对不能动
│  ├─ PPMap / LobbyMap / LobbyMap_HLOD0_Instancing
│  └─ Dev/Test_ALS                    ← 测试图单独放（可选）
│
├─ Player/                            [保持] 代码硬编码，本批不动
├─ Game/                              [保持] ini 引用，本批不动
│
├─ UI/
│  ├─ HUD/
│  │  ├─ WBP_HUDWidget / BP_HUD
│  │  ├─ Combat/                      ← WBP_AbilitySlot、WBP_Crosshair、WBP_DamageNumber
│  │  ├─ Info/                        ← WBP_Notification、WBP_Scoreboard
│  │  └─ Status/                      ← WBP_HealthBar、WBP_ManaBar、WBP_PlayerStatsWidget、WBP_AttributeBar
│  ├─ Inventory/                      ← WBP_Inventory、WBP_EquipSlot、WBP_ItemSlot、WBP_ItemTooltip
│  ├─ Shop/                           ← WBP_Shop、WBP_ShopItem、WBP_BuyCount
│  ├─ Dialogue/                       ← WBP_DialogueBox、WBP_DialogueChoice
│  ├─ Quest/                          ← 由 TaskUI 改名（3 个 WBP）
│  ├─ Lobby/
│  │  ├─ Pages/                       ← 由 Pags 改名（7 个 WBP）
│  │  └─ Components/                  ← WBP_PlayerEntry、WBP_TopBar
│  ├─ Components/                     ← WBP_CustomButton（原 BPW_CustomButton）
│  └─ Textures/Imported/              ← 由 Button/UI 整体搬入
│     ├─ Button/ Common/ Equip/ Icon/ Login/ Main/ Misc/
│
├─ FX/                                [保持] 3 个，不值得再拆
└─ _Archive/                          [新增] 待删除/待确认资产，1~2 周后清空
```

### 2.3 命名约定（新资产 + 本次顺手修正）

| 类型 | 前缀 | 示例 |
|---|---|---|
| Actor 蓝图 | `BP_` | `BP_Bonfire` |
| Widget 蓝图 | `WBP_` | `WBP_Inventory` |
| 项目自建动画蓝图 | `ABP_` | `ABP_Enemy_Melee` |
| ALS 重定向动画蓝图 | `AB_`（保持） | `AB_Als_Kwang` |
| 蒙太奇 | `AM_` | `AM_MeleePlayer_Death` |
| 动画序列 | `A_` | `A_Als_IdleKwang` |
| 混合空间 | `BS_` | `BS_Als_LeanKwang` |
| GameplayAbility | `GA_PP_` | `GA_PP_Primary` |
| GameplayEffect | `GE_PP_` | `GE_PP_Cost_Stamina` |
| GameplayCue | `GC_` | `GC_HitReact` |
| 数据表 | `DT_<域>_<对象>` | `DT_NPC_Dialogue_Merchant` |
| 输入 | `IA_` / `IMC_` | `IA_Primary` / `IMC_Abilities` |
| 动画通知 | `AN_` | `AN_MeleeAttack` |
| 材质 / 网格 / 贴图 | `M_` / `SM_` / `T_` | `T_ICON_Coin` |
| IK / 重定向 | `IKR_` / `RTG_` | `IKR_Kwang` |

硬规则：
- 禁止纯数字、单字母文件名（`1`、`s`、`x`、`w68` 这类只允许存在于 `Textures/Imported/`）。
- 目录名用英文复数，不用缩写、不用拼音。
- 同一动作只保留一个权威资产，废弃版本移入 `_Archive`，不留在原目录。
- 技能蒙太奇**只能**放项目目录（`Animations/Als*/Montages` 或角色目录），**不得**放回 ALS 插件目录。

### 2.4 为什么这样分（贴合现有代码结构）

- 5 个代码模块（Core / Inventory / Quest / 主玩法 / UI）是**编译边界**，资产不需要与模块一一对应；资产层的等价边界是：
  - `AbilitySystem` = GAS 全部资产（GA/GE/GC），对应所有模块共用的战斗层；
  - `Characters` = 角色实体 + 其动画，对应主玩法模块；
  - `UI` = 唯一可以反向引用所有系统的层，因此必须物理隔离，禁止把 GameMode/Controller 放进来；
  - `Data` / `Input` / `Maps` = 全局配置型资产。
- ALS 的两个角色目录（`AlsKwang` / `AlsShinbi`）是重定向工具的输入根目录，代码里写死了，所以保持「角色 → 资产类型」两层不变。
- `UI/Textures/Imported` 把第三方素材集中隔离，这样「项目自有 UI」和「素材包」一眼可分，也方便以后整包替换。

---

## 3. 迁移映射表

> 排序原则：**先删/归档 → 再动不需要改代码的目录 → 最后动大范围素材**。每批做完就验证一次，不要连着做两批再测。

### 批次 0：清理（不需要搬目录，先做）

| # | 现有路径 | 操作 | 数量 | 风险 | 说明 |
|---|---|---|---:|---|---|
| 0-1 | `UI/LobbyInterface/BP_LobbyGameMode` | Fix Up 后删除 | 1 | 极低 | 指向 `Core/BP_LobbyGameMode`；`LobbyMap` 已引用 Core 版 |
| 0-2 | `Characters/NPC/NPC_A` | Fix Up 后删除 | 1 | 极低 | Redirector → `NPC_LI` |
| 0-3 | `Characters/NPC/NPC_B` | Fix Up 后删除 | 1 | 极低 | Redirector → `NPC_Zhang` |
| 0-4 | `Animations/AlsKwang/AB_Als_ViewKwang` | 确认后删除 | 1 | 低 | 自指 Redirector，0 引用；删前用 Reference Viewer 复核 |
| 0-5 | `Animations/AlsShinbi/AB_Als_ViewShinbi` | 确认后删除 | 1 | 低 | 同上 |
| 0-6 | `UI/LobbyInterface/NewBlueprint` | 删除 | 1 | 极低 | 空 UserWidget 蓝图，0 引用 |
| 0-7 | `Characters/PlayerCharacter/Animations/AM_Death_Player` | 移入 `_Archive/` | 1 | 低 | 0 引用 |
| 0-8 | `.../Animations/Melee/AM_Death_MeleePlayer` | 移入 `_Archive/` | 1 | 低 | 0 引用 |
| 0-9 | `.../Animations/Melee/AM_MeleePlayer_HitReact` | 移入 `_Archive/` | 1 | 低 | 0 引用 |
| 0-10 | `.../Melee/Attack_A_Med_MSA` + `Attack_A_Med_MSA_Montage` | 移入 `_Archive/` | 2 | 低 | 只互相引用 |
| 0-11 | `.../Animations/PP_Primary_A_Montage`、`PP_Primary_B_Montage` | 移入 `_Archive/` | 2 | 低 | 0 引用 |
| 0-12 | `.../Animations/AM_SecondaryAbility`、`Q_Tracker_Cut` | 移入 `_Archive/` | 2 | 低 | 前者 0 引用，后者只被前者引用 |
| 0-13 | `Data/DT_Dialogue_Merchant` | 移入 `_Archive/` | 1 | 低 | 0 引用 |
| 0-14 | `AbilitySystem/GameplayEffects/Consumable/GE_Equip_IronSword` | 移入 `_Archive/` | 1 | 低 | 0 引用，且分类错误 |
| 0-15 | `AbilitySystem/GameplayCues/GC_BurstImpact` | 移入 `_Archive/` | 1 | 低 | 0 引用 |
| 0-16 | `Game/BP_Als_GameMode`、`Characters/PlayerCharacter/BP_Als_Character_Child`、`Input/IMC_PP_AlsTest`、`Maps/Test_ALS` | 整组移入 `_Archive/` | 4 | 中 | 四件只互相引用，ALS 模板 POC 残留；**先确认不再用 Test_ALS 调移动**再动 |

批次 0 完成后验证：PIE 进 PPMap 打一场、进大厅建一次房。`_Archive` 只归档不删除，确认一周无异常后再清空。

### 批次 1：目录收口（零代码、零 ini）

| # | 现有路径 | 目标路径 | 数量 | 风险 | 说明 |
|---|---|---|---:|---|---|
| 1-1 | `UI/WBP_HealthBar` | `UI/HUD/Status/` | 1 | 低 | UI 根目录散落 |
| 1-2 | `UI/WBP_ManaBar` | `UI/HUD/Status/` | 1 | 低 | |
| 1-3 | `UI/WBP_PlayerStatsWidget` | `UI/HUD/Status/` | 1 | 低 | |
| 1-4 | `UI/GAS/Core/WBP_HUDWidget` | `UI/HUD/` | 1 | 低 | `GAS` 目录名无信息量 |
| 1-5 | `UI/GAS/Status/WBP_AttributeBar` | `UI/HUD/Status/` | 1 | 低 | |
| 1-6 | `UI/GAS/Combat/WBP_AbilitySlot`、`WBP_Crosshair`、`WBP_DamageNumber` | `UI/HUD/Combat/` | 3 | 低 | |
| 1-7 | `UI/GAS/Info/WBP_Notification`、`WBP_Scoreboard` | `UI/HUD/Info/` | 2 | 低 | |
| 1-8 | `UI/Inventory/WBP_Shop`、`WBP_ShopItem`、`WBP_BuyCount` | `UI/Shop/` | 3 | 低 | 商店不是背包 |
| 1-9 | `UI/Inventory/WBP_DialogueBox`、`WBP_DialogueChoice` | `UI/Dialogue/` | 2 | 低 | 对话不是背包 |
| 1-10 | `UI/Inventory/WBP_Inventory`、`WBP_EquipSlot`、`WBP_ItemSlot`、`WBP_ItemTooltip` | 留在 `UI/Inventory/` | 4 | — | 不动 |
| 1-11 | `UI/TaskUI/*` | `UI/Quest/` | 3 | 低 | 统一术语（代码里是 Quest） |
| 1-12 | `UI/LobbyInterface/Pags/*` | `UI/Lobby/Pages/` | 7 | 低 | 修正 Pags 拼写 |
| 1-13 | `UI/LobbyInterface/Components/WBP_PlayerEntry`、`WBP_TopBar` | `UI/Lobby/Components/` | 2 | 低 | **`BP_LobbyPlayerState` 留在原处**（C++ 硬编码） |
| 1-14 | `UI/Button/BPW_CustomButton` | `UI/Components/WBP_CustomButton` | 1 | 低 | 移动 + 改名 |
| 1-15 | `Notifies/AN_MeleeAttack`、`AN_SendEventToActor` | `Animations/Notifies/` | 2 | 低 | 顶层小目录归位 |
| 1-16 | `Word/Gatherables/BP_Gatherable_Wood` | `GameObjects/Interactables/` | 1 | 低 | 修正 Word 拼写并归位 |
| 1-17 | `Player/BP_EnemyController` | `Characters/EnemyCharacter/` | 1 | 低 | 唯一引用者是 `BP_PP_EnemyBase`，会自动重定向 |
| 1-18 | `GameObjects/BP_Bonfire` | `GameObjects/Interactables/` | 1 | 低 | |
| 1-19 | `GameObjects/BP_BossArenaGate` | `GameObjects/Boss/` | 1 | 低 | |
| 1-20 | `GameObjects/BP_HealthPickup`、`BP_ManaPickup`、`BP_SoulPickupActor` | `GameObjects/Pickups/` | 3 | 低 | |
| 1-21 | `GameObjects/BP_PP_Projectile`、`BP_PP_PlayerProjectile` | `GameObjects/Projectiles/` | 2 | 低 | |
| 1-22 | `GameObjects/Mesh/SM_Projectile`、`M_Projectile` | `GameObjects/Projectiles/Mesh/` | 2 | 低 | |
| 1-23 | `Materials/M_HiddenWeapon` | `Characters/PlayerCharacter/Materials/` | 1 | 低 | 单文件目录消除 |
| 1-24 | `Animations/AlsKwang/{Primary_AKwang, Primary_BKwang, PP_Primary_B_AlsKwang}` | `Animations/AlsKwang/Montages/` | 3 | 低 | 重定向工具递归扫描该目录，不受影响 |
| 1-25 | `Animations/AlsKwang/Sequences/PP_Primary_A_AlsKwang` | `Animations/AlsKwang/Montages/` | 1 | 低 | 技能蒙太奇不该在 Sequences |
| 1-26 | `Animations/AlsShinbi/` 对应 4 个 | `Animations/AlsShinbi/Montages/` | 4 | 低 | 同上 |
| 1-27 | `Animations/AlsRetarget/*` | `Animations/Retarget/` | 4 | 低 | 目录改名；无代码/配置引用 |
| 1-28 | `Input/AbillityActions/` | `Input/AbilityActions/` | 6 | 低 | 目录改名，修正拼写 |

### 批次 2：大范围迁移（中风险，单独做）

| # | 现有路径 | 目标路径 | 数量 | 风险 | 说明 |
|---|---|---|---:|---|---|
| 2-1 | `UI/Button/UI/Button` | `UI/Textures/Imported/Button/` | 104 | 中 | 24 个被 WBP/DT 引用，整目录搬迁后必须 Fix Up |
| 2-2 | `UI/Button/UI/common` | `UI/Textures/Imported/Common/` | 16 | 中 | |
| 2-3 | `UI/Button/UI/Equip` | `UI/Textures/Imported/Equip/` | 36 | 中 | `DT_Items` 引用其中 9 个图标 |
| 2-4 | `UI/Button/UI/Icon` | `UI/Textures/Imported/Icon/` | 29 | 中 | |
| 2-5 | `UI/Button/UI/login` | `UI/Textures/Imported/Login/` | 48 | 中 | |
| 2-6 | `UI/Button/UI/Main` | `UI/Textures/Imported/Main/` | 7 | 中 | |
| 2-7 | `UI/Button/UI/{1,2,3,4}` | `UI/Textures/Imported/Misc/` | 4 | 中 | 纯数字贴图 |
| 2-8 | `Characters/PlayerCharacter/Animations/Melee/*` | `Characters/PlayerCharacter/Animations/Montages/` | 11 | 中 | 先完成批次 0 的归档，再搬剩下的 11 个 |
| 2-9 | `Characters/PlayerCharacter/Animations/{Primary_A, Primary_B}` | `Characters/PlayerCharacter/Animations/Source/` | 2 | 低 | 原始序列，等重定向用 |
| 2-10 | `Characters/EnemyCharacter/Animation/*` | `Characters/EnemyCharacter/Animations/` | 16 | 低中 | 目录名改复数 |

### 批次 2.5：可选的命名修正（与批次 2 一起做，或单独做）

| 现名 | 新名 | 风险 | 注意 |
|---|---|---|---|
| `GA_PP_Attack_Ramged` | `GA_PP_Attack_Ranged` | 低 | 被 `BP_Enemy_Ranged` 引用，Fix Up 可覆盖 |
| `GE_PlyaerDamage` | `GE_PP_PlayerDamage` | 低 | 被 GA/BP 引用 |
| `AM_Attack_Melle_1..6` | `AM_Attack_Melee_1..6` | 低 | 被 `ABP_Enemy_Melee` 引用 |
| `GA_Secondary` | `GA_PP_Secondary` | 中 | 若 `CharacterAbilityMap` 里按类引用，Fix Up 可覆盖；若 Lua 里写死类名字符串则需同步改 |
| `GA_Tertiary` | `GA_PP_Tertiary` | 中 | 同上 |
| `NPC_LI` | `NPC_Li` | 低 | 被 `PPMap` 引用 |
| `BPW_CustomButton` | `WBP_CustomButton` | 低 | 已在批次 1-14 处理 |
| `DT_DialogueNPC_Merchant` | `DT_NPC_Dialogue_Merchant` | 低 | 被 `BP_NPC_Merchant` 引用 |

> 命名修正**不要批量重命名全部资产**。每改一个都要 Fix Up，否则旧路径会留下一堆 redirector，比原来更乱。

### 3.6 AbilitySystem 专项整理（2026-11-06 补充扫描）

> 对 `AbilitySystem` 全部 41 个资产做了逐个引用核实（扫描所有 uasset 的二进制引用）。结论：**目录骨架（Abilities → PlayerAbilities/EnemyAbilities，GameplayEffects → Player/Enemy/Consumable，GameplayCues）符合 GAS 规范，不需要推倒重排**；需要做的是归档 4 个废弃、移动 1 个放错侧的 GA、统一命名，以及不做几件「看起来整齐」的事。

**现状结构与引用核实结果**

```
AbilitySystem/  (41)
├─ Abilities/
│  ├─ GA_PP_Death                    ← 保留原地：是 Death_Player/_Melee/_Ranged 三个子类的父类，共享合理
│  ├─ GA_PP_ListenForHealthChange    ← 【移动】仅被 BP_Enemy_Melee / BP_Enemy_Ranged 引用，是敌人侧监听，应归入 EnemyAbilities
│  ├─ EnemyAbilities/
│  │  ├─ GA_PP_HitReact              ← 保留：EnemyBase + 两个 HitReact 子类的父类
│  │  ├─ GA_PP_SearchForTarget       ← 保留：敌人索敌（Boss 也配置用）
│  │  ├─ Melee/ (3)  Ranged/ (3)
│  └─ PlayerAbilities/ (8)           ← 全部在用（StartupAbilities / CharacterAbilityMap 链路）
├─ GameplayEffects/
│  ├─ 根目录平铺 6 个：
│  │  ├─ GE_AddHealth / GE_AddMana   ← 被拾取物蓝图引用（BP_HealthPickup / BP_ManaPickup）
│  │  ├─ GE_InitializeAttributes / GE_ResetAttributes ← 属性初始化/重置，三张角色蓝图引用
│  │  ├─ GE_Death                    ← 死亡 GE
│  │  └─ GE_Heal_Health              ← 【废弃】0 引用，与 GE_AddHealth 功能重复
│  ├─ Consumable/ (2)：GE_Consume_HealthPotion【废弃 0 引用】、GE_Equip_IronSword【废弃 0 引用，见 0-14】
│  ├─ Enemy/ (2)
│  └─ Player/ (11)：含 CT_PlayerDamage（曲线表，被 GE_PlyaerDamage 引用）
└─ GameplayCues/ (2)：GC_HitReact 在用、GC_BurstImpact【废弃 0 引用，见 0-15】
```

**补充操作（并入主批次表执行）**

| 编号 | 现有路径 | 操作 | 目标 | 风险 | 依据 |
|---|---|---|---|---|---|
| 0-17 | `GameplayEffects/GE_Heal_Health` | 移入 `_Archive/` | — | 低 | 0 引用，与 GE_AddHealth 重复 |
| 0-18 | `GameplayEffects/Consumable/GE_Consume_HealthPotion` | 移入 `_Archive/` | — | 低 | 0 引用；注意：药水使用功能若后续要做，从 _Archive 取回再接线 |
| 1-29 | `Abilities/GA_PP_ListenForHealthChange` | 移动 | `Abilities/EnemyAbilities/` | 低 | 仅敌人蓝图引用；与 PlayerAbilities 的 GA_PP_ListenForKillScored 形成对称 |
| 2.5-9 | `GE_Player_Secondary_Damage` | 改名 | `GE_Secondary_Damage` | 低 | 已在 Player/ 目录，"Player_" 冗余 |
| 2.5-10 | `GE_CoolDown_Secondary` | 改名 | `GE_Cooldown_Secondary` | 低 | GAS 惯例拼写 Cooldown |

**AbilitySystem 内「不要动」**

| 资产 | 原因 |
|---|---|
| `GameplayCues/` 根路径 | `DefaultGame.ini` 的 `GameplayCueNotifyPaths` 写死该目录（第 4 节 401 行）；GC 根目录不能挪，但可以在其下加子目录 |
| `Abilities/PlayerAbilities/GA_PP_Primary` | `DT_WeaponData.WeaponAbilityClass` 硬引用（见第 4 节） |
| `Abilities/PlayerAbilities/GA_PP_Fifth` | 历史坑：删/移父类会破坏 GA_PP_Fifth.uasset（见第 4 节 404 行） |
| `GameplayEffects/Player/CT_PlayerDamage` | 被 GE_PlyaerDamage 引用的曲线表；若做 2.5 的 GE_PlyaerDamage 改名，两者同批 Fix Up 即可，位置不动 |
| `GameplayEffects/Player/GE_StaminaRegen` / `GE_StaminaRegenDelay` / `GE_StaminaSprintDrain` | 耐力系统刚全链路验证（Ongoing TagRequirements 配置敏感），不为整齐移动 |
| `GA_PP_Death`（Abilities 根） | 三个 Death 子类的父类，位置正确 |

**明确不建议做的事**

- **不要**把 `GameplayEffects/Player/` 的 10 个 GE 再拆成 `Cost/ Cooldown/ Damage/ Regen/` 子目录——每类只有 1~3 个文件，41 资产规模下多一层目录纯粹增加点击；GAS 惯例是靠命名前缀（`GE_Cost_` / `GE_Cooldown_` / `GE_Damage_`）表达功能分类，目录只按 Owner（Player/Enemy）分。命名修正做完后，分类信息已在名字里。
- **不要**把 `Abilities/PlayerAbilities`、`EnemyAbilities` 改名为 `Player` / `Enemy`（与 GameplayEffects 对齐）——收益只是名字短一点，代价是 16 个 GA 全部改路径 + 全项目 Fix Up。
- **不要**按 GE 的 DurationType（Instant/Duration/Infinite）重新分类——那是引擎内部概念，不是人类查找维度。
- 根目录平铺的 5 个共享 GE（AddHealth/AddMana/Death/Initialize/Reset）**可选**建 `GameplayEffects/Shared/` 收纳；5 个文件不算乱，做不做都行，做的话归入批次 1。

---

## 4. 不要动清单（含原因与受影响位置）

| 路径 | 被谁引用（已核实） | 为什么不能动 |
|---|---|---|
| `Maps/PPMap.umap`、`Maps/LobbyMap.umap` | `DefaultEngine.ini:13-14`、`DefaultGame.ini:118-119`、`PP_CrashGameMode.cpp:202`、`PP_LobbyGameMode.cpp:216`、`PP_LobbyTypes.h:38` | 配置 + C++ 双硬编码；LobbyMap 还是 World Partition，外部 Actor 存在 `Content/__ExternalActors__/PersonalProject/Maps/LobbyMap/`，地图一移动外部 Actor 与 HLOD 全部失联 |
| `Maps/LobbyMap_HLOD0_Instancing` | `LobbyMap` | 与地图绑定 |
| `Animations/AlsKwang/**`、`Animations/AlsShinbi/**` | `DT_Characters` 里的 `AB_Als_Kwang` / `AB_Als_Shinbi`、`PP_AlsRetargetTool.cpp:71,561,772,778` | 重定向工具的目标根目录写死；`DT_Characters` 硬引用动画蓝图；每个角色 172 个资产整体迁路径收益极低、风险极高 |
| `Characters/PlayerCharacter/BP_PP_PlayerCharacter` | `PP_AlsRetargetTool.cpp:730` | 编辑器工具写死路径 |
| `Player/BP_PP_PlayerController` | `PP_LobbyGameMode.cpp:45` | 运行时 `LoadClass` 写死 |
| `UI/LobbyInterface/Components/BP_LobbyPlayerState` | `PP_LobbyGameMode.cpp:49` | 运行时 `LoadClass` 写死 |
| `UI/LobbyInterface/Core/BP_LobbyHUD` | `PP_LobbyGameMode.cpp:59` | 运行时 `LoadClass` 写死 |
| `UI/LobbyInterface/Core/BP_LobbyGameMode` | `LobbyMap` World Settings | 地图 GameMode Override |
| `Data/DT_Items`、`DT_Task`、`DT_Characters` | `DefaultGame.ini:136-138` | DataManager 三表路径写死 |
| `Data/Login/DT_LoginMenu` | `Content/UI/UIBP/Login/WBP_LoginView`（**本目录之外的蓝图**） | 跨目录硬引用，移动会连带影响外部 UI 资产 |
| `AbilitySystem/GameplayCues/**` | `DefaultGame.ini:2` `GameplayCueNotifyPaths` | 路径写死，移走后 Cue 不再被识别 |
| `Game/BP_CrashGameMode`、`Game/BP_CrashGameInstance` | `DefaultEngine.ini:17,20`、`PPMap` | 配置 + 地图引用 |
| `AbilitySystem/Abilities/PlayerAbilities/GA_PP_Primary` | `DT_WeaponData` 的 `WeaponAbilityClass` | 数据表硬引用；换路径要改表 |
| `AbilitySystem/Abilities/PlayerAbilities/GA_PP_Fifth` | `BP_PP_PlayerCharacter`、`BP_Als_Character_Child` | 蓝图引用；且历史上因删类导致资产父类丢失，**不要因为「看起来没用」删除** |
| `Characters/NPC/NPC_LI`、`NPC_Zhang`、`BP_NPC_Merchant` | `PPMap`、NPC 对话表 | 地图引用 |
| `Plugins/ALS/Content/**` | ALS 插件自身 | 自定义技能蒙太奇**不得**放回插件目录（重装插件会丢） |

---

## 5. 操作顺序与安全规则

### 5.1 动手前：双备份

1. **物理备份**（不依赖 git）：
   ```powershell
   robocopy "Content\PersonalProject" "..\_Backups\PersonalProject_Content_20261106" /MIR
   ```
   备份放项目目录之外，避免被 Content Browser 扫描。
2. **git 检查点**：
   ```powershell
   git status --short
   git add Content/PersonalProject Config Source
   git commit -m "chore: checkpoint before content reorganization"
   ```
   `Content/PersonalProject` 已被 `.gitignore` 白名单放行、`.gitattributes` 走 LFS，可以正常提交/回滚。
3. **关闭 PIE 和所有蓝图编辑器窗口**，回到主编辑器。

### 5.2 每批的标准操作（8 步）

1. **先移动，后修正**：一次只做一个批次，不要边搬边改名。
2. **只用 Content Browser 或 `EditorAssetLibrary.move_asset`**：
   - 手动：在 Content Browser 里选中资产/文件夹 → 右键 **Move Here**；
   - 脚本：`Tools → Execute Python Script`（UE 控制台 `py` 不支持多行缩进），
     ```python
     import unreal
     unreal.EditorAssetLibrary.move_asset(
         "/Game/PersonalProject/Old/Asset",
         "/Game/PersonalProject/New/Asset")
     ```
   - **绝对禁止**在资源管理器 / PowerShell 里移动 `.uasset`，否则 `SoftObjectPath` 与蓝图节点里的字符串引用会留在旧路径。
3. **Fix Up Redirectors**：移动后在 Content Browser 里右键**旧目录** → **Fix Up Redirectors in Folder**。旧目录会留下 redirector，这一步把它们改写成新路径。
4. **Refresh Asset Registry**：右键根目录 → **Refresh Asset Registry**（或重启编辑器）。
5. **Save All**：`Ctrl+Shift+S`，让所有被重定向的蓝图落盘。
6. **重启编辑器**（批次 2 必做；批次 0/1 可只重启 PIE）。
7. **跑验证清单**（见 5.3）。
8. **提交**：验证通过后 `git add -A && git commit -m "refactor(content): 批次 N ..."`，把这一批固化，再开始下一批。

### 5.3 每批验证清单

| 场景 | 检查点 |
|---|---|
| 主战斗图 `PPMap` | 玩家角色外观/动画正常（不是灰模）、技能按键有反应、翻滚/奔跑有体力消耗 |
| 战斗 | 近战小怪、远程小怪能索敌并攻击；受击有硬直；死亡后能复活（不掉输入） |
| 交互 | 篝火可休息、回满属性、敌人重置；拾取物可拾取 |
| Boss | 进入场地门触发、阶段血条切换、结算界面弹出 |
| 大厅 `LobbyMap` | 角色列表有数据（DT_Characters 生效）、预览角色正常、可建房/开始 |
| UI | HUD 血条/蓝条/体力条刷新；背包、商店、对话、任务面板可打开 |
| 输入 | 视角能转、跳跃/冲刺正常、UI 打开后鼠标模式正确 |
| 日志 | 无 `Failed to load`、`Unable to travel`、`Failed to find`、`Redirector` 警告 |

### 5.4 出问题怎么回滚

| 现象 | 处理 |
|---|---|
| 某一批验证失败 | 先 **Ctrl+Z**（编辑器内移动是可撤销的）；若已 Save All，则用 git 回滚：`git restore --source=HEAD -- Content/PersonalProject`，并删除本批新建的空目录 |
| 蓝图引用变空 / `Component class is not set` | 说明该资产被移动但 redirector 未 Fix Up：回到旧目录执行 Fix Up Redirectors，再 Save All |
| 只有 `DT_Characters` 选角失效 | 该表内部是软路径，运行 `Fix Up Redirectors` 后重新保存数据表 |
| 打包后资源丢失 | 说明被移动的资产只被「字符串路径」引用（ini / C++ / Lua）。按第 4 节核对是否有遗漏的配置项 |
| 彻底乱掉 | 关闭编辑器，删掉 `Content/PersonalProject`，从 5.1 的物理备份 `robocopy /MIR` 还原，再 `git reset` 到检查点 |

### 5.5 常见误区

- **不要对 `_Archive` 里的资产做 Fix Up**，否则会把引用重新拉回正式资产。
- **不要用 Windows 资源管理器整理蓝图**，这是本项目历史上引用失效最常见的原因。
- **不要在 Live Coding 状态下做大范围移动**，移动完必须完整重启编辑器再验证。
- **不要一次性移动 300+ 个资产后再测**，一批一测，出问题才能定位到具体批次。

---

## 6. 明确「不需要整理」的部分

| 范围 | 原因 |
|---|---|
| `Animations/AlsKwang`、`Animations/AlsShinbi` | 344 个资产已经按「动画蓝图/混合空间/蒙太奇/序列」分好，且是重定向工具的写死根目录。只需把根目录散落的 4~5 个蒙太奇挪进 `Montages/`（批次 1-24~1-26），其余不动 |
| `Maps/` | 配置 + C++ + World Partition 外部 Actor 三重绑定，收益为 0，风险最高 |
| `AbilitySystem/` 主体结构 | `Abilities / GameplayEffects / GameplayCues` 已经符合 GAS 规范；只需归档 2 个无引用资产 |
| `Data/` 根目录的三张全局表 | 路径写在 `DefaultGame.ini`，移动需要改配置，收益小 |
| `Input/` | 已按 Ability/Movement/UI 分好，只需改目录拼写 |
| `FX/` | 只有 3 个资产，再拆只会增加层级 |
| `Characters/NPC/` | 3 个蓝图，无需再分 |
| `UI/Button/UI` 内部逐个细分 | 220 个无引用素材不值得人工分类；整包搬到 `UI/Textures/Imported/` 即可，将来整包替换更省事 |
| `Content/` 下的第三方素材包（`Paragon*`、`SciFi_UI_Project`、`GuiParts`、`Longsword` 等） | 不在本次范围，且多数可从市场重新获取；不要为了「整齐」去动它们 |

---

## 7. 执行顺序速查

```
[ ] 备份 + git checkpoint
[ ] 批次 0：删 redirector / 空蓝图 / 归档无引用资产（16 项）
[ ] 验证 → 提交
[ ] 批次 1：小目录归位 + UI 分类细化 + 拼写修正（28 项，零代码）
[ ] 验证 → 提交
[ ] 批次 2：UI/Button/UI 整包迁移 + 角色动画目录重整（10 项）
[ ] 验证 → 提交
[ ] 批次 2.5（可选）：个别资产改名（每改一个 Fix Up 一次）
[ ] 一周后：确认 _Archive 无用，清空
[ ] 批次 3（本次不做）：见第 4 节，需同步改 C++ / ini
```
