# 魂类战斗核心 · 设计方案

> 状态：背景资料 · 最后更新：2026-10-09 · 取代关系：无（战斗设计 spec，部分方案未落地）
> **日期**：2026-10-09
> **项目**：PersonalProject（UE 5.7 / C++ / GAS / UnLua / 联机）
> **目标**：在现有联机 GAS 项目上，注入一套魂类战斗骨架
> **周期**：约 4.5 周（+ 前置 3–5 天）
> **状态**：设计已确认，待实施

---

## 一、目标与范围

### 1.1 要达成什么

让项目具备魂类战斗的**核心循环**：

```
锁定 → 观察敌人前摇 → 选择（攻击 / 翻滚 / 格挡）→ 消耗耐力
    → 进入无敌帧或硬直 → 抓住反击窗口
```

具体交付：状态机 + 翻滚（含无敌帧）+ 耐力 + 锁定 + 韧性硬直 + 格挡弹反，**保留现有关卡与联机能力**。

**交付形态**：其中三个通用组件抽成独立插件 `ActionCombatCore`（见 3.4），使成果**脱离本项目也能存在** —— 可独立开源、复用到下一个项目。

### 1.2 明确不做

| 不做 | 原因 |
|---|---|
| 篝火 / 存档 / 检查点 | 属于"单机魂类"形态，不在"战斗核心"范围 |
| 关卡重做、Boss 设计 | 同上 |
| 敌人 AI 换行为树 | 现有 `UPP_SearchForTarget` 搜索-追击-攻击循环够用 |
| 联机预测 / 回滚 | 先走服务端权威，手感确认不行再议 |
| 玩家角色重做 / 美术资产替换 | 沿用 Greystone 现有素材 |

---

## 二、现状基线

### 2.1 已有资产（复用）

| 层 | 现状 | 位置 |
|---|---|---|
| 属性 | `Health / MaxHealth / Mana / MaxMana / Attack / Defense` | `Public/AbilitySystem/PP_AttributeSet.h` |
| 属性回调 | 仅 `PostGameplayEffectExecute`（**无** `PreAttributeChange`） | `Private/AbilitySystem/PP_AttributeSet.cpp:25-108` |
| 伤害结算 | `PP_DamageExecutionCalculation`：`max(Attack × 系数 − Defense, 1)` | `Private/.../Executions/PP_DamageExecutionCalculation.cpp:59-65` |
| 能力基类 | `UPP_GameplayAbility`（仅加调试屏显） | `Public/AbilitySystem/Abilities/PP_GameplayAbility.h` |
| 能力入口 | `TryActivateAbilitiesByTag` | `Private/Player/PP_PlayerController.cpp:159-166` |
| 输入 | Enhanced Input，绑定集中在 PlayerController | `Private/Player/PP_PlayerController.cpp:34-86` |
| ASC 挂载 | 玩家在 `PlayerState`（Mixed 复制）；敌人在自身（Minimal） | `PP_PlayerState.cpp:16-20` / `PP_EnemyCharacter.cpp:22-26` |
| 命中检测 | 球体重叠 / 骨骼 socket 扫描 | `PP_BlueprintLibrary.cpp` / `Notifies/PP_MeleeAttack.cpp:41` |
| 受击 | 四方向判定、击退、`BlockHitReact`（超霸体，不减伤） | `PP_BlueprintLibrary.cpp:15-46, 236-284`；`PP_Primary.cpp:47` |

### 2.2 素材基线（Greystone，145 个动画）

| 魂类要素 | 素材 | 状态 |
|---|---|---|
| 攻击连段 + 前后摇 | `Attack_PrimaryA/B/C`、`Attack_A~D_{Fast,Med,Slow}`、`*_Slow_Recovery` | ✅ |
| 格挡 / 弹反 | `Deflect_InPlace` | ✅ |
| 锁定姿态 | `RMB_Targeting`、`RMB_Targeting_MSA` | ✅ |
| 受击四方向 | `HitReact_Front/Back/Left/Right` | ✅ |
| 硬直 | `Stun_Start`、`Stun_Idle` | ✅ |
| 跳跃攻击 | `Jump_Melee` + `Jump_Start/Apex/Fall/Land/Recovery` | ✅ |
| 倒地 / 重生 | `Bound`、`Respawn` | ✅ |
| **翻滚 / 闪避** | —— | ❌ **唯一缺口** |
| **格挡被击** | —— | ❌ 缺口 |

