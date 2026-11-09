# 架构与代码归属

## 1. 五模块边界

```
PersonalProjectCore       基础设施：EventBus / DataManager / GameplayTag / 接口 / 数据头 / 基类 / ASC / AttributeSet
        ↑
PersonalProjectInventory  背包、商店、采集
        ↑
PersonalProjectQuest      任务、NPC 对话
        ↑
PersonalProject           主玩法：角色、PlayerController、GameMode、Boss、篝火、GA/AI、关卡 Actor
        ↑
PersonalProjectUI         HUD、UIManager、全部 Widget、大厅 UI
```

- 依赖只能向上（下游依赖上游），禁止反向；`Core` 不得 `include` 任何项目模块头。
- 不新增模块。全局管理器用 `UActorComponent` 挂到 `GameMode` / `GameInstance`，不用单例类。
- 跨模块类型放 `Core` 的 `Public/`，并带 `<MODULE>_API` 导出宏（例如 `PERSONALPROJECTCORE_API`）。
- 模块改名/搬类后：`Config/DefaultEngine.ini` 的 CoreRedirects、`DefaultGame.ini` 的 ini 段名、`.uproject` 模块声明三处都要同步。

## 2. 接口（跨模块唯一契约）

核心头在 `Source/PersonalProjectCore/Public/`：

| 接口 | 头文件 |
|---|---|
| `IPP_Interactable` / `IPP_Resettable` / `IPP_Staggerable` / `IPP_EquipmentUser` | `Interaction/PP_InteractableInterface.h` |
| `IPP_InventoryQuery` / `IPP_InventoryActions` | `Inventory/PP_InventoryQuery.h`、`PP_InventoryActions.h` |
| `IPP_ShopQuery` / `IPP_ShopActions` | `Shop/PP_ShopQuery.h`、`PP_ShopActions.h` |
| `IPP_QuestQuery` | `Quest/PP_QuestQuery.h` |
| `IPP_LobbyActions` / `IPP_LobbyPlayerInfo` / `IPP_GameFlowRequest` | `Framework/PP_LobbyActions.h`、`PP_LobbyPlayerInfo.h`、`PP_GameFlowRequest.h` |
| `IPP_DialogueSource` | `Dialogue/PP_DialogueSource.h` |

不在 Core 的两个：

| 接口 | 位置 | 说明 |
|---|---|---|
| `IPP_UIGateway` | `Source/PersonalProject/Public/Player/PP_UIGateway.h` | 由 `APP_HUD` / `APP_LobbyHUD` 实现，PlayerController 用 `ForwardToUIGateway` 泛型模板转发 |
| `IPP_DialogueRequester` | `Source/PersonalProjectQuest/Public/Dialogue/PP_DialogueRequester.h` | 由 `APP_PlayerController` 实现 |

规则：

- `BlueprintNativeEvent` 在 C++ 侧只能用 `Execute_Xxx(Object, ...)`；接口指针直调会走 `check(0)` 桩。
- 接口实现类必须提供 `_Implementation`；新增接口时同时补调用方，只声明不调用等于死代码。
- UI 模块只通过接口 + EventBus 访问业务，不 `Cast` 具体业务组件反向直调。

## 3. EventBus

`UPP_EventBus` 是 `UGameInstanceSubsystem`，头文件 `Framework/PP_EventBus.h`。

```cpp
// 订阅
FDelegateHandle Handle = Bus->Subscribe<FPP_InventoryUpdatedEvent>(
    [this](const FPP_InventoryUpdatedEvent& E) { /* ... */ });

// 广播（内部封装 GetWorld → GetGameInstance → GetSubsystem 判空）
UPP_EventBus::BroadcastEvent(this, FPP_BossDeadEvent{ /* ... */ });
```

