# GAS：GameplayTag 与技能/效果规范

## 1. Tag 声明

只在这两个文件成对声明：

| 文件 | 内容 |
|---|---|
| `Source/PersonalProjectCore/Public/GameplayTags/PPTags.h` | `PERSONALPROJECTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Xxx);` |
| `Source/PersonalProjectCore/Private/GameplayTags/PPTags.cpp` | `UE_DEFINE_GAMEPLAY_TAG_COMMENT(Xxx, "PPTags.<Ns>.<Xxx>", "用途说明")` |

- **声明前必须加 `PERSONALPROJECTCORE_API`**。`UE_DECLARE_GAMEPLAY_TAG_EXTERN` 自身不带导出宏，跨 DLL 链接缺宏会 undefined symbol。
- `name` 用现有命名空间嵌套块（`namespace Status { ... }`），不要用新式 `namespace A::B` 混写。
- 完整路径固定 `PPTags.` 前缀，例如 `PPTags.Status.Invulnerable`。不要写成 `PP.Status.*`。
- 新增后需关编辑器完整编译（UHT 重新扫描），Live Coding 不生效。

现有分组（按需在对应组内新增，不新建顶层组）：

- `SetByCaller`（`Projectile`、`StaminaCost`、`Player.Secondary/Tertiary`）
- `PPAbilities`（`Primary/Secondary/Tertiary/Fourth/Fifth`、`Enemy.Attack`、`ActivateOnGiven`、`BlockHitReact`）
- `Events`（`KillScored`、`Inventory.EquipmentChanged`、`Player.*`、`Enemy.*`、`UI.*`、`Boss.Dead`、`Settlement.Show`）
- `Status`（`Dead`、`Sprinting`、`StaminaRegenBlocked`、`Boss`、`BossPhaseTransition`、`Invulnerable`、`BossArenaArmed`）

## 2. 添加 Tag 的步骤

1. 在 `PPTags.h` 对应 namespace 加 `PERSONALPROJECTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Xxx);`
2. 在 `PPTags.cpp` 同位置加 `UE_DEFINE_GAMEPLAY_TAG_COMMENT(Xxx, "PPTags.<Ns>.Xxx", "说明");`
3. 关编辑器 → Build.bat（见 `build-and-verify.md`）
4. 使用处 `AddLooseGameplayTag` / GE 授予
5. 注释写明**谁负责 Remove**

## 3. 生命周期

任何 AddTag 都必须有对应 Remove 路径：

| 加 Tag | 移除方 |
|---|---|
| `Status.Invulnerable`（翻滚开始） | 翻滚结束 Notify + 超时 Timer 兜底 |
| `Status.BossPhaseTransition` | 阶段切换 1.5s 后 Timer 移除 |
| `Status.StaminaRegenBlocked` | 由限时 GE 到期自动移除 |
| `Status.Sprinting` | 玩家 Tick 中 Gait 不再是 Sprint 时移除 loose tag |
| `Status.Dead` | 复活流程显式清理 |

典型 bug：只 Add 不 Remove → 玩家永久无敌 / Boss 永久免伤。

## 4. 使用规则

- 不要用 `GameplayTag::RequestGameplayTag(FName("字符串"), false)` 临时请求。
- 不要把 Tag 存进 ini 或 DataTable 的字符串列，必须用 `FGameplayTag` 类型。
- 不要在客户端 Add、只在服务端 Remove（不同步会永久卡死）。
- 持久状态用 GE 授予或 loose tag；一次性消息用 EventBus 事件结构体，不要用 Tag 当事件总线。
- `PP_BaseCharacter::CanBeDamaged` 统一检查 `Status.Invulnerable`，新伤害路径不必各自判无敌。

## 5. 技能（GA）

- 基类 `UPP_GameplayAbility`（`Source/PersonalProject/Public/AbilitySystem/Abilities/`）。
  - 消耗在 `CheckCost` / `ApplyCost` 里扩写，不要写进技能 Tick。
  - 每个执行分支最终必须 `EndAbility`；否则 ASC 卡住，同 GA 无法再激活。
- 授予入口只有两处：`BP_PP_PlayerCharacter` 的 `StartupAbilities`、`BP_CrashGameMode` 的 `CharacterAbilityMap`。
  - 同一 GA 两处都配 = 双实例 = 体力双倍消耗。新增角色专属技能从 `StartupAbilities` 移除，只走 `CharacterAbilityMap`。
- 监听类 GA（`GA_PP_ListenForKillScored`、`GA_PP_ListenForHealthChange`）Net Execution Policy 设 **Server Only**，避免客户端预测刷 `ClientActivateAbilityFailed`。
- 蒙太奇 Slot 必须 `Locomotion.Default`；`DefaultGroup.DefaultSlot` 会丢姿势。
- 角色专属技能槽位要与 C++ 基类一一对应，否则动画事件等待错误。
- GA 执行前若需要权威判定，先 `if (!HasAuthority()) return`；数值统一走 GE，不写死在 GA。

## 6. 游戏效果（GE）

- 耐力消耗：`GE_Cost_Stamina` = Instant，Modifier 为 `PP_AttributeSet.Stamina` Add (Base)，Magnitude 用带 Data Tag 的 SetByCaller，标签 `PPTags.SetByCaller.StaminaCost`。禁止旧式无标签 `SetByCallerFloat`。
- 耐力回复：`GE_StaminaRegen` / `GE_StaminaSprintDrain` 用 Infinite 周期 GE + `OngoingTagRequirements` 抑制，不写 Tick 手算。
- UE 5.7 注意：Grant Tags、Ongoing Tag Requirements 不再是 GE 本体字段，需通过 Components 添加（Grant Tags to Target Actor、Require Tags to Apply/Continue This Effect）。
- 休息 / 复活回满属性用代码直接赋值，不要用 Add 修饰符的 GE（会每次叠加）。
- 复活时清理死亡相关 GE 与 `Status.Dead`；只清客户端不够，服务端也要清。

## 7. AttributeSet 与 ASC

- `UPP_AbilitySystemComponent`、`UPP_AttributeSet` 在 `PersonalProjectCore/Public/AbilitySystem/`。
- 玩家：`PP_ALSPlayerCharacter` 继承 ALS 角色并实现 `IAbilitySystemInterface`，`PostInitializeComponents` 强制设置网格与动画蓝图。
- 敌人复用同一 ASC/AttributeSet；敌人不吃体力，靠韧性/架势。