素材根目录：`Content/ParagonGreystone/Characters/Heroes/Greystone/Animations/`
玩家已导入部分：`Content/PersonalProject/Characters/PlayerCharacter/Animations/Melee/`

### 2.3 必须一并处理的技术债

1. **三套伤害链路并存** —— 旧 `SetByCaller`（`PP_BlueprintLibrary.cpp:83-133`）、新 ExecCalc（`:135-174`）、普攻直挂 GE（`PP_Primary.cpp:132-136`）。魂类判定要插进伤害结算，必须先收敛成一条。
2. **无状态机** —— 状态是散装 `GameplayTags`，"翻滚中不能攻击""受击打断攻击"无处表达。
3. **`GE_InitializeAttributes` 玩家敌人共用** —— 直接加 `Stamina/Poise` 会同时改变双方，必须拆表。

### 2.4 拟引入的引擎插件

| 插件 | 类型 | 用途 | 引入方式 |
|---|---|---|---|
| **Motion Warping** | UE5 运行时插件 | 攻击吸附朝向目标、翻滚贴向输入方向、后续处决对齐目标位置 | `.uproject` 启用 + `MotionWarping` 加入 Build.cs 依赖 |
| **GameplayInsights** | UE5 编辑器插件 | 可视化 GAS 的标签 / GE / 技能时间线，排查"翻滚为何未触发" | `.uproject` 启用（仅 Editor target） |

两者均为**引擎自带**，无需第三方下载。

⚠️ Motion Warping **不是可选项**：不引入则攻击永远是"边跑边挥空"，翻滚也无法贴向目标方向。

---

## 三、架构设计

### 3.1 状态机：用 GAS 原生 Tag 机制，不自建状态类

**决定**：不写状态机类。`UGameplayAbility` 已内建状态机所需的全部语义：

| 需求 | GAS 原生字段 |
|---|---|
| 翻滚中不能攻击 | 攻击 GA 的 `ActivationBlockedTags = State.InAction.Rolling` |
| 受击打断攻击 | 攻击 GA 的 `CancelAbilitiesWithTag = State.InAction.HitReact` |
| 翻滚时自身处于什么状态 | 翻滚 GA 的 `ActivationOwnedTags = State.InAction.Rolling` |
| 死亡取消一切 | 所有 GA 配 `ActivationBlockedTags = State.Action.Dead` |

**状态标签集**（新增到 `Public/GameplayTags/PPTags.h` / `Private/GameplayTags/PPTags.cpp`）：

```
State.InAction              ← 根标签，聚合所有"正在做动作"的状态
State.InAction.Attacking
State.InAction.Rolling
State.InAction.Blocking
State.InAction.HitReact     （轻硬直）
State.InAction.Riposte      （弹反后大硬直，可处决）
State.Action.Dead           ← 独立于 InAction，死亡优先于一切

State.Invulnerable                        ← 无敌帧
State.InAction.Blocking.ParryWindow       ← 弹反判定窗口
State.Stamina.RegenBlocked                ← 耐力回复停顿（项目声明，见下）
```

**声明归属**：动作状态与无敌帧相关标签由插件 `ActionCombatCore` 以 Native GameplayTags 声明（见 3.4）；`State.Stamina.RegenBlocked` 属于项目侧概念（插件不涉及耐力），由项目在 `PPTags` / `DefaultGameplayTags.ini` 中声明。技能标签（`PPAbilities.*`）本就属项目，不受影响。

**语义约定**（避免歧义）：

