# ALS 集成 Bug 修复复盘

> 日期：2026-10-22 | 模块：ALS (Advanced Locomotion System) + GAS 集成
> 目标：把 ALS 全面集成到已有五模块 GAS 项目

---

## 一、Bug 全景图（按发现顺序）

```
改父类 (ACharacter → AAlsCharacter)
    │
    ├── 1. 鼠标视角完全不动 ────── Tick 守卫 AnimationInstance=null ── 强制 SetAnimInstanceClass(AB_Als)
    ├── 2. 技能/UI 全失效 ──────── PC 侧 AddMappingContext 被注释 ──── 恢复 priority=1
    ├── 3. Scoreboard 抢输入 ──── Overlay层+ConsumeAll+初始可见 ──── bConsumesAllInput=false
    ├── 4. HUD 可能拦事件 ──────── 全屏 Canvas 默认 Visible ───────── HitTestInvisible
    ├── 5. 上下视角反 ──────────── Value.Y 正负号 ───────────────────── -Value.Y
    └── 6. Settings null 兜底 ── 蓝图覆盖 C++ 默认值 ──────────────── PostInit 最终兜底
```

---

## 二、每个 Bug 的完整技术细节

### Bug 1：鼠标视角完全不动（核心问题）

**现象**：PIE PPMap 后晃鼠标，视角完全没反应；WASD 正常。

**排查过程**：

| 步骤 | 做了什么 | 结果 |
|---|---|---|
| 1 | Output Log 搜 `Input_OnLookMouse` | **疯狂触发**，X/Y 值正常（-4.76~+4.83） |
| 2 | 确认 EnhancedInput 链路 | ALS IMC priority=0 正确，IA_Als_LookMouse 绑 Mouse2D |
| 3 | 看 `AddControllerYawInput/PitchInput` | 调了，但视角不动 |
| 4 | 查 ALS 相机怎么取视角 | `UAlsCameraComponent::TickCamera` → `Character->GetViewRotation()` |
| 5 | `GetViewRotation()` 返回什么 | ALS 重写的 → `return ViewState.Rotation` |
| 6 | ViewState.Rotation 谁更新 | `AAlsCharacter::RefreshView` → 拷 `Super::GetViewRotation()` 进来 |
| 7 | RefreshView 为什么没跑 | ALS Tick 开头有守卫 |
| 8 | Tick 守卫内容 | `if (!IsValid(Settings) \|\| !AnimationInstance.IsValid()) return;` |
| 9 | Settings 有吗 | `Settings OK: CS_Als_Default` ✅ |
| 10 | AnimationInstance？ | 动画蓝图是 **Greystone_AnimBlueprint** 不是 ALS 的 → `Cast<UAlsAnimationInstance>` 失败 → **null** ❌ |

**根因链**：

```
蓝图 BP_PP_PlayerCharacter 继承自 BP_Als_Character
    → 改父类到 C++ PP_BaseCharacter
    → 蓝图里 Mesh 的 AnimClass 残留 Greystone_AnimBlueprint（Paragon 资源）
    → AAlsCharacter::PostInitializeComponents()
        AnimationInstance = Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance())
        → Greystone Anim Instance 不是 UAlsAnimationInstance
        → AnimationInstance = TWeakObjectPtr(nullptr)
    → AAlsCharacter::Tick() 守卫 return
    → RefreshView 永远不执行
    → ViewState.Rotation 永远是 PreRegisterAllComponents 时的初始 Actor Rotation
    → UAlsCameraComponent 读 ViewState.Rotation → 永远不变
    → 视角不动
```

**修复**：PP_BaseCharacter::PostInitializeComponents 强制覆盖

```cpp
// 之前的兜底（条件太松）
if (!GetMesh()->GetAnimClass())
    SetAnimInstanceClass(AB_Als);  // Greystone 不是 null → 跳过

// 修复后（强制覆盖）
static UClass* CachedDefaultAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
if (CachedDefaultAnimClass && GetMesh()->GetAnimClass() != CachedDefaultAnimClass)
{
    UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] Overriding AnimClass %s → AB_Als"), *GetNameSafe(GetMesh()->GetAnimClass()));
    GetMesh()->SetAnimInstanceClass(CachedDefaultAnimClass);
}
```

**为什么 ALS 测试地图没问题**：测试地图的角色蓝图从一开始就是 ALS 体系的，Anim BP 天然是 AB_Als，没残留。

---

### Bug 2：技能/UI 全失效

**根因**：之前为排查问题临时注释了 PC 侧 AddMappingContext，恢复后忘了取消注释。

**修复**：恢复 AddMappingContext，priority=1

```cpp
for (UInputMappingContext* Context : InputMappingContexts)
{
    InputSubsystem->AddMappingContext(Context, 1);  // ALS IMC 是 priority=0
}
```

**Priority 分层逻辑**：

| IMC | Priority | 负责 | 按键 |
|---|---|---|---|
| ALS `IMC_Als_Default` | **0**（最高） | 视角/移动 | 鼠标 XY、WASD、Space、Shift |
| PC 侧 `IMC_Movement` | **1** | 技能/UI | 鼠标左右键、1-5、Esc、Tab、I、J |

两者按键完全不冲突，priority=0 保证 ALS 鼠标路由永远优先。

---

### Bug 3：Scoreboard 抢输入模式

**现象**：UIManager 日志显示 `Top 覆盖层 layer: WBP_Scoreboard ConsumeAll=YES Visible=YES → GameAndUI`

**根因**：WBP_Scoreboard 初始可见（蓝图配置）、Overlay 层、`bConsumesAllInput=true`（基类默认值）。RecalculateInputMode 扫描到 Overlay 层有可见+ConsumeAll 的 Widget → 切 GameAndUI → `EMouseLockMode::DoNotLock` + `bShowMouseCursor=true` → EnhancedInput Mouse2D 在光标未锁定时不产生有效 delta。

**修复**：

```cpp
// PP_ScoreboardWidget.cpp 构造函数
bConsumesAllInput = false;  // 计分板是纯展示型，不该拦截输入
```

---

### Bug 4：常驻 HUD 可能 Slate Hit Test 拦截

**现象**（低概率）：AddToViewportByLayer 只 AddToViewport 不设 Visibility → 默认 `ESlateVisibility::Visible` → 全屏 Canvas 盖顶层 → OnMouseMove 被 Slate 拦截。

**修复**：

