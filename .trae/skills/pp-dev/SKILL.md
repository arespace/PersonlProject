---
name: pp-dev
description: PersonalProject UE 5.7 development entry point for planning, C++ changes, build checks, review, and debugging. Use when changing gameplay, GAS, UI, or before build and commit. Do not use for non-UE tasks.
---

# PersonalProject 开发主流程（唯一入口）

所有「写代码 / 重构 / 构建 / 审查 / 调试」请求都从这里开始。细规则按路由表读 `references/`，不要凭记忆写类名、路径或命令。

## 何时用

- 新增或修改 C++（玩法、GAS、AI、UI、数据、接口）
- 重构、模块解耦、批量迁移
- 编译失败、构建报错、PIE 异常、运行时报错
- 提交前审查、蓝图配置收尾、批次交付说明

不用于：纯知识问答、与 UE 或本项目无关的任务。

## 0. 动手前先定位（强制）

1. 判断改动归属哪个模块（见下表）；跨模块改动先确认依赖方向。
2. `Grep` / `Read` 现有实现、同名类、同名资产。**C++ 零引用 ≠ 资产零引用**，删类/删文件前必须搜 `Content`。
3. 判断变更类型，决定是否必须完整编译：
   - 只改 `.cpp` 函数体 → 可 Live Coding（Ctrl+Alt+F11）
   - 改/加 `UPROPERTY`、`USTRUCT` 成员、`UCLASS`、默认组件、Server RPC、`Build.cs`、新增文件 → 关闭编辑器与全部 PIE 窗口后完整编译
4. 新功能有多种合理实现，或预计跨 3 个以上文件 → 先读 `references/feature-planning.md`，不要直接开写。
5. 开始执行前确认：没有其它执行会话在改仓库，编辑器也没有被其它会话占用；有冲突先停，规则见 `references/workflow.md`。

## 1. 项目事实（2026-11-06 核实，不要猜）

| 项 | 值 |
|---|---|
| 引擎 | UE 5.7，`D:\Ue\UnrealEngine\UE_5.7` |
| 工程 | `d:\Ue\UnrealEngine\program\PersonalProject\PersonalProject.uproject` |
| 构建目标 | `PersonalProjectEditor Win64 Development` |
| 编辑器 | `D:\Ue\UnrealEngine\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe` |

`.uproject` 声明五个模块，依赖严格单向：

| 模块 | 职责 | 依赖 |
|---|---|---|
| `PersonalProjectCore` | EventBus、DataManager、GameplayTag、接口、数据头、角色基类、ASC/AttributeSet | 仅引擎 + ALS |
| `PersonalProjectInventory` | 背包、商店、采集 | Core |
| `PersonalProjectQuest` | 任务、NPC 对话 | Core、Inventory |
| `PersonalProject` | 主玩法：角色、PlayerController、GameMode、Boss、篝火、GA/AI | Core、Inventory、Quest |
| `PersonalProjectUI` | HUD、UIManager、全部 Widget、大厅 UI | 以上四个 + UMG/Slate |

- 依赖方向：`Core ← Inventory ← Quest ← PersonalProject ← UI`，禁止反向，禁止新增模块。
- 源码布局：`Source/<Module>/Public/<域>/` 与 `Private/<域>/`（域 = Characters / AbilitySystem / Framework / Inventory / Quest / UI / GameObjects / Game …）。
- 资产：`Content/PersonalProject/{Characters,AbilitySystem,GameObjects,UI,Game,Data,Input,Maps,Animations}`；数据表在 `Content/PersonalProject/Data/`。
- 数据表配置段：`[/Script/PersonalProjectCore.PP_DataManager]`。模块改名后 ini 段名必须同步，CoreRedirects 不作用于 ini 段名。

## 2. 红线（违反即返工）

1. **模块单向依赖**：`Core ← Inventory ← Quest ← PersonalProject ← UI`。Core 不得依赖任何项目模块；UI 可依赖其余四个。新功能落在现有五模块内；全局管理器做成 ActorComponent 挂 GameMode/GameInstance，不写单例类。
2. **接口 BlueprintNativeEvent 只能 `Execute_Xxx(Object, ...)`**，不能经接口指针直调（直调命中 `check(0)` 桩）。具体 UCLASS 的 BlueprintNativeEvent 由 UHT 生成真正分发函数，可以直调。
3. **Live Coding 边界**：仅 `.cpp` 函数体可热更；改反射声明/默认组件/Server RPC/`Build.cs`/新增文件必须关编辑器完整编译。函数内 `static` 变量热更不会重新初始化。
4. **资产操作不可逆**：`.uasset` 是二进制，禁止编辑器外修改；禁止用 PowerShell 复制/移动，必须用 `unreal.EditorAssetLibrary.move_asset`。组件/类解析失败时**不要保存蓝图**（空引用会被固化且无法回滚）。`Content/PersonalProject/`、`Content/Maps/`、`Content/UI/`、`Config/` 已于 2026-11-06 核实纳入 git 白名单、可回滚；但 `Paragon*`、`Scifi_desert_city` 等第三方素材包不入库，删除即永久丢失。仍按「资产操作不可逆」对待，不要用 `git checkout` 覆盖编辑器已加载的资产。
5. **禁止直接改 ALS 插件**（`Plugins/ALS`）：自定义逻辑用子类 / 子蓝图；技能蒙太奇放项目资产目录（`Content/PersonalProject/...`，重定向动画放 Retargeted 目录），不得放插件目录。
6. **技能蒙太奇 Slot 必须 `Locomotion.Default`**（与 ALS 一致）；`DefaultGroup.DefaultSlot` 会丢弃姿势。
7. **GameplayTag 成对声明**：`PPTags.h` + `PPTags.cpp`，声明前加 `PERSONALPROJECTCORE_API`，完整路径 `PPTags.<Ns>.<Tag>`；禁止 `RequestGameplayTag` 临时字符串。细节见 `references/gas-tags.md`。