- **"Idle" 不是一个真实标签**，而是"`State.InAction` 及其子标签均不存在"的状态。不声明 `State.Action.Idle`，因为动作类标签由 GA 的 `ActivationOwnedTags` 自动授予/撤销，多一个需要手动维护的 Idle 标签反而会制造不同步。动画蓝图需要时用 `ActionStateComponent::IsInState(State.InAction)` 取反判断，或在 ABP 里读 `State.InAction` 的缺席。
- **每个动作 GA 必须声明 `ActivationOwnedTags`**：攻击类 GA 授予 `State.InAction.Attacking`，翻滚授予 `...Rolling`，以此类推；GA 结束时 GAS 自动撤销。
- **`State.Action.Dead` 独立于 `InAction` 子树**，以便"死亡时取消一切"可以用单条 `ActivationBlockedTags = State.Action.Dead` 表达，不必逐个列举。

收益：**状态互斥是配置出来的，不是代码分支出来的**。新增动作（如弹反）只配 Tags，不动状态机代码。

### 3.2 `UActionStateComponent`：只补 GAS 缺的三件事

**类位于插件 `ActionCombatCore` 内**（见 3.4），不含任何项目依赖。
挂载点：`APP_BaseCharacter`（`Public/Characters/PP_BaseCharacter.h`），玩家与敌人共用。

职责**仅限**：

1. **输入缓冲** —— 魂类手感核心。攻击后摇中按下的翻滚指令排队（窗口 ≈ 0.2s），当前动作结束立即接上。
2. **行动预检** —— 统一判断"耐力够不够、当前状态是否允许"，避免每个 GA 重复实现。
3. **状态查询** —— 向动画蓝图暴露 `IsInState(FGameplayTag)`。

**关键约束**：该组件**不持有自己的状态**。真实状态唯一存在于 ASC 的 LooseGameplayTags。组件只是查询与缓冲的薄封装，从根本上杜绝"组件状态与 GAS 状态不同步"这一类 bug。

### 3.3 数据流

```
Enhanced Input（复用现有 IMC/IA 体系）
      ↓
APP_PlayerController::SetupInputComponent 绑定 IA_Roll / IA_Block / IA_LockOn
      ↓
UActionStateComponent::TryQueueAction(Tag)          ← 插件内
      ├ 输入缓冲
      ├ 状态互斥预检
      └ 耐力预检
      ↓
TryActivateAbilitiesByTag(PPAbilities.Player.Roll)     ← 沿用现有入口
      ↓
UPP_Roll（GameplayAbility，ServerOnly）
      ├ CommitAbility → GE_Cost_Roll 扣耐力 + GE_StaminaRegenDelay
      ├ ActivationOwnedTags: State.InAction.Rolling
      ├ 播放 AM_Roll 蒙太奇
      │      └ UInvulnerabilityWindow（AnimNotifyState）
      │             NotifyBegin → AddLooseGameplayTag(State.Invulnerable)
      │             NotifyEnd   → RemoveLooseGameplayTag
      └ ApplyRootMotionConstantForce 施加位移曲线
      ↓
伤害结算（服务端）→ PP_DamageExecutionCalculation 检查目标状态
```

### 3.4 插件边界：通用件抽成 `ActionCombatCore`

**决定**：把三个完全通用的组件抽成独立插件，项目特有的留在项目内。

| 组件 | 通用性 | 归属 |
|---|---|---|
| `UActionStateComponent`（输入缓冲 + 行动预检 + 状态查询） | 完全通用 | **→ 插件** |
| `UInvulnerabilityWindow`（`UAnimNotifyState`，无敌帧窗口） | 完全通用 | **→ 插件** |
| `ULockOnComponent`（锁定目标选择与相机接管） | 完全通用 | **→ 插件** |
| 伤害结算的通用判定逻辑 | 半通用（依赖项目属性名） | 留项目 |
| `PP_AttributeSet`、具体 GA（Roll/Block）、耐力 GE | 项目特有 | 留项目 |

**插件结构**：