```cpp
void APP_HUD::AddToViewportByLayer(UPP_UIUserWidget* Widget)
{
    Widget->AddToViewport(static_cast<int32>(Widget->GetOwningLayer()));
    // Gameplay/Notification 层（< Modal=40）默认 HitTestInvisible
    // Modal/Overlay 层保持 Visible
    if (Widget->GetOwningLayer() < EPP_UILayer::Modal
        && Widget->GetVisibility() != ESlateVisibility::Hidden)
    {
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
```

---

### Bug 5：上下视角反了

**根因**：UE EnhancedInput 的 Mouse2D 轴语义——鼠标下移 → Value.Y 为**正**；`AddControllerPitchInput` 正角 → **抬头**。ALS 测试地图蓝图里配了 `LookUpMouseSensitivity=-1.0`，我们没配（默认 1.0）。

**修复**：

```cpp
AddControllerPitchInput(-Value.Y * LookUpMouseSensitivity);  // 加负号
AddControllerYawInput(Value.X * LookRightMouseSensitivity);
```

---

### Bug 6：Settings/MovementSettings 兜底

**根因**：蓝图属性值应用在 C++ 构造函数之后——如果 BP 里 Settings 配 null，会覆盖 C++ 默认值。

**修复**：PostInitializeComponents 里最终兜底（时机最晚，蓝图已应用完毕）

```cpp
void APP_BaseCharacter::PostInitializeComponents()
{
    if (!Settings)
    {
        Settings = LoadObject<UAlsCharacterSettings>(nullptr, TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
    }
    if (!MovementSettings)
    {
        MovementSettings = LoadObject<UAlsMovementSettings>(nullptr, TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
    }
    Super::PostInitializeComponents();
}
```

**注意**：LoadObject 不能在构造函数里调（CDO 阶段资产未加载），所以用 FObjectFinder 兜底。PostInitializeComponents 时机合适。

---

## 三、关键技术知识点

### ALS 视角链路 vs UE 默认

```
UE 默认视角链路:
    鼠标 XY → EnhancedInput → AddControllerYaw/PitchInput
    → PlayerController.ControlRotation 直接更新
    → UCameraComponent 读 ControlRotation → 渲染

ALS 视角链路:
    鼠标 XY → EnhancedInput → AddControllerYaw/PitchInput
    → PlayerController.ControlRotation 更新
    → Tick → RefreshView → Super::GetViewRotation() (= ControlRotation)
    → ViewState.Rotation 更新 (含网络平滑)
    → UAlsCameraComponent 读 Character->GetViewRotation() (= ViewState.Rotation) → 渲染
```

### Tick 守卫设计

ALS Tick 开头守卫：

```cpp
void AAlsCharacter::Tick(const float DeltaTime)
{
    if (!IsValid(Settings) || !AnimationInstance.IsValid())
    {
        Super::Tick(DeltaTime);  // 只跑基类，RefreshView/Locomotion 全部跳过
        return;
    }
    // ... 完整 ALS Tick 逻辑
}
```

**为什么两个条件**：
- `Settings`：UAlsCharacterSettings 包含 TeleportDistanceThreshold 等运行期配置
- `AnimationInstance`：AAlsCharacter::PostInitializeComponents 用 `Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance())` 赋值。只有 Anim BP 继承自 UAlsAnimationInstance 才有效。

### 改父类蓝图属性丢失陷阱

```
蓝图属性生命周期:
1. C++ CDO 构造函数运行 → C++ 默认值
2. 蓝图加载 → BP 覆盖值（如果有）
3. PostInitializeComponents → 组件初始化
```

**改父类后蓝图会：**
- 保留旧父类的组件（如 Paragon 的 Greystone SkeletalMesh）→ 需要兜底
- 丢失新父类的 UPROPERTY（如 Settings 如果是新属性但蓝图显式覆盖了 null）→ 需要最终兜底
- 但输入资产（IMC、IA）因为是 UPROPERTY 且蓝图可编辑，一般能保留引用

### EnhancedInput Priority 机制

```
AddMappingContext(IMC, Priority)
    Priority 越大，越后处理（后处理的会覆盖前面的同 InputAction 映射）
    Priority 0 = 最先处理 = 最低优先级？不！
    实际上：Priority 0 是最高优先级的上下文，先处理
           同一 InputAction 在低 Priority 的上下文中也能触发（只要没被 bBlockInputActions=true）
```

**实战：ALS 用 0 最高优先，PC 侧用 1 不冲突的按键**

### FInputModeGameOnly vs GameAndUI

| 模式 | MouseLockMode | bShowMouseCursor | 鼠标 XY delta | 适用场景 |
|---|---|---|---|---|
| GameOnly | LockOnCapture | false | ✅ 正常产生 | 战斗中，无 UI 覆盖 |
| GameAndUI | DoNotLock | true | ❌ 光标未锁定 | 暂停/商店/背包 |
| UIOnly | - | true | ❌ 完全禁用 | 纯 UI（对话确认） |

### RecalculateInputMode 扫描逻辑

```cpp
void UPP_UIManager::RecalculateInputMode()
{
    constexpr EPP_UILayer Layers[] = { Modal, Overlay };  // 只扫这两层
    for (EPP_UILayer Layer : Layers)
    {
        Top = GetTopWidgetInLayer(Layer);
        if (Top && Top->DoesConsumeAllInput() && Top->IsVisible())
        {
            // Modal → UIOnly, Overlay → GameAndUI
            return;
        }
    }
    // Gameplay 层 Widget 不算 → 即使可见也保持 Gameplay
}
```

---

## 四、调试方法总结

### 如何确认 EnhancedInput 有没有收到事件

在 Input_OnLookMouse 里加 UE_LOG：

```cpp
void APP_PlayerCharacter::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
    const FVector2f Value{ActionValue.Get<FVector2D>()};
    UE_LOG(LogPP, Log, TEXT("[ALS_Input] Input_OnLookMouse  X=%.3f Y=%.3f (Sens R=%.2f U=%.2f)"), 
        Value.X, Value.Y, LookRightMouseSensitivity, LookUpMouseSensitivity);
    AddControllerPitchInput(-Value.Y * LookUpMouseSensitivity);
    AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}
```

### 如何确认 InputMode 是什么

在 RecalculateInputMode 里加日志（临时），或者控制台 `ShowDebug PlayerController`。

### 如何确认 Tick 守卫有没有 return

