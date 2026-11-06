# 项目架构梳理 — 现状 / 问题 / 目标 / 迁移步骤

日期：2026-11-06
分支：`poc/als-integration`（已推送到 origin，含 F1/F2/F3/F5/F10 全部实现）
配套文档：[FEATURE_PLAN.md](FEATURE_PLAN.md)（功能规划）、[ALS_Bug复盘.md](../复盘/ALS_Bug复盘.md)（历史教训）

---

## 一、现状总览

### 1.1 模块依赖图（当前真实依赖，单向无环 ✓）

```
                ┌─────────────────────────────────────────────┐
                │              引擎 / 插件层                    │
                │  GAS · ALS · AIModule · OnlineSubsystem · UMG │
                └──────────────────────┬──────────────────────┘
                                       │
┌──────────┐   ┌──────────────┐   ┌────┴─────────┐   ┌──────────┐
│    UI    │──▶│PersonalProject│──▶│    Quest     │──▶│Inventory │
│ HUD/大厅 │   │ 战斗/玩家/Boss │   │ 任务/对话/NPC │   │库存/商店  │
└────┬─────┘   └──────┬───────┘   └──────┬───────┘   └────┬─────┘
     │                │                  │                │
     └────────────────┴────────┬─────────┴────────────────┘
                               ▼
                    ┌────────────────────┐
                    │  PersonalProjectCore │
                    │ 角色基类/标签/EventBus │
                    │ 数据头/接口/ASC/ALS基类│
                    └────────────────────┘
```

- 依赖方向 `UI → PersonalProject → Quest → Inventory → Core` 单向无环，**这条主线是健康的**。
- UI 模块额外 Private 依赖 UnLua/Lua（MenuData 胶水）。
- Core 依赖较重但均有明确理由（见各 Build.cs 注释）：ALS（BaseCharacter 继承 AAlsCharacter）、AIModule（Tick 读 GetFocusActor）、OnlineSubsystem（SessionManager 持 IOnlineSessionPtr）、GAS（ASC 下沉 Core）。

### 1.2 模块内容清单

| 模块 | 目录 | 内容 |
|---|---|---|
| **Core** | Characters/ | APP_BaseCharacter（继承 AAlsCharacter）、CharacterData、CharacterPreviewActor |
| | AbilitySystem/ | UPP_AbilitySystemComponent、UPP_AttributeSet |
| | AI/ | UPP_AIPerceptionComponent |
| | GameplayTags/ | PPTags.h/cpp |
| | Framework/ | **13 个文件混杂**（见问题 P5） |
| | Data/ | ItemData/ItemTypes/QuestData/ShopData/DialogueData |
| | Interfaces/ + Interaction/ | Interactable/Resettable/Staggerable/EquipmentUser 接口 |
| | Inventory/ Shop/ Quest/ Dialogue/ | Query/Actions 纯接口头（供上层实现，防反向依赖）|
| **Inventory** | Inventory/ Gather/ Shop/ | PP_InventoryComponent（10 装备槽 + GE 加成）、GatherableActor、ShopComponent |
| **Quest** | Quest/ Dialogue/ | QuestComponent、QuestEventBridge、NPCActor |
| **PersonalProject** | Characters/ | ALSPlayerCharacter、PlayerCharacter、EnemyCharacter、BossCharacter |
| | AbilitySystem/ | GA 基类 + Primary~Fifth + Enemy GA + DamageExecution + AbilityTask |
| | GameObjects/ | Bonfire、BossArenaTrigger/Gate、SoulPickup、Projectile×2 |
| | Game/ | **CrashGameMode、LobbyGameMode、GameInstance、EnemyRespawnManager** |
| | Player/ | PlayerController、PlayerState、LobbyPlayerState、UIGateway |
| | Notifies/ Utils/ Editor/ | MeleeAttack Notify、蓝图库×2、AlsRetargetTool |
| **UI** | Framework/ | UIManager、UIUserWidget、UIViewModel、UIStyle、WidgetPool、UICrossModule |
| | HUD/ Lobby/ Inventory/ Quest/ Shop/ Dialogue/ Interaction/ Widgets/ | 各域 Widget；**Widgets/ 为 8 个杂项 Widget 的垃圾桶** |

### 1.3 已建立且值得保留的架构约定

