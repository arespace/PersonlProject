# 蓝图配置清单

## 何时输出

C++ 改动出现以下任一情况，交付时必须附结构化清单，不能零散描述：

- 新增 `UPROPERTY`（尤其 `EditDefaultsOnly` / `EditAnywhere` / `TSubclassOf` / `TSoftClassPtr`）
- 新增 `UCLASS` 需要放进关卡或设为 GameMode/HUD/WidgetClass
- 新增 `BindWidget` 控件
- 新增数据表列或 RowName 约定
- 新增 GA / GE 需要在蓝图里挂载

## 前置

> 新增 `UPROPERTY` / 新 `UCLASS` 后，**必须关闭编辑器完整编译并重开**，字段才会出现在 Details 面板。Live Coding 不刷新反射。

## 输出模板

```markdown
## <功能名> 蓝图配置 Checklist

> 前置：已关闭编辑器完整编译；重开后再配。

### 1. 资产清单

| # | 资产 | 类型 | 状态 |
|---|---|---|---|
| 1 | /Game/PersonalProject/Characters/PlayerCharacter/BP_PP_PlayerCharacter | 已有蓝图 | 修改 |
| 2 | /Game/PersonalProject/Game/BP_CrashGameMode | 已有蓝图 | 修改 |

### 2. Class Defaults / Component 字段

| # | 资产 | Category | 字段名（与 C++ 完全一致） | 值 | 必填 |
|---|---|---|---|---|---|
| 1 | BP_PP_PlayerCharacter | PP\|Weapon | WeaponDataTable | DT_WeaponData | 是 |

### 3. 关卡摆放

| # | Actor 类或蓝图 | 数量 | 引用 | 备注 |
|---|---|---|---|---|
| 1 | PP_BossArenaTrigger（C++ 类可直接摆放） | 1 | Boss / Gate | Box Extent 覆盖场地，不含玩家出生点 |

### 4. 动画 / GA 配置

| 资产 | 位置 | 动作 |
|---|---|---|
| GA_PP_Primary | 蒙太奇 Slot | 设为 Locomotion.Default |

### 5. 验证步骤

1. 完整编译通过后重开编辑器，确认字段出现在 Details
2. PIE：<具体操作> → <预期结果>
3. 检查 Saved/Logs/PersonalProject.log 无新增 Error
```

## 字段写法规则

- 字段名**一字不差**匹配 C++ 声明，不写描述性别名。
- `Category` 取 `UPROPERTY(Category="...")` 原文，例如 `PP|Weapon`。
- `TSubclassOf<T>` 的值必须是 **Blueprint 类资产**，不能填 C++ 类；`TSoftClassPtr` 填资产引用。
- `BindWidget` 控件名必须与 C++ 成员名完全一致，否则蓝图编译报错。
- DataTable 的 RowName 与关联表/枚举 ID 必须一致（`DT_Items` ↔ `DT_WeaponData` ↔ `WeaponMeshMap` Key ↔ ItemID）。
- 资产路径必须先用 Glob 核实存在；不存在就写「需新建」并给父类 C++ 类名，**禁止编造路径**。

## 已核实的真实资产（2026-11-06）

| 资产 | 路径（`/Game/...`） |
|---|---|
| BP_PP_PlayerCharacter | `/Game/PersonalProject/Characters/PlayerCharacter/BP_PP_PlayerCharacter` |
| BP_PP_PlayerController | `/Game/PersonalProject/Player/BP_PP_PlayerController` |
| BP_CrashGameMode | `/Game/PersonalProject/Game/BP_CrashGameMode` |
| BP_Als_GameMode | `/Game/PersonalProject/Game/BP_Als_GameMode` |
| BP_CrashGameInstance | `/Game/PersonalProject/Game/BP_CrashGameInstance` |
| BP_Bonfire | `/Game/PersonalProject/GameObjects/BP_Bonfire` |
| BP_BossArenaGate | `/Game/PersonalProject/GameObjects/BP_BossArenaGate` |
| BP_SoulPickupActor | `/Game/PersonalProject/GameObjects/BP_SoulPickupActor` |
| DT_Characters / DT_Items / DT_WeaponData / DT_Task / DT_ShopInventory | `/Game/PersonalProject/Data/` |
| GE_Cost_Stamina / GE_StaminaRegen / GE_StaminaSprintDrain | `/Game/PersonalProject/AbilitySystem/GameplayEffects/Player/` |
| WBP_HUDWidget | `/Game/PersonalProject/UI/GAS/Core/WBP_HUDWidget` |

说明：`BP_BossArenaTrigger`、`WBP_Settlement`、`GA_PP_Roll`、`WBP_HUD` 等名称在当前 `Content` 中**不存在**，不要沿用旧文档示例。

## 常见必配项

| C++ 改动 | 蓝图侧必须配 |
|---|---|
| 新 GameMode | 地图 World Settings 的 GameMode Override，或 Project Settings 默认 GameMode |
| 新 HUD | GameMode 的 `HUDClass` |
| 新 Widget | HUD 的 Widget 类字段 + 该 Widget 蓝图的 `OwningLayer` |
| 新 GA | `StartupAbilities` 或 `CharacterAbilityMap`（二选一，禁止重复） |
| 新 GE | GA 的 Cost / Cooldown GE 字段 |
| 新 DataTable | `DefaultGame.ini` 的 `[/Script/PersonalProjectCore.PP_DataManager]` 路径 + DataManager 字段 |
| 新交互 Actor | 实现 `IPP_Interactable`，并在关卡中设置碰撞与提示 Widget |

## 反模式（禁止）

- 「你可能需要配一下 HUD 里的那个字段」→ 必须写清资产 + 字段名 + 值。
- 「Boss 激活需要加 tag」→ 必须写清由谁在什么时机 Add/Remove。
- 漏掉「关编辑器完整编译」前提。
- 把 C++ 类名当成蓝图资产路径。
- 复制旧文档里未核实的示例资产名。