```
Plugins/ActionCombatCore/
├── ActionCombatCore.uplugin
├── Source/ActionCombatCore/
│   ├── ActionCombatCore.Build.cs     （依赖 GameplayAbilities / GameplayTags / EnhancedInput）
│   ├── Public/
│   │   ├── Components/ActionStateComponent.h
│   │   ├── Components/LockOnComponent.h
│   │   ├── Notifies/InvulnerabilityWindow.h
│   │   └── ActionCombatTags.h        （Native GameplayTags 声明）
│   └── Private/
└── (不打包任何资产)
```

**标签方案（关键设计决定）**：核心状态标签由插件用 **Native GameplayTags** 自行声明，项目直接引用 —— 插件自洽，消费者无需在 `DefaultGameplayTags.ini` 重复声明。

```cpp
// ActionCombatTags.h
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_InAction);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_InAction_Rolling);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invulnerable);
// ...
```

**项目特有标签通过配置注入**：

- `UActionStateComponent` 暴露 `UPROPERTY(EditDefaultsOnly) FGameplayTagContainer BlockingActionTags;` 等
- `ULockOnComponent` 暴露 `UPROPERTY(EditDefaultsOnly) FGameplayTag TargetActorTag;`（项目填 `Character.Enemy`）

**耐力预检的注入方式**：耐力是项目属性，插件不应知道。`UActionStateComponent` 暴露

```cpp
UPROPERTY(BlueprintAssignable)
FOnValidateAction OnValidateAction;   // 任一绑定返回 false 即拒绝该动作
```

项目在 `APP_BaseCharacter` 里绑上"耐力是否足够"的检查。这样插件的预检机制通用，判定内容由项目提供。

**边界规则（硬约束）**：

1. 插件内**不得**出现任何 `PP_` 前缀符号
2. 插件**不得** include 项目头文件；依赖方向单向：项目 → 插件
3. 插件不引用 `PPTags.h`

**收益**：可独立开源（喂"个人 IP"线）、可复用到下一个项目、边界强制解耦。
**代价**：多维护一个 `.uplugin` 与 Build.cs；"快速 hack"变麻烦（这正是想要的约束）。

---

## 四、系统设计

### 4.1 伤害链路统一（地基，最先做）

所有伤害来源收敛到 `UPP_BlueprintLibrary::ApplyDamageEffectFromCauser` + `PP_DamageExecutionCalculation`：

| 现有调用方 | 改为 |
|---|---|
| `UPP_Primary` 直挂 GE（`PP_Primary.cpp:132-136`） | `ApplyDamageEffectFromCauser` |
| `UPP_Secondary` SetByCaller（`PP_Secondary.cpp:132-139`） | 同上 |
| `APP_Projectile` 敌人远程（`PP_Projectile.cpp:34`） | 同上 |
| `APP_FireballProjectile` | 已在新链路，无需改 |
| 敌人近战/远程蓝图 GA | 同上 |

⚠️ 约束：spec 必须建在**施法者** ASC 上（`ApplyDamageEffectFromCauser` 已满足；旧的 `SendDamageEventToPlayer` 建在目标 ASC 上，只适用固定负值伤害，不可复用）。

**扩展 `PP_DamageExecutionCalculation`**，在现有公式之上叠加三层判定：

```
输入：Attack(snapshot) × Coefficient、Defense(实时)
     + SetByCaller.PoiseDamage

1. 目标含 State.Invulnerable？
     → 伤害 0，PoiseDamage 0，直接返回
2. 目标含 State.InAction.Blocking 且正面命中（攻击者方向 · 目标前向 > 0）？
     → 伤害 × (1 − BlockReduction)
     → 若同时含 State.InAction.Blocking.ParryWindow → 判定弹反（见 4.6）
     → 扣目标耐力与 Poise
3. 正常结算：FinalDamage = max(Attack × Coefficient − Defense, MinimumDamage)
     → 输出到 Health（Additive 负值）
     → 同时输出 PoiseDamage 到 Poise
```

### 4.2 耐力 Stamina