1. **EventBus 解耦**：`UPP_EventBus`（GameInstance Subsystem，类型擦除 TMap<UScriptStruct*>）+ `PP_EventTypes.h` 集中定义事件结构；HUD/RespawnManager/Gate 等跨系统通信全部走它。
2. **接口下沉 Core**：`IPP_InventoryQuery/Actions`、`IPP_ShopQuery/Actions`、`IPP_QuestQuery`、`IPP_UIGateway`、`IPP_GameFlowRequest` —— 上层模块只 include 接口头，不依赖实现模块。UI 跨模块调 PlayerController 的实现在 UI 模块 `PP_UICrossModule.cpp`（避免 PersonalProject 依赖 UMG 深层）。
3. **GameplayTag 驱动状态**：`Status.Invulnerable / Boss / BossArenaArmed / Dead` 等，GAS 与 AI 门控均读标签，不硬编码。
4. **服务器权威**：Inventory/Shop 写操作全部 Server RPC，客户端 OnRep + EventBus 广播刷新 UI。
5. **组件挂 PlayerState**：Inventory/Quest 挂 PlayerState 保留跨地图数据。

---

## 二、问题清单（按影响排序）

### P1 · 命名遗留："Crash" 课程模板残留（面广，纯机械替换）
- 主 GameMode 类名 `APP_CrashGameMode`（[PP_CrashGameMode.h](../../Source/PersonalProject/Public/Game/PP_CrashGameMode.h)），对应蓝图 BP_CrashGameMode。
- **89 处** `"Crash|..."` 蓝图 Category 散布 19 个头文件（PP_PlayerController.h 15 处、PP_EnemyCharacter.h 9 处、PP_BaseCharacter.h 8 处…）。
- 文件头模板注释 `// Chen Tianbao uses GAS to create online battle games` 残留全项目。
- Category 字符串只影响编辑器显示分组，改名**不影响**蓝图已配置的属性值，安全。

### P2 · 双 GameplayTag 命名空间并存
- 主力 `namespace PPTags`（[PPTags.h](../../Source/PersonalProjectCore/Public/GameplayTags/PPTags.h)），但 `namespace PersonalTags::Player` 仍在 [PP_BaseCharacter.h](../../Source/PersonalProjectCore/Public/Characters/PP_BaseCharacter.h) 声明、`UPP_AIPerceptionComponent.cpp` 和 `PP_PlayerCharacter.cpp` 两处使用（判断阵营用的 ActorTag）。
- 同一语义两套命名，新人不知道该用哪个。

### P3 · 转发头未拆干净
- 真身在 `Core/Public/Interaction/PP_InteractableInterface.h`，`Core/Public/Interfaces/PP_InteractableInterface.h` 是 forwarding header。
- `PP_EnemyCharacter.h/.cpp` 仍 include 旧路径 `Interfaces/...`，转发头**不能直接删**，需先改 2 处 include。

### P4 · EventBus 事件结构重复
- `FPP_BossDeadEvent` 与 `FPP_SettlementShowEvent` 字段完全相同（BossName/TimeToKill/GoldReward），在 [PP_EventTypes.h](../../Source/PersonalProjectCore/Public/Events/PP_EventTypes.h) 各写一份，后续每加字段要改两处。

### P5 · Core/Framework 目录垃圾桶化
一个 Framework/ 塞了 13 个互不相关的东西：
- 事件系统：EventBus、EventTypes
- 数据：DataManager
- 联机：SessionManager、LobbyActions、LobbyPlayerInfo、LobbyTypes、ScoreboardTypes
- 存档：LocalSaveGame
- 杂项：Log、GameFlowRequest、ItemUILibrary（名字叫 Item，实际是 GE 属性摘要 UI 工具）

后果：找文件靠猜、include 路径全部 `Framework/` 前缀失去语义。

### P6 · UI/Widgets 杂项目录
Crosshair/DamageNumber/DeathScreen/Notification/PauseMenu/Scoreboard/Settlement/AbilitySlot 8 个 Widget 与 HUD/、Lobby/ 等既有域目录无对应关系，新 Widget 不知道该进哪。

### P7 · PersonalProject/Game 目录混杂
CrashGameMode（主战斗）、LobbyGameMode（大厅）、GameInstance、EnemyRespawnManager 四个不相关角色挤在 Game/；GameObjects/ 里 Bonfire（交互物）、BossArena（关卡机关）、Projectile（投射物）、SoulPickup（掉落物）四种性质混放。