现有事件结构体在 `Framework/PP_EventTypes.h`：`FPP_QuestCompletedEvent`、`FPP_QuestsUpdatedEvent`、`FPP_InventoryUpdatedEvent`、`FPP_EquipmentUpdatedEvent`、`FPP_GoldUpdatedEvent`、`FPP_ShopResultEvent`、`FPP_PurchaseCompletedEvent`、`FPP_GatherEvent`、`FPP_BossDeadEvent`、`FPP_SettlementShowEvent`。

规则：

- 新增跨系统通知：优先新增 USTRUCT 事件走 EventBus，不要在 A 里持有 B 的指针直调。
- 广播必须有订阅方（通常是 HUD / Widget / 组件）；无订阅方的广播是死代码。
- 强因果时序（例如任务完成立即发奖）允许直接调用，但要在注释里写明豁免原因。
- 订阅方在 `EndPlay` / `BeginDestroy` 解绑，避免野回调。

## 4. 数据表与 DataManager

- 全局表统一走 `UPP_DataManager`（`UGameInstanceSubsystem`，`Config=Game`），不要各系统自己 `LoadObject`。
- 配置段：`Config/DefaultGame.ini` → `[/Script/PersonalProjectCore.PP_DataManager]`
  - `ItemDataTable=/Game/PersonalProject/Data/DT_Items.DT_Items`
  - `QuestDataTable=/Game/PersonalProject/Data/DT_Task.DT_Task`
  - `CharacterDataTable=/Game/PersonalProject/Data/DT_Characters.DT_Characters`
- 实例级表（商人商品 `DT_ShopInventory`）留在 `UPP_ShopComponent`，不放 DataManager。
- 武器属性表 `DT_WeaponData` 与物品表 `DT_Items` 相互独立，RowName 必须一致；外观 `WeaponMeshMap` 的 Key 也必须与 RowName、ItemID 三者一致。
- 查询模板：`FindRowFromTable<RowType>`；任务表缺行会 warn，物品表不 warn。

## 5. UI 层

- `APP_HUD` / `APP_LobbyHUD` 实现 `IPP_UIGateway`，实现体在 `PersonalProjectUI/Private/Framework/PP_UICrossModule.cpp`。
- 面板创建统一走 `CreatePanelWidget` / `ShowPanel` 模板；层级由蓝图 Widget 的 `OwningLayer` 配置，输入模式由 `UIManager` 栈统一管理。
- `CreateWidget` 的 OwningObject 只能是 `UWorld` / `APlayerController` / `UserWidget` / `GameInstance`，不能传 `AHUD`。
- 常驻 HUD 顶层 Widget 必须 `HitTestInvisible`，`Visible` 会拦截 Slate 鼠标导致视角不转。
- `BindWidget` 控件名必须与 C++ 声明完全一致，否则蓝图编译失败。

## 6. 新代码放哪（决策顺序）

1. 是纯数据/接口/事件/基类/全局服务？→ `PersonalProjectCore`
2. 是背包、商店、采集？→ `PersonalProjectInventory`
3. 是任务、对话？→ `PersonalProjectQuest`
4. 是角色、GameMode、PlayerController、Boss、篝火、GA、AI、关卡 Actor？→ `PersonalProject`
5. 是 HUD、Widget、UIManager、大厅界面？→ `PersonalProjectUI`
6. 需要新的第三方能力？→ 先评估放 `Plugins/`，不要动现有模块边界。

目录内按域分 `Public/<Domain>/` 与 `Private/<Domain>/`，同一功能 .h/.cpp 同域。

## 7. 已知扩展点

- 角色差异化技能：`DT_Characters` → `CharacterAbilityMap` 授予角色专属 GA；`StartupAbilities` 与 `CharacterAbilityMap` 两个入口必须去重，否则同 GA 双实例（体力双倍消耗）。
- 敌人重置：`UPP_EnemyRespawnManager` 由 GameMode 自动创建，敌人 `BeginPlay` 注册；篝火只广播 `PPTags.Events.Enemy.Reset`。
- 存档：`UPP_LocalSaveGame` 已存在，`UPP_SaveGameManager` 方案已定但未实现，不要假设可用。