- `PP_AttributeSet` 新增 `Stamina` / `MaxStamina`（含 `OnRep` + 复制，沿用现有模式）
- **回复用 GAS 机制，不写计时器**：`GE_StaminaRegen`（Infinite + Period 0.1s）配 `OngoingTagRequirements.IgnoreTags = State.Stamina.RegenBlocked` —— 标签存在时效果自动暂停
- **魂类手感关键（消耗后停顿）**：每次消耗同时施加 `GE_StaminaRegenDelay`（Duration ≈ 0.5s），授予 `State.Stamina.RegenBlocked`
- 消耗走现有 `CommitAbility` Cost GE 模式（参考 `GE_Cost_Secondary`）：翻滚 25、轻攻击 15、重攻击 25、格挡被击按伤害比例
- 耐力不足 → `ActionStateComponent` 预检拦截
- UI：复用 `Public/UI/PP_AttributeBarWidget.h`，加一根耐力条

### 4.3 翻滚 Roll

- 新增 `IA_Roll`，按现有模式绑在 `APP_PlayerController::SetupInputComponent`（`PP_PlayerController.cpp:49-85`）
- `UPP_Roll : UPP_GameplayAbility`：
  - `ActivationOwnedTags = State.InAction.Rolling`
  - `ActivationBlockedTags = State.InAction.Rolling / State.InAction.HitReact / State.InAction.Riposte / State.Action.Dead`
  - Cost GE 扣耐力 + 施加回复停顿
- **无敌帧**：`UInvulnerabilityWindow : UAnimNotifyState`（**插件内**），写法照抄现有 `UPP_MeleeAttack`（`Public/Notifies/PP_MeleeAttack.h` / `Private/Notifies/PP_MeleeAttack.cpp`），只是把"发事件"换成"挂/摘 `State.Invulnerable` 标签"。窗口置于蒙太奇前 1/3 段。
- **位移**：Mixamo 翻滚为原地动画，用 GAS 自带的 `ApplyRootMotionConstantForce` + 曲线推进。
  **不使用 `LaunchCharacter`** —— 它中途不可取消，翻滚会被卡住。
- **贴向目标方向**：起手用 **Motion Warping**（`UMotionWarpingComponent` + `AN_MotionWarpingWindow`）把角色旋转吸附到翻滚方向，消除"先转身再滚"的僵直感
- **方向决策**：锁定中 → 相对目标方向；未锁定 → 输入方向；无输入 → 向后
- **取消窗口**：翻滚后段允许接攻击（魂类经典），通过蒙太奇 Section + `CancelAbilitiesWithTag` 实现

### 4.4 锁定 Lock-on

- `ULockOnComponent`（**位于插件** `ActionCombatCore`，见 3.4）挂 `APP_PlayerCharacter`
- 通过 `TargetActorTag` 配置注入"谁是敌人"（插件不硬编码项目标签）
- 输入：`IA_LockOn`（Toggle）+ 复用 `IA_Look` 水平轴切换目标
- 选目标：复用 `UPP_BlueprintLibrary::FindClosestActorWithTag`（`PP_BlueprintLibrary.cpp`），改为"前方向锥内最近"
- **不新增相机**：保留现有 SpringArm（`PP_PlayerCharacter.cpp:37-44`），锁定仅每帧插值接管 control rotation 朝向目标
- 角色朝向：`bOrientRotationToMovement`（`PP_PlayerCharacter.cpp:27-34`）按锁定状态动态切换
- 解除条件：目标死亡 / 超出距离阈值 / 手动取消 / 目标丢失
- **不复制**：锁定为纯客户端表现，目标本地选择，联机成本为零

### 4.5 韧性与硬直 Poise