其他高频硬约束（会导致编译失败或卡死）：

- `UCLASS(Abstract)` 禁止纯虚函数（UE 5.7 仍会实例化，C2259）。
- `UActorComponent` 没有 `HasAuthority`，用 `GetOwner()->HasAuthority()`。
- 私有 `UPROPERTY` 需要蓝图访问时加 `meta=(AllowPrivateAccess="true")`。
- 构造函数里禁止同步加载引用「后加载模块」（UI）蓝图的类；运行时用 `LoadClass<T>()`。
- 持有 UObject 的成员必须 `UPROPERTY()`；成员指针用 `TObjectPtr<>`。
- 跨系统通信走 EventBus / Core 接口，禁止 Actor 互持引用直调；UI 只经接口 + 事件访问业务。
- `check()` 不留在合法 null 场景；改 `if (!IsValid(...)) return` + `UE_LOG`。

## 3. 标准流程

| 阶段 | 做什么 | 读 |
|---|---|---|
| 1 规划 | 澄清目标 → 列 2-3 方案 → 设计/计划文档 → 用户批准 | `references/feature-planning.md` |
| 2 实现 | 按模块落代码；先接口/数据，后逻辑；边写边自检红线 | `references/architecture.md` |
| 3 构建 | 关编辑器 → Build.bat → 按失败关键词分类处理 | `references/build-and-verify.md` |
| 4 蓝图 | 新增 UPROPERTY/组件/BindWidget 时输出配置清单 | `references/blueprint-config.md` |
| 5 审查 | 按 P0/P1/P2 查，只报会导致失效/崩溃的问题 | `references/code-review.md` |
| 6 提交 | 达到完成定义后一个批次一次提交；未明确要求不 commit | `references/workflow.md` |
| 7 排障 | 先根因后修复；3 次修复无效则质疑架构 | `references/debugging.md` |

## 4. 路由表

| 场景 | 读这个 |
|---|---|
| 新功能 / 重构批次 / 需求有多种实现 | `references/feature-planning.md` |
| 多会话协作、批次节奏、完成定义、WIP 上限 | `references/workflow.md` |
| 模块边界、接口、EventBus、数据表该放哪 | `references/architecture.md` |
| 编译、构建命令、失败分类、Live Coding | `references/build-and-verify.md` |
| 新增 GameplayTag、写 GA/GE/ASC | `references/gas-tags.md` |
| C++ 写完后要告诉用户配什么蓝图 | `references/blueprint-config.md` |
| 提交前 / commit 前审查 | `references/code-review.md` |
| 报错、崩溃、行为异常、性能问题 | `references/debugging.md` |
| 角色/动画/武器外观历史坑 | `docs/复盘/ALS_Bug复盘.md`、`docs/复盘/WeaponVisual_Bug复盘.md` |

## 5. 提交与交付

- 默认不提交。用户偏好「一个批次完成后统一提交」，批次中途只做本地构建与 PIE 验证。
- 完成定义见 `references/workflow.md`：编译通过 + 双端 PIE（进战斗、存档链路、重开一局）+ 蓝图清单勾完 + 一次提交；提交前另按 `references/code-review.md` 审查。
- commit message 沿用 Conventional Commits + scope，例如 `feat(weapon): ...`、`fix(enemy): ...`、`chore(PP): ...`；说明「为什么」，不逐条复述 diff。
- 交付时说清三件事：改了哪些文件、蓝图还要配什么（按 checklist）、验证到哪一步（编译 / PIE / 双端）。

## 6. 不要做

- 不新增模块、不整体引入第三方战斗框架、不边优化边拆模块。
- 不为「可能的未来需求」加抽象；改动只覆盖当前任务。
- 未读代码不提议修改；不凭记忆写类名、资产路径、模块名。
- 不把客户端表现当服务端状态（`SetMaterial` / `SetStaticMesh` 不走网络复制）。
- 不在客户端和服务端各写一套判定；权威逻辑放服务端，表现放客户端。