### P8 · 双事件机制无约定
同一事件既有 GameplayTag（`Events.Boss.Dead`、`Events.Enemy.Reset`）又有 EventBus 结构体。两者各有用途（Tag 给 GAS/AI 门控，EventBus 给跨系统通知），但**没有文档约定何时用哪个**，容易长出平行系统。

### P9 · HUD 调度器持续膨胀
APP_HUD 已有 16 个 WidgetClass 配置 + IPP_UIGateway 全量实现 + EventBus 订阅，每加一个界面都要改它。UIManager/ViewModel 机制已存在但 Widget 创建仍集中在 HUD。

---

## 三、目标架构

### 3.1 原则（不变项）

- **不新增模块**（5 模块规模够用）；依赖主线 `UI → PersonalProject → Quest → Inventory → Core` 不动。
- 接口下沉 Core 的模式保留并推广为标准：**跨模块调用一律走 Core 接口 + EventBus，禁止上层互相 include 实现**。
- 事件约定写进文档：
  - GAS/动画/AI 门控状态 → **GameplayTag**（PPTags 声明）
  - 跨系统通知/数据广播 → **EventBus 结构体**（EventTypes 声明）
  - 两者可并存，但语义必须在注释中互相引用。

### 3.2 Core 目录重组（Framework 拆分）

```
PersonalProjectCore/Public/
├── Framework/          # 只留真正的公共骨架
│   ├── PP_Log.h
│   ├── PP_DataManager.h
│   └── PP_GameFlowRequest.h
├── Events/             # 事件系统（原 Framework）
│   ├── PP_EventBus.h
│   └── PP_EventTypes.h
├── Session/            # 联机会话（原 Framework）
│   ├── PP_SessionManager.h
│   ├── PP_LobbyActions.h
│   ├── PP_LobbyPlayerInfo.h
│   ├── PP_LobbyTypes.h
│   └── PP_ScoreboardTypes.h
├── Save/               # 存档（原 Framework）
│   └── PP_LocalSaveGame.h
├── Interfaces/         # 所有 UInterface 归口（删除 Interaction/ 转发）
│   └── PP_InteractableInterface.h
├── Data/  Characters/  AbilitySystem/  AI/  GameplayTags/   # 不变
└── Query/              # Query/Actions 接口头（原散在 Inventory/Shop/Quest/）
    ├── PP_InventoryQuery.h / PP_InventoryActions.h
    ├── PP_ShopQuery.h / PP_ShopActions.h
    └── PP_QuestQuery.h
```

### 3.3 命名规范（目标态）

| 项 | 规范 | 迁移动作 |
|---|---|---|
| 类前缀 | `A/U/F` + `PP_`（现状保持） | — |
| Category | `PP\|域\|子域` | 89 处 `"Crash\|"` → `"PP\|"`（批量替换） |
| 主战斗 GameMode | `APP_GameMode`（PP_GameMode.h） | 改名 + CoreRedirects 兜底旧蓝图 |
| 文件头注释 | 删除课程模板行 | 批量清理 |
| Tag 命名空间 | 只保留 `PPTags` | `PersonalTags::Player` → `PPTags::ActorTag::Player` |
| GameplayTag 层级 | `Status.*`（状态）`Event.*`（事件广播）`PPAbilities.*`（能力标识） | 新增标签照此归类 |

### 3.4 UI/Widgets 归类

```
UI/Private|Public/
├── HUD/        # HUD 屏元素：AttributeBar、Attribute、TopBar、HUDWidget、HUD
├── Widgets/    # 战斗浮层：Crosshair、DamageNumber、Notification、AbilitySlot
├── Screens/    # 全屏界面：DeathScreen、PauseMenu、Scoreboard、Settlement（原 Widgets/）
└── Framework/ Lobby/ Inventory/ Quest/ Shop/ Dialogue/ Interaction/   # 不变
```

### 3.5 Game 目录拆分（PersonalProject 模块）

```
PersonalProject/
├── Game/          # GameMode 族：PP_GameMode（原 Crash）、PP_LobbyGameMode、PP_GameInstance
├── Managers/      # 世界管理组件：PP_EnemyRespawnManager（后续 LevelManager/SaveGameManager 也进这里）
└── GameObjects/   # 按性质分子目录：Bonfire/、Arena/、Pickup/、Projectile/
```