- 新增 `Poise` / `MaxPoise`
- 伤害结算时扣 Poise，**归零才出硬直**（魂类标准：普通命中只掉血，不打断动作）
- 硬直触发：发送 GameplayEvent → `UPP_HitReact` 挂 `State.InAction.HitReact`，并通过 `CancelAbilitiesWithTag` 打断当前动作
- 分级：普攻低 Poise 伤害；重击 / 弹反直接破防并进入 `State.InAction.Riposte`（大硬直，可处决）
- Poise 归零重置为 Max，延迟 1s 后开始回复（与耐力回复停顿同构）
- **与现有 `BlockHitReact` 的关系（重要）**：`GE_BlockHitReact` 目前被 Primary/Secondary/Tertiary 在施法时挂上（`PP_Primary.cpp:47`、`PP_Secondary.cpp:39`、`PP_Tertiary.cpp:35`），语义是"攻击期间免疫受击反应"。改造后它**不再是独立机制**，而是"攻击期间 Poise 不可被击空"的实现——对外语义保留，实现并入 Poise 系统。
- 硬直动画现成：`HitReact_Front/Back/Left/Right`、`Stun_Start`、`Stun_Idle`

### 4.6 格挡与弹反

- `IA_Block`（Hold）→ `UPP_Block : UPP_GameplayAbility`
  - `ActivationOwnedTags = State.InAction.Blocking`
  - `ActivationBlockedTags = State.InAction.Rolling / State.InAction.HitReact / State.InAction.Riposte / State.Action.Dead`
  - 循环播放 `Deflect_InPlace`
- **减伤**：ExecCalc 检测目标 `State.InAction.Blocking` 且正面命中 → 伤害 × (1 − BlockReduction)，同时扣耐力与 Poise
- **弹反**：格挡起始后 **0.3s** 内被命中 → 不扣血，攻击方进入 `State.InAction.Riposte`
  - 实现：格挡 GA 开始时施加一个短 Duration 的 GE，授予 `State.InAction.Blocking.ParryWindow`；ExecCalc 命中该标签即判弹反
  - 素材即 `Deflect_InPlace`
- ⚠️ 缺口：无"格挡被击"动画，与翻滚一并通过 Mixamo 补充，或先用 `Deflect_InPlace` 凑

### 4.7 攻击前后摇与取消窗口

- 现有攻击为"蒙太奇 + AnimNotify 触发判定"，**无前后摇概念**，这是手感最大短板
- 蒙太奇划分 Section：`Windup` / `Active` / `Recovery`；`Recovery` 段允许取消到翻滚或接下一段
- 取消规则配置在 GA 的 `CancelAbilitiesWithTag`
- **攻击吸附**：攻击 GA 起手用 Motion Warping 将角色旋转吸附向锁定目标或输入方向，解决"边跑边挥空"。这是魂类打击感的关键一环，也是 2.4 引入 Motion Warping 的主要理由
- **素材直接可用**：`Attack_A~D_{Fast,Med,Slow}` + `*_Slow_Recovery` 即"同一招的快/中/慢 + 长后摇"四态

### 4.8 属性表拆分（避免玩家敌人共变）

现状：`GE_InitializeAttributes` 为玩家与所有敌人共用。

改造：拆为
- `GE_InitializeAttributes_Base` —— 基础共用值（Health / Mana / Attack / Defense 基准）
- 各角色类型各自覆盖 `Stamina / MaxStamina / Poise / MaxPoise`

---

## 五、实施阶段

### 前置 · 打包与缺陷修复（3–5 天，不占魂类预算）

打通 Development 打包 → 复现并修复 #6（打包后角色技能静默丢失）。
**理由**：每阶段的验证都依赖打包；若不先修，#6 会伪装成"新写的翻滚不生效"。

### 阶段 1 · 插件骨架 + 地基（约 1.5 周）

**先立插件边界，再填内容** —— 顺序重要：先有边界，后面写的东西才不会往项目里乱塞。

1. 建 `ActionCombatCore` 插件骨架（`.uplugin` + `Build.cs` + `ActionCombatTags.h` 的 Native GameplayTags）
2. `UActionStateComponent`（含 `OnValidateAction` 委托，供项目注入耐力检查）
3. `UInvulnerabilityWindow`（本阶段只需可编译，阶段 2 接线使用）
4. `.uproject` 启用 **MotionWarping** 与 **GameplayInsights**；`MotionWarping` 加入 Build.cs 依赖
5. 统一伤害链路（三套 → ExecCalc 一条）
6. `Stamina` / `Poise` 属性 + 拆开共用的 `GE_InitializeAttributes`