在 ALS Tick 守卫 return 前加日志（需要改插件源码，不如在 PP_BaseCharacter PostInit 里加 Settings/AnimationInstance 的验证日志快）。

### 如何确认蓝图属性覆盖值

蓝图编辑器 Class Defaults 里直接看：SkeletalMesh、Anim Class、Settings、MovementSettings 是不是配的非空值。

---

## 五、修改的文件清单

| 文件 | 类型 | 修改内容 |
|---|---|---|
| `Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp` | **核心** | Settings/MovementSettings PostInit 兜底 + AnimClass 强制覆盖 AB_Als + 日志 |
| `Source/PersonalProject/Private/Characters/PP_PlayerCharacter.cpp` | 输入 | Input_OnLookMouse Pitch 加负号 + 诊断日志 |
| `Source/PersonalProject/Private/Player/PP_PlayerController.cpp` | 输入 | 恢复 AddMappingContext，priority=1 |
| `Source/PersonalProjectUI/Private/Widgets/PP_ScoreboardWidget.cpp` | UI | bConsumesAllInput=false |
| `Source/PersonalProjectUI/Private/HUD/PP_HUD.cpp` | UI | 常驻 Widget HitTestInvisible |
| `Source/PersonalProjectUI/Private/Framework/PP_UIManager.cpp` | UI | RecalculateInputMode 诊断日志 |

---

## 六、教训与避坑

1. **改父类后必须验证蓝图所有 UPROPERTY**：Assets 引用（Settings/IMC/Mesh/Anim BP）、数值配置（Sensitivity）都可能丢。用 PostInitializeComponents 兜底最稳。
2. **EnhancedInput Priority 要明确分层**：ALS 核心输入（视角）永远最高优先，游戏功能输入（技能）次优先，UI 按键（暂停）最次。
3. **ALS Tick 守卫是硬编码的两道门**：Settings null 或 AnimationInstance null → 完整 ALS 循环跳过 → 视角/移动/动画全不刷新。这是最容易踩的坑。
4. **Gameplay 层 Widget 不该切输入模式**：UIManager 的 RecalculateInputMode 只扫 Modal/Overlay 是对的，但常驻 HUD 默认 Visible 可能拦 Slate Hit Test。
5. **Pitch 方向反了先查敏感度默认值**：ALS 源码里 AddControllerPitchInput(Value.Y) 没负号，测试地图蓝图里 LookUpMouseSensitivity=-1.0。如果 C++ 默认值是 1.0，就手动加负号。

---

# 第二轮：Paragon 角色（Kwang）接入 ALS 重定向体系

> 日期：2026-11-01 | 模块：ALS 动画重定向 + Paragon Kwang
> 目标：把 Sixze ALS（C++ 插件版）动画体系应用到 Paragon 角色 Kwang

---

## 七、Bug：站立时转动视角躯干拉长约 1 秒

**现象**：Kwang 站立不动时转鼠标视角，躯干被拉长变形，约 1 秒后自行恢复。移动中不出现。

**排查过程**：

| 步骤 | 做了什么 | 结果 |
|---|---|---|
| 1 | 定性症状 | 躯干拉长 = 两套不同骨架的动画被混合播放 → 必有**曼尼昆（SK_Als）原始动画引用残留**在往 Kwang 骨架上混 |
| 2 | 跑 `PP.FixAlsKwangRefs`（放宽到扫描 AlsKwang 包内全部对象） | Standing 替换 2 处，壳蓝图 `AB_Als_Kwang` 0 处，症状未消除 → **残留引用不在 AlsKwang 包内** |
| 3 | 换思路：症状与「站立 + 转视角」强相关 → 锁定 ALS 的 **Turn In Place（原地转身）** 逻辑 | 转身由 C++ `UAlsAnimationInstance` 驱动，动画序列从 **Settings 数据资产** 读取 |
| 4 | 新增 `PP.FindTurnRefs` 反查运行时引用 | 定位到 `/ALS/ALS/Data/AnimationInstance/AIS_Als_Default` 的子对象 `AlsTurnInPlaceSettings_0~7`，共 8 处引用曼尼昆原版 `A_Als_Turn_90_*` 序列 |

**根因链**：

```
站立转视角 → 触发 ALS Turn In Place
    → UAlsAnimationInstance 从 Settings 数据资产 (AIS_Als_Default)
      的子对象 AlsTurnInPlaceSettings_0~7 读取转身动画序列
    → 序列是曼尼昆原版 A_Als_Turn_90_*（SK_Als 骨架，未重定向）
    → 以动态蒙太奇形式往 TurnInPlaceStanding 槽播放，权重 ≈ 0.39
    → 曼尼昆骨骼动画混合进 Kwang 骨架 → 骨骼拉扯 → 躯干拉长
    → 蒙太奇播完（约 1 秒）→ 恢复
```

**关键认知**：转身动画引用**不在任何动画蓝图里**，而在**插件数据资产的子对象**上——扫描资产包内对象是扫不到的，必须沿 C++ 运行时读取路径反查。

**修复**：新增 `PP.FixTurnRefs` 命令（[PP_AlsRetargetTool.cpp](../../Source/PersonalProject/Private/Editor/PP_AlsRetargetTool.cpp)），三步：

1. **复制** `AIS_Als_Default` → `AIS_Als_Kwang`（不动插件原资产，保证 ALS 自带角色不受影响）；
2. **替换** 副本里 `AlsTurnInPlaceSettings_0~7` 共 8 处序列引用 → 重定向后的 `A_Als_Turn_90_*Kwang`；
3. **切换** `AB_Als_Kwang` 的 Settings 属性：`AIS_Als_Default` → `AIS_Als_Kwang`。

**验证**：转视角躯干拉长消失，修复生效。残留的 `No Inertialization node found`（AnimGraphNode_Slot_2/3）警告为 ALS 原生行为（Slot 节点请求惯性化混合但图中未挂 Inertialization 节点），无害可忽略。

---

## 八、第二轮新增教训

6. **数据资产（Data Asset）的子对象也是引用载体**：重定向角色时，不能只扫描画蓝图/混合空间——C++ 逻辑从 Settings 数据资产读取的动画序列同样要复制重指。扫包扫不到时，沿代码读取路径反查。
7. **症状特征即排查线索**：「站立转视角才出现」直接锁定 Turn In Place 系统，比漫无目的全量扫描高效得多。
8. **不污染插件原资产**：所有修复通过「复制到项目目录 + 改副本引用」完成（AIS_Als_Kwang、AB_Als_Kwang），ALS 插件保持原样，升级不丢修复。