### 3.6 与后续功能开发的衔接

| 待做功能 | 架构落点 |
|---|---|
| F4 格挡弹反 | UPP_CombatStateComponent → Core/Characters/；新标签走 `Status.*` / `Event.Combat.*` |
| F6 角色差异化 | 纯蓝图，零 C++，无架构影响 |
| F7 篝火传送 | UPP_LevelManager → PersonalProject/Managers/；`Event.Bonfire.*` |
| F8 精英怪+多Boss | 纯蓝图子类，零 C++ |
| F9 存档 | UPP_SaveGameManager → Core/Save/（与 LocalSaveGame 同目录）；事件结构在 EventTypes 追加 |

---

## 四、分阶段迁移步骤（每步结束可编译、可单独 commit）

> 原则：只做移动/改名/删除，**不改任何运行逻辑**。每步完成后跑一次 Build.bat 验证。

### Step 1 · 清死代码与统一标签（低风险，半小时级）
1. `PP_EnemyCharacter.h/.cpp` 两处 include 改为 `Interaction/PP_InteractableInterface.h`。
2. 删除 `Core/Public/Interfaces/PP_InteractableInterface.h` 转发头。
3. `PersonalTags::Player` 迁入 `PPTags::ActorTag::Player`（PPTags.h/cpp 加声明，改 2 个使用点，删 BaseCharacter.h 里的 namespace 声明）。
4. 批量删除文件头 `// Chen Tianbao ...` 模板注释行。
- commit：`refactor(core): unify tag namespace and remove forwarding header`

### Step 2 · Category 批量改名（零逻辑风险）
1. 19 个头文件 `"Crash|` → `"PP|`（全局替换即可，编译无影响）。
- commit：`refactor: rename blueprint categories Crash| → PP|`

### Step 3 · Core/Framework 拆目录（中等风险，纯 include 路径替换）
1. 按上文 3.2 移动 13 个文件（Events/Session/Save/Query 子目录 + 保留项）。
2. 全局替换 include 路径（`Framework/PP_EventBus.h` → `Events/PP_EventBus.h` 等）。
3. 注意：SessionManager/LobbyTypes 等 Public 头被 Inventory/Quest/UI 引用，逐模块替换后编译。
- commit：`refactor(core): split Framework/ into Events/Session/Save/Query`
- ⚠️ 蓝图/DataTable 中引用了这些类不受影响（类路径靠 CoreObject，不靠 include 路径）。

### Step 4 · GameMode 改名 + CoreRedirects（需要蓝图配合）
1. `APP_CrashGameMode` → `APP_GameMode`，文件 PP_CrashGameMode.h/.cpp → PP_GameMode.h/.cpp。
2. `DefaultEngine.ini` 加：
   ```ini
   [CoreRedirects]
   +ClassRedirects=(OldName="/Script/PersonalProject.CrashGameMode", NewName="/Script/PersonalProject.GameMode")
   ```
3. 编辑器打开 BP_CrashGameMode 确认父类无损。
- commit：`refactor: rename CrashGameMode → GameMode with CoreRedirect`

### Step 5 · UI Widgets 归类 + Game 目录拆分（低风险）
1. Screen 类 Widget 移入 Screens/（DeathScreen/PauseMenu/Scoreboard/Settlement）。
2. EnemyRespawnManager 移入 Managers/；GameObjects 按性质分子目录。
3. include 路径全局替换。
- commit：`refactor(ui,game): reorganize widget and manager directories`

### Step 6 · EventBus 事件去重（小）
1. `FPP_SettlementShowEvent` 删除，SettlementWidget/HUD 改订 `FPP_BossDeadEvent`（字段一致）。
- commit：`refactor(core): merge duplicate settlement event struct`

### 明确不做（避免过度工程）
- 不动 EventBus 实现（类型擦除设计已够用）。
- 不拆 HUD（等 F9 存档界面后再评估 UIManager 接管创建）。
- 不把 ALS 依赖从 Core 摘出（BaseCharacter 需要它，代价远大于收益）。

---

## 五、建议执行顺序

**先做 Step 1~3（半天内可全部完成）**，它们只清理不碰行为，做完后 F4/F7/F9 开发就在干净目录上进行；Step 4~6 可以和 F4 并行（互不交叉文件）。