- **验证**：插件可被项目正常引用并编译；一次攻击经 ExecCalc 扣血；HUD 耐力条会掉、会停顿、会回复；Motion Warping 组件挂到角色上能生效
- **产出**：《我把三套伤害链路收敛成了一套》+《为什么我把战斗组件抽成了插件》

### 阶段 2 · 翻滚（约 1 周）

翻滚动画重定向（**先评估 Epic Game Animation Sample 是否可用，再落到 Mixamo**）、`IA_Roll` + `UPP_Roll`、`UInvulnerabilityWindow` 接线、`ApplyRootMotionConstantForce` 位移 + Motion Warping 贴向、输入缓冲接入 `UActionStateComponent`。

- **验证**：翻滚能吃掉火球（命中时伤害为 0）；翻滚贴向输入/锁定方向而非"先转身再滚"；耐力扣减与停顿回复正确；后摇能接下一段
- **产出**：《给联机 GAS 项目加无敌帧：蒙太奇窗口 + ExecCalc 拦截》

### 阶段 3 · 锁定与攻击节奏（约 1 周）

`ULockOnComponent` 加入插件 + `IA_LockOn`（`TargetActorTag` 由项目注入）；攻击蒙太奇划 Section + 取消窗口；攻击 Motion Warping 吸附。

- **验证**：锁定后镜头跟随不晕；攻击会吸附朝向锁定目标而非原地挥空；攻击后摇能取消接翻滚
- **产出**：《锁定系统怎么做才不晕：只接管 control rotation》

### 阶段 4 · 韧性与格挡弹反（约 1 周）

Poise 结算 + 分级硬直 + `BlockHitReact` 并入；`IA_Block` + `UPP_Block` + 0.3s 弹反窗口。

- **验证**：普通命中不打断敌人，攒够韧性伤害才出硬直；弹反成功敌人进 `Riposte`
- **产出**：《从"超级护甲"到韧性系统：一次战斗机制的拆解》

**总计约 4.5 周**（不含前置）。

### 计划粒度说明

本 spec 覆盖 4 个独立可交付的阶段，**不建议写成单一实施计划**。进入 writing-plans 时按阶段拆分：先只出**阶段 1（插件骨架 + 地基）**的计划并实施，完成后依据实际结果再出阶段 2 的计划。理由：阶段 2 的翻滚重定向结果会显著影响阶段 3/4 的细节（例如若重定向失败退回垫步方案，则取消窗口的配置要跟着调），提前细化后续阶段属于无效工作。

---

## 六、验证标准

**每阶段必须同时满足**：

1. 编辑器 PIE 中功能可复现
2. **打包后同样可复现**（防止重蹈 #6）
3. **插件边界未被破坏** —— `ActionCombatCore` 内不含任何 `PP_` 前缀符号、不 include 项目头文件（可 grep 校验）
4. 已产出一篇文章

未满足第 4 条视为阶段未完成。

---

## 七、风险与止损

| 风险 | 止损 |
|---|---|
| Mixamo 重定向踩坑（Paragon 骨架 ≠ Mixamo，比例与手指需调） | **3 天止损**，超时立即退回"用 `Deflect_InPlace` 做垫步" |
| 联机下翻滚有输入延迟感（服务端权威） | 先按服务端权威实现；手感确认不可接受再考虑预测，不提前优化 |
| `GE_InitializeAttributes` 共用导致玩家敌人一起变 | 阶段 1 必须完成拆表 |
| #6 伪装成新 bug | 前置阶段先修复；每阶段打包验证 |
| 插件边界拖慢"快速试错" | 试错性代码先写在项目侧，定型后再上移插件；不为了边界牺牲验证速度 |
| GASP 动画集不满足需求或授权不明 | 3 天内评估完，不满足则直接转 Mixamo，不纠缠 |
| 范围蔓延 | 每阶段"打包通过 + 文章发布"才算完 |

---

## 八、涉及文件清单

### 插件内新增（`Plugins/ActionCombatCore/`）