---

# 第三轮：死亡/复活 + 锁定系统（ALS Ragdoll / 复活抖动）

> 日期：2026-11-02 | 模块：ALS Ragdoll + GAS 死亡复活 + LockOn
> 约束：**不改 ALS 插件源码**，所有规避/修复都放在项目代码里

---

## 九、Bug：死亡瞬间 `ALS_ENSURE_MESSAGE` 断点，游戏冻结约 5 秒

**现象**：角色死亡后调试器断在 [AlsCharacter_Actions.cpp:718](../../Plugins/ALS/Source/ALS/Private/AlsCharacter_Actions.cpp#L715-L722) 的 `ALS_ENSURE_MESSAGE`（提示需要 pelvis / spine_03 刚体）；点继续后进入游戏，角色移动/动作"一帧一帧"。

**日志证据**（`Saved/Logs/PersonalProject.log`）：

```
Ensure failed: ... AlsCharacter_Actions.cpp, Line: 718   @21:32:51
Ignoring very large delta of 4.89 seconds                 @21:32:51
```

4.89 秒就是调试器在 ensure 处断下、整个 PIE 被暂停的时间。**"一帧一帧"不是掉帧，是调试暂停。**

**根因链**：

```
StartRagdolling()
    → IsRagdollingAllowedToStart()
        → ALS_ENSURE_MESSAGE(pelvis 刚体 && spine_03 刚体)
            → Development 构建 + 附加调试器 = PLATFORM_BREAK() → 暂停 4.89s
```

而 `StartRagdolling()` 不只在死亡时调用：

| 入口 | 默认配置 | 触发时机 |
|---|---|---|
| `APP_BaseCharacter::HandleDeath` | 项目代码 | 玩家/敌人死亡 |
| `Ragdolling.bStartRagdollingOnLand` | **true** | 高处落地 |
| `Rolling.bInterruptRollingWhenInAir` | **true** | 翻滚中离地 |

**关键事实**：`BP_PP_PlayerCharacter` 的 Mesh **PhysicsAsset = None**（日志 `物理资产=None Pelvis=0 Spine03=0`），所以三条入口都会命中 ensure。

**修复（不碰插件）**：

1. [PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L86-L105) 的 `PostInitializeComponents` 里运行时关闭两个自动入口：
   ```cpp
   Settings->Ragdolling.bStartRagdollingOnLand = false;
   Settings->Rolling.bInterruptRollingWhenInAir = false;
   ```
2. `HandleDeath` 用 `CanStartRagdolling(GetMesh())` 前置检查 pelvis / spine_03，不满足就跳过并打 Warning。
3. [PP_PlayerCharacter.cpp](../../Source/PersonalProject/Private/Characters/PP_PlayerCharacter.cpp#L722-L738) 的 `Input_OnRagdoll`（手动布娃娃按键）同样先做刚体检查。

**注意**：插件源码（`AlsCharacter_Actions.cpp`）已恢复原样，本轮的拦截全在项目侧；如果想恢复真正的布娃娃倒地，需要给角色网格指定包含 `pelvis` / `spine_03` 刚体的 PhysicsAsset，再把上面两处开关打开。

---

## 十、Bug：锁定敌人 → 死亡 → 复活后，移动抖约 1 秒

**现象**：死亡前**锁定过敌人**时，复活后不到 1 秒内角色移动会抖；不锁定时基本没有。

**定性**：`[PP_Trace]` 采样显示复活前后 `平均帧=16.7ms / 最大帧<25ms`、`URO=0`，**帧率稳定 60FPS，没有长帧** → 不是掉帧，是位移/视角插值类抖动。

**根因（三点叠加）**：

| # | 原因 | 说明 |
|---|---|---|
| 1 | 死亡期间残留速度 | 锁定状态下多在敌人附近冲刺移动，死亡时 `CharacterMovement` 还留着速度与待发送移动；复活恢复 `Walking` 的第一秒被插值出来，表现为抖动 |
| 2 | 出生点传送没标 Teleport | `ServerRespawn` 原来用 `SetActorLocation(loc, false)`（`Teleport=None`），客户端把大位移当**普通移动**做网络平滑 → 复活后第一秒回拉抖动 |
| 3 | 旧锁定视角被继承 | 锁定把 `ControlRotation` 拉到敌人方向，死亡/复活时没重置；复活后角色朝旧方向转身，和移动方向打架，看起来像抖一下 |

**为什么感觉"和锁定强相关"**：锁定死亡时角色通常处于运动状态（冲刺/绕圈），三个条件同时满足；不锁定时多是站桩被击中，速度≈0、视角也没被改写，所以症状不明显。锁定不是原因，只是把三个诱因同时集齐了。

**修复**：

1. [PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L445-L467) `HandleRespawn` 里清残留：
   ```cpp
   CM->StopMovementImmediately();
   CM->Velocity = FVector::ZeroVector;
   // 本机玩家把视角 Yaw 拉回角色朝向，不继承死亡前的锁定方向
   ControlRotation.Yaw = GetActorRotation().Yaw;
   PC->SetControlRotation(ControlRotation);
   ```
2. [PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L391-L407) `ServerRespawn` 出生点传送改为瞬移语义：
   ```cpp
   SetActorLocation(PlayerStart->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
   SetActorRotation(FRotator(0.f, PlayerStart->GetActorRotation().Yaw, 0.f));
   ```

**验证**：锁定 → 死亡 → 复活 → 立刻冲刺/翻滚，抖动消失，`[PP_Trace]` 全程 16.7ms，`旋转=ViewDirection` 正常恢复。

---

## 十一、排错日志（保留在项目里，便于回归）

| 日志标签 | 位置 | 内容 |
|---|---|---|
| `[PP_DeathFlow]` | `APP_BaseCharacter::HandleDeath` | 开始/完成：LocalRole、控制器、移动模式、LocomotionAction、物理资产、pelvis/spine_03、胶囊碰撞、输入忽略、复活计时器 |
| `[PP_RespawnFlow]` | `ServerRespawn` / `ClientOnRespawn` / `HandleRespawn` | 各入口前后的移动模式、碰撞、输入忽略、清空残留速度值、瞬移目标 |
| `[PP_Hitch]` | `APP_PlayerCharacter::Tick` | 本机玩家单帧 >50ms 时每秒一条，区分"掉帧"与"状态异常" |
| `[PP_Trace]` | `APP_PlayerCharacter::Tick` | 每 0.5s 采样：平均/最大帧、存活、锁定、目标、LocomotionMode/Action、旋转模式、步态、姿态、速度、角色/视角 Yaw、Montage、URO、网络平滑（**问题确认后已删除**） |
| `[LockOn]` | `ToggleLockOn` / `ReleaseLockOn` | 锁定目标、解除锁定、恢复旋转模式 `A → B` |

**观察项（未修，待后续确认）**：客户端 `ClientOnRespawn` 里 `EnableInput` 之后 `IsMoveInputIgnored/IsLookInputIgnored` 仍为 1，但实测移动正常。说明 `EnableInput`（PushInputComponent）与 `SetIgnoreMoveInput` 是两套机制，Ignore 计数由别处设置且未被清理。若后续出现"复活后不能转向/不能移动"，优先查这里。

---

## 十二、第三轮新增教训

9. **先分清"掉帧"还是"暂停"**：`Ignoring very large delta of X seconds` 是调试器断点暂停的直接证据，不要按性能问题去优化。
10. **ALS 的 ensure 不只在死亡路径**：落地、翻滚离地都会调 `StartRagdolling()`；不改插件时，用运行时开关 `Settings->Ragdolling.* / Rolling.*` 关闭入口是成本最低的规避方式。
11. **复活要当成"瞬移"处理**：大位移必须 `ETeleportType::TeleportPhysics`，并清速度、重置视角，否则客户端网络平滑会把复活第一秒演绎成抖动。
12. **症状与功能的"相关性"不等于因果**：锁定只是让"带速度死亡 + 视角被改写"同时发生，真正的修复点是复活流程的状态清理。

---

# 第四轮：篝火/存档点 + 死亡掉魂（复活流程收尾）

## 十三、Bug：死亡/复活后技能无法释放

**现象**：死亡复活后按技能键没有反应；进一步排查时表现为"复活后只能连放两次技能"。

**定性**：按键链路本身正常（`[PP_Skill] 技能按键绑定: 主=IA_Primary 次=IA_Secondary ...` 说明 IA 资产有效），拦截发生在 `APP_PlayerController::ActivateAbility` 的存活判断和 GAS 的 `CanActivateAbility` 两层。

**根因（三重叠加）**：

| # | 原因 | 日志证据 | 修复 |
|---|---|---|---|
| 1 | 客户端 `bAlive` 残留 false | `[PP_Skill] 按键: ... 存活=0 bAlive=0`，而服务端已复活；`bAlive` 是复制属性，服务端的 `true` 不保证再复制一次 | `ClientOnRespawn` 里以这条 Client RPC 的语义在客户端本地置 `bAlive = true` |
| 2 | 客户端 `Status.Dead` 标签清不掉 | `客户端清理 Dead 后: DeadTag=1 拥有标签=[PPTags.Status.Dead]`，只移除授予 Dead 的 GE 无效 | 死亡能力（`GA_PP_Death`）的 `ActivationOwnedTags` 也授予 `Status.Dead`；**GE 移除清不掉"仍处于激活状态的能力"持有的同名标签**，必须先 `CancelAbilities` |
| 3 | 同一个技能被授予两份 | `[PP_Skill] 匹配能力数=2`（`GA_PP_Primary_C_0`、`GA_PP_Primary_C_1`）；`BP_PP_PlayerCharacter.StartupAbilities` 与 `BP_CrashGameMode.CharacterAbilityMap` 都配置了 `GA_PP_Primary` | 两处授予点都加去重（`FindAbilitySpecFromClass` 命中就跳过） |

**为什么表现成"只能放两次"**：`ASC->TryActivateAbilitiesByTag` 会激活**所有**匹配的 spec。同一个技能两份实例 → 一次按键激活两份、扣两次体力；体力归零后 `CheckCost` 失败。

**为什么失败日志是 `失败原因=[]`**：`CanActivateAbility` 里 `CheckCost` 失败时添加的 `ActivateFailCost` 标签依赖 `AbilitySystemGlobals` 配置，本项目未配置 → 拿到的是空标签。所以 `[PP_Skill] 失败原因=[]` 实际上等价于"体力不足 / 冷却中 / 激活抑制"，必须补打体力值才能定位。

关键修复：

```cpp
// ClientOnRespawn：客户端本地按 RPC 语义记存活（服务端 true 值不保证再复制）
bAlive = true;
```

```cpp
// GiveStartupAbilities：重复授予会让一次按键激活多份实例、多扣资源
if (GetAbilitySystemComponent()->FindAbilitySpecFromClass(Ability))
{
    UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 跳过重复授予: %s 已存在"), *Ability->GetName());
    continue;
}
```

修复位置：[PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L365-L388)（授予去重）、[PP_CrashGameMode.cpp](../../Source/PersonalProject/Private/Game/PP_CrashGameMode.cpp#L141-L175)（授予去重）、[PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L564-L648)（`ClientOnRespawn` 的 bAlive / Dead / 碰撞恢复）。

**复测点**：`[PP_Skill] 匹配能力数` 应为 1；每次按键只应出现一条 `[PP_Skill] 扣除体力:`；`激活结果` 后面的 `体力后=` 与 `GA_PP_Primary` 配置的 `StaminaCost` 对应，体力回满后可继续释放。

**遗留观察**：最新日志里 `客户端取消激活能力后: DeadTag=1`（取消能力 + 移除 GE 之后 Dead 仍在），说明复活后仍有别的来源重新授予 Dead（疑为死亡 GE 的复制时序晚到）。已在复活后的 3 秒采样窗口内每 0.1 秒兜底清一次（取消 `PPTags.PPAbilities.Death` 能力 + 移除授予 Dead 的 GE，日志 `复活窗口内清理残留 Dead`），表现为恢复正常。若后续再复现"复活后技能失效"，优先查：死亡能力是否被重复激活、死亡 GE 是否在 `ClientOnRespawn` 之后才复制到客户端。

---

## 十四、Bug：复活后移动卡顿

**现象**：复活后移动一卡一卡，像一直在撞墙推不进去。

**定性**：`[PP_RespawnMove]` 采样显示 `传送=1` 长期为 1、`胶囊碰撞=0`，说明不是掉帧，而是服务端在持续把角色校正回权威位置（`PostNetReceiveLocationAndRotation`）。

**根因（按排查顺序）**：

| # | 原因 | 说明 |
|---|---|---|
| 1 | 客户端重复执行 `HandleRespawn` | 死亡能力的蒙太奇回调在客户端也会执行；客户端再 `StopMovementImmediately` 会把刚恢复的速度（日志里速度=375）清零。修复：`HandleRespawn` 加 `HasAuthority` 守卫，只让服务端执行 |
| 2 | 服务端一次死亡执行两次 `HandleDeath` | `PossessedBy` 与 `OnRep_PlayerState` 都 `AddUObject` 绑定属性委托 → 一次掉血触发两次 `OnHealthChanged`；第二次又禁输入、重启复活计时。修复：两处绑定前 `RemoveAll(this)`，并给服务端 `HandleDeath` 加"已死亡直接 return"守卫（日志 `HandleDeath 重复触发已忽略`） |
| 3 | **客户端胶囊碰撞没有恢复（真正主因）** | `HandleDeath` 里把胶囊设为 `NoCollision`，但**碰撞状态不复制**；复活只恢复了服务端的碰撞 → 客户端本地移动预测不参与碰撞、服务端照常挡住，每帧都被校正 |

关键修复：

```cpp
// ClientOnRespawn：碰撞不复制，必须客户端本地恢复
if (UCapsuleComponent* Capsule = GetCapsuleComponent())
{
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
```

**验证**：采样从 `传送=1 胶囊碰撞=0` 变为 `传送=0 胶囊碰撞=3`，卡顿消失。

---

## 十五、本轮新增排错日志

| 日志标签 | 位置 | 内容 |
|---|---|---|
| `[PP_Skill]` | [PP_PlayerController.cpp](../../Source/PersonalProject/Private/Player/PP_PlayerController.cpp#L173-L238) / [PP_GameplayAbility.cpp](../../Source/PersonalProject/Private/AbilitySystem/Abilities/PP_GameplayAbility.cpp#L11-L60) / 授予点 | 按键时的 存活/bAlive/DeadTag/体力/激活抑制/拥有标签；匹配能力数；每次扣体力的实例、LocalRole 与消耗；跳过重复授予 |
| `[PP_RespawnMove]` | [PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp#L257-L310) | 复活后 3 秒内每 0.1s 采样：位置/速度/加速度/输入/MovementMode/传送/忽略输入/胶囊碰撞，并兜底清理 Dead |
| `[PP_RespawnFlow]` | `ServerRespawn` / `HandleRespawn` / `ClientOnRespawn` | 各入口的 bAlive/DeadTag/移动模式/碰撞/输入忽略、残留激活能力、Dead 清理结果 |
| `[PP_Attributes]` | `APP_BaseCharacter::ResetAttributes` | 复活前后 生命/魔力/耐力 的当前值与上限 |
| `[PP_Bonfire]` | `APP_BonfireActor::Rest` / `APP_PlayerController::ServerInteractBonfire` | 篝火交互、贴地射线结果、重生点写入 |

---

## 十六、第四轮新增教训

13. **复活要同时检查"服务端状态"和"客户端本地状态"**：`bAlive`、`Status.Dead`、胶囊碰撞、输入忽略，都是"服务端对了客户端不一定对"。
14. **不是所有状态都会复制**：碰撞开关、动画状态、`IgnoreMoveInput` 计数都不走网络；复活流程必须在 owning 客户端本地补一次。
15. **GAS 的 `ActivationOwnedTags` 不是 GE**：移除授予某标签的 GE，清不掉"仍处于激活状态的能力"持有的同名标签，必要时先 `CancelAbilities`。
16. **同一 Ability 可能被多处授予**：`StartupAbilities` + GameMode 角色表 + 蓝图层都可能配同一份；授予点必须去重，否则一次按键激活多份、多扣资源。
17. **`失败原因=[]` 不是"没有原因"**：只是 `ActivateFailCost/Cooldown` 等标签没在 `AbilitySystemGlobals` 里配置；定位时要自己打印体力、冷却、激活抑制。
18. **判断"卡顿"先看 `bJustTeleported`**：长期为 1 说明服务端在持续校正，九成是客户端本地预测与权威状态不一致（碰撞/速度/位置）。

---

# 第五轮：敌人 AI 索敌距离 + 攻击节奏异常

> 日期：2026-11-04 | 模块：AIPerception 索敌 + SearchForTarget 攻击循环（GAS）
> 现象：敌人"要贴到脸上才索敌"；修好后攻击速度又偶尔特别快

---

## 十七、Bug：敌人必须在很近的距离才会索敌

**现象**：把 `SearchRadius` 从 1500 放大到 15000 后，敌人仍然要等玩家走到跟前才触发搜索。

**排查方式**：在 `UPP_AIPerceptionComponent::Search()` 里加"漏斗诊断"——记录最近玩家的距离，以及它被**哪一条规则**拒绝（超出半径 / 高度差 / 视野锥外 / 视线被遮挡 + 挡住它的 Actor），无目标时每秒限频打印。

**日志证据**：

```
[AIPerception] 未获取目标: BP_Enemy_Melee_C_3 半径=15000 最近玩家=BP_PP_PlayerCharacter_C_0
    距离=3667 拒绝原因=在视野锥外 Dot=-0.97
[AIPerception] 未获取目标: BP_Enemy_Ranged_C_1 半径=15000 ... 距离=2555 拒绝原因=在视野锥外 Dot=-0.88
[AIPerception] 筛选统计: 半径=15000, 候选=3, PlayerTag=1, 距离内=1, 高度内=1, 视野内=0, 视线内=0, 最终=null
[AIPerception] 获得目标: BP_PP_PlayerCharacter_C_0 距离=133
```

**关键结论**：

- `半径=15000` 说明 C++ 默认值**已生效**，不是蓝图序列化覆盖；
- `距离内=1 / 高度内=1 / 视野内=0` 说明半径和高度差都通过，**唯一的拦截是视野锥**；
- `Dot≈-0.95 ~ -0.97` 表示玩家几乎在敌人**正后方**（Dot=-1 即正背），而 `VisionHalfAngle=60°`（共 120° 视野）把背后全部挡掉；
- 最后`距离=133` 才获得目标，是因为玩家绕到了 `Dot=0.09`（侧前方）。

**根因链**：

```
敌人出生后朝向固定、从不主动转向玩家
    → Search() 用 Pawn->GetActorRotation() 做视野锥判定（Dot ≥ cos(60°)=0.5）
    → 玩家从背后接近，Dot < 0 永远不通过
    → 只有绕到正面/贴脸才满足视野锥
    → 表现为"必须很近才索敌"
```

**修复**：按"魂类以外的实用取舍"改成**全向 360° 感知**——删除视野锥判定（当前目标保持 + 候选扫描两处），只保留 半径 → 高度差 → 视线 LineTrace 三道筛选；`VisionHalfAngle` 属性随之下线。验证通过后把 `SearchRadius/LeashRange` 恢复为原来的 `1500/3500`。

---

## 十八、Bug：敌人攻击速度偶尔特别快（冷却被绕过）

**现象**：敌人有时 0.3~0.7 秒就再打一次，而不是设计的 1.5~2.5 秒冷却。

**排查方式**：在 `UPP_SearchForTarget` 全链路加 `[EnemyAttack]` 日志，并加"挂起搜索 / 并发 MoveTo / Attack 序号与间隔 / 前摇值"计数。

**日志证据**：

```
Attack #1 距上次=-1.00s 前摇=0.20s 激活=1 挂起搜索=1 并发MoveTo=0
进入攻击冷却 2.03s                     ← 冷却 2 秒
收到 EndAttack 事件                    ← 攻击蒙太奇播完立刻发事件
StartSearch #28 延迟=0.15             ← 直接重启搜索，绕过冷却
Attack #2 距上次=0.69s                ← 实际只隔 0.69 秒
...
冷却结束 → 下一轮搜索（挂起搜索=0）
StartSearch #31 延迟=0.39 挂起搜索=1
收到 EndAttack 事件：SearchPending=1
StartSearch #32 延迟=0.23 挂起搜索=2   ← 两条循环叠加
Attack #5 距上次=0.39s                ← 进一步加速
```

**根因（两个问题叠加）**：

| # | 问题 | 说明 |
|---|---|---|
| 1 | 冷却被 `EndAttack` 短路 | `Attack()` 里启动了 1.5~2.5s 的 `PostAttackCooldownTask`，但攻击蒙太奇结束立刻广播 `Events.Enemy.EndAttack` → `EndAttackEventReceived` → `StartSearch()`（0.1~0.5s）→ MoveTo → 0.2s 前摇 → 再 Attack。冷却形同虚设 |
| 2 | 搜索循环没有去重 | 冷却任务到点也 `StartSearch()`，与 `EndAttack` 路径并发 → `挂起搜索=2`，多路 `Search → MoveTo → Attack` 叠加，间隔越来越短 |

`前摇=0.20s` 说明 `GetTimeLiveLenght()` 正常，**不是前摇为 0 的问题**。

**修复**：

1. **冷却成为唯一节奏来源**：删除 `EndAttack` 的监听与 `EndAttackEventReceived`，攻击结束不再重启搜索，只由冷却任务到点后 `StartSearch()`。
2. **攻击门控**：新增 `NextAttackAllowedTime`，`Attack()` 在冷却未结束时直接跳过并继续等待，任何路径都无法绕过冷却；每次攻击前先取消旧冷却任务，保证只有一条。
3. **`StartSearch()` 幂等**：已有挂起搜索时直接忽略（日志 `StartSearch 被忽略：已有挂起搜索`），杜绝 HitReact / 冷却叠成双循环。
4. **HitReact 不再取消冷却任务**：只取消前摇 Delay 和 MoveTo，冷却计时不会被清掉。

**验证标准**（`[EnemyAttack]` 日志）：

- `Attack #N 距上次=` 稳定在 1.7~2.7s（冷却 1.5~2.5 + 前摇 0.2），不再出现 0.3~0.7s；
- `挂起搜索` 始终 ≤ 1；
- 不再出现 `收到 EndAttack 事件`。

**顺带发现（未改）**：远程敌人 `MoveTo 结束 Result=0` 时距离 867~923，而 `AcceptanceRadius` C++ 默认 500，说明蓝图覆盖了更大的值，不影响节奏但会让远程怪贴脸。

---

## 十九、本轮排错日志

| 日志标签 | 位置 | 内容 |
|---|---|---|
| `[AIPerception]` | [UPP_AIPerceptionComponent.cpp](../../Source/PersonalProjectCore/Private/AI/UPP_AIPerceptionComponent.cpp) | 无目标时每秒：实际半径、最近玩家、距离、**拒绝原因**（超出半径/高度差/视野锥外 Dot/视线被遮挡 + HitActor）；30 秒一次漏斗统计；获得目标的距离。**定位完成后已全部删除** |
| `[EnemyAttack]` | [PP_SearchForTarget.cpp](../../Source/PersonalProject/Private/AbilitySystem/Abilities/Enemy/PP_SearchForTarget.cpp) | `StartSearch #序号/延迟/挂起数`、`Search 到时`、`MoveTo 开始/结束 + 并发数`、`Attack #序号/距上次/目标距离/前摇/激活结果`、`进入冷却`、`冷却结束`、`冷却中跳过`、`StartSearch 被忽略`、`HitReact 事件` |

---

## 二十、第五轮新增教训

19. **先打印"被哪条规则拒绝"，再调数值**：索敌问题是多条件漏斗（半径/高度/视野/视线），只打印"没找到"毫无信息量；打印"最近玩家 + 拒绝原因"能一次定位到具体条件。
20. **日志里打印参数本身可以验证配置来源**：`半径=15000` 直接排除了"蓝图覆盖默认值"的猜测。
21. **"随机变快"多半是并发不是参数**：凡是"有时……有时……"的节奏异常，优先怀疑多条同构循环叠加，加"挂起任务计数 + 序号 + 间隔"日志即可实锤。
22. **一个节奏只能有一个驱动源**：事件（EndAttack/HitReact）和计时器（冷却）同时驱动同一循环必然双循环；把计时器定为唯一来源，事件只负责打断，循环入口还要做幂等去重。

---

# 第六轮：篝火休息不复活已击杀的敌人

> 日期：2026-11-04 | 模块：篝火重置 + 敌人重生管理器（`UPP_EnemyRespawnManager`）
> 现象：杀掉敌人后再用篝火休息，附近的敌人不会复活

---

## 二十一、Bug：击杀后篝火休息不复活附近敌人

**现象**：篝火休息功能本身正常（属性回满、重生点写入都对），但被击杀的敌人不会回到生成点；活着的敌人（没被打死的）重置正常。

**关键事实**：

- 敌人 `BeginPlay` 只打印过一次 `[EnemyRespawnManager] RegisterAllEnemies: 注册 2 个敌人`，说明注册表里有记录；
- 篝火休息时 `ResetEnemiesInRadius` 的重置数为 0，没有任何 `ResetToSpawn` 输出；
- `PP_Death` 的收尾分支对敌人执行 `Avatar->SetLifeSpan(1.f)`，尸体 1 秒后销毁。

**根因链**：

```
UPP_EnemyRespawnManager 只保存 TArray<TObjectPtr<APP_EnemyCharacter>> EnemiesInLevel
    → 敌人死亡 → GA_PP_Death 收尾 SetLifeSpan(1.0)
    → 尸体 1 秒后被 GC 销毁，数组里只剩空指针
    → ResetEnemiesInRadius 里 if (!IsValid(Enemy)) continue 全部跳过
    → 重置数 0，篝火"什么都没做"
```

**本质**：注册表的生命周期**绑在 Actor 实例**上，而"敌人会被销毁"是设计行为（不设 `bShouldRespawn` 的怪就是一次性消耗品）。要支持篝火复活，记录必须独立于实例存在。

**修复（三处）**：

1. **注册表改成"生成记录"**（[PP_EnemyRespawnManager.h](../../Source/PersonalProject/Public/Game/PP_EnemyRespawnManager.h#L15-L31)）

   ```cpp
   USTRUCT()
   struct FPP_EnemySpawnRecord
   {
       UPROPERTY() TSubclassOf<APP_EnemyCharacter> EnemyClass;   // 重建用
       UPROPERTY() FTransform SpawnTransform;                    // 生成点
       UPROPERTY() TWeakObjectPtr<APP_EnemyCharacter> Enemy;     // 当前实例，销毁后自动变空
   };
   ```

   `TWeakObjectPtr` 而不是 `TObjectPtr`：不阻止 GC，但实例销毁后自然失效，可直接作为"该重建了"的判据。

2. **`RegisterEnemy` 三态合并 + `ResetEnemiesInRadius` 两阶段遍历**（[PP_EnemyRespawnManager.cpp](../../Source/PersonalProject/Private/Game/PP_EnemyRespawnManager.cpp)）

   - 同一实例 → 刷新类/生成点；
   - 存在"空指针 + 同类 + 生成点 25cm 内"的记录 → 接管（蓝图自行重生同款怪时不会产生重复记录）；
   - 否则新建记录。

   ```cpp
   // 阶段一：只读遍历
   //   实例有效且半径内 → ResetToSpawn()
   //   实例已销毁但记录在半径内 → 收集索引到 PendingRespawn
   // 阶段二：按索引 SpawnActor 重建并回填指针
   ```

   **必须分两阶段**：`SpawnActor` 会同步执行敌人的 `BeginPlay` → 回调 `RegisterEnemy` → 直接 `Add` 到正在遍历的数组里，循环内重建会让引用/索引失效。实现上用 `bIsRespawning` 在重建窗口内抑制 `RegisterEnemy`，由 `ResetEnemiesInRadius` 自己回填指针，避免新增重复记录。

3. **`ResetToSpawn` 补齐死亡残留清理**（[PP_EnemyCharacter.cpp](../../Source/PersonalProject/Private/Characters/PP_EnemyCharacter.cpp#L134-L180)）

   ```cpp
   SetLifeSpan(0.f);              // 撤销死亡流程挂的延迟销毁
   SetActorHiddenInGame(false);   // 死亡时隐藏了
   SetActorEnableCollision(true);
   ...
   GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
   bIsBeingLaunched = false;
   LandedDelegate.RemoveAll(this);
   SendGameplayEventToActor(this, PPTags::Events::Enemy::EndAttack, {});
   ```

   `SetLifeSpan(0.f)` 是关键补充：篝火可能在尸体销毁前的 1 秒窗口内被使用，撤回销毁计时否则会出现"复活后立刻消失"。

**验证步骤**：

1. 杀一个离篝火 2000（`ResetRadius` 默认值）以内的敌人，等 1 秒以上让尸体销毁；
2. 篝火按 `E` 休息；
3. 尸体已销毁时日志应为 `[EnemyRespawnManager] 重建敌人 xxx @ (x, y, z)`；未销毁时应有 `ResetToSpawn`；
4. 敌人回生成点、满血、可见、可碰撞，且能重新索敌（`EndAttack` 事件恢复了攻击循环）。

---

## 二十二、本轮排错日志

| 日志标签 | 位置 | 内容 |
|---|---|---|
| `[EnemyRespawnManager]` | [PP_EnemyRespawnManager.cpp](../../Source/PersonalProject/Private/Game/PP_EnemyRespawnManager.cpp) | `RegisterAllEnemies` 关卡内敌人数 / 注册表记录数；`重建敌人` 的名字与生成点；`重建失败` 的类名；`ResetEnemiesInRadius` 的 Origin / Radius / 重置数 / 总记录数 |
| `[Enemy]` | [PP_EnemyCharacter.cpp](../../Source/PersonalProject/Private/Characters/PP_EnemyCharacter.cpp) | `ResetToSpawn` 的敌人名与目标位置；AIController 缺少感知组件时自动补建 |
| `[PP_Bonfire]` | [PP_BonfireActor.cpp](../../Source/PersonalProject/Private/GameObjects/PP_BonfireActor.cpp) | 休息成功、贴地射线结果、重生点写入、触发的敌人重置数与半径 |

---

## 二十三、本轮新增教训

23. **"要重置/要复活"的对象，注册表不能只存 Actor 指针**：凡是死亡后 `SetLifeSpan`/`Destroy` 的对象，注册表必须存"重建所需的信息（类 + 生成点）"，实例只作为弱引用缓存。
24. **不要在遍历中生成 Actor**：`SpawnActor` 同步执行 `BeginPlay`，被生成对象往往会在回调里改同一个容器（注册自己），必须"先收集、后重建"，或加抑制标志。
25. **重置类操作要撤销"延迟销毁"**：`SetLifeSpan(N)` 的待销毁状态不会因为 `SetActorHiddenInGame(false)`、`SetAlive(true)` 自动取消，必须显式 `SetLifeSpan(0.f)`。
26. **复活要补发战斗循环的"结束事件"**：敌人 AI 的攻击循环挂在 `Events.Enemy.EndAttack` 上，被硬重置打断后如果不补发，敌人会卡在"攻击中"不再索敌。