```
ActionCombatCore.uplugin
Source/ActionCombatCore/ActionCombatCore.Build.cs
Source/ActionCombatCore/Public/ActionCombatTags.h
Source/ActionCombatCore/Public/Components/ActionStateComponent.h
Source/ActionCombatCore/Private/Components/ActionStateComponent.cpp
Source/ActionCombatCore/Public/Components/LockOnComponent.h
Source/ActionCombatCore/Private/Components/LockOnComponent.cpp
Source/ActionCombatCore/Public/Notifies/InvulnerabilityWindow.h
Source/ActionCombatCore/Private/Notifies/InvulnerabilityWindow.cpp
```

⚠️ 插件内所有类不带 `PP_` 前缀，且不得 include 任何项目头文件。

### 项目内新增

```
Source/PersonalProject/Public/AbilitySystem/Abilities/Player/PP_Roll.h
Source/PersonalProject/Private/AbilitySystem/Abilities/Player/PP_Roll.cpp
Source/PersonalProject/Public/AbilitySystem/Abilities/Player/PP_Block.h
Source/PersonalProject/Private/AbilitySystem/Abilities/Player/PP_Block.cpp
```

### 配置变更

```
PersonalProject.uproject                        （启用 MotionWarping / GameplayInsights）
Source/PersonalProject/PersonalProject.Build.cs （+ MotionWarping 依赖；插件由 .uproject 自动纳入）
Config/DefaultGameplayTags.ini                  （+ State.Stamina.RegenBlocked）
```

### 修改

```
Source/PersonalProject/Public/AbilitySystem/PP_AttributeSet.h          （+ Stamina / Poise）
Source/PersonalProject/Private/AbilitySystem/PP_AttributeSet.cpp
Source/PersonalProject/Public/GameplayTags/PPTags.h                   （+ State.Stamina.RegenBlocked，动作标签由插件声明）
Source/PersonalProject/Private/GameplayTags/PPTags.cpp
Source/PersonalProject/Public/Characters/PP_BaseCharacter.h           （挂 ActionStateComponent）
Source/PersonalProject/Public/Characters/PP_PlayerCharacter.h         （挂 LockOnComponent）
Source/PersonalProject/Private/Player/PP_PlayerController.cpp         （绑定 IA_Roll / IA_Block / IA_LockOn）
Source/PersonalProject/Public/Player/PP_PlayerController.h
Source/PersonalProject/Public/Utils/PP_BlueprintLibrary.h
Source/PersonalProject/Private/Utils/PP_BlueprintLibrary.cpp
Source/PersonalProject/Private/AbilitySystem/Executions/PP_DamageExecutionCalculation.cpp
Source/PersonalProject/Private/AbilitySystem/Abilities/Player/PP_Primary.cpp
Source/PersonalProject/Private/AbilitySystem/Abilities/Player/PP_Secondary.cpp
Source/PersonalProject/Public/GameObjects/PP_Projectile.h / .cpp
```

### 资产（Content）

```
新增 Input Action：Content/PersonalProject/Input/IA_Roll、IA_Block、IA_LockOn
新增 GameplayEffect：GE_Cost_Roll、GE_Cost_Block、GE_StaminaRegen、
                     GE_StaminaRegenDelay、GE_BlockReduction、GE_Block_ParryWindow、
                     GE_InitializeAttributes_Base
新增 Montage：AM_Roll、AM_Block_Hit
新增 AnimNotifyState：AN_InvulnerabilityWindow（类来自插件，此处仅为资产实例）
```

Motion Warping 的窗口通知使用引擎自带的 `UAnimNotifyState_MotionWarping`，无需新增资产。

---

## 九、一句话总结

**不动状态机轮子，用 GAS 原生 Tag 机制表达状态；通用件抽成 `ActionCombatCore` 插件以强制解耦并独立开源；先把三套伤害链路收敛成一条，再往这条链路上叠无敌帧、减伤与韧性，用 Motion Warping 补上打击感。** 每阶段能独立打包、独立验证、独立成文。
