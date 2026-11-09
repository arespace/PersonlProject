# 调试（根因优先）

## 铁律

```text
没有完成根因调查，不得提出修复。
```

症状修复视为失败。连续 3 次修复无效，停止改代码，质疑架构或设计前提。

## 四阶段

### 1. 根因调查（先于任何改动）

- 完整读报错：文件、行号、错误码、调用栈，不跳读。
- 稳定复现：记录确切步骤；不能稳定复现就先加日志，不猜。
- 查最近改动：`git diff`、`git log`、新依赖、配置变化。
- 多组件系统分段取证：在边界打日志，确认数据从哪一层开始错。
- 追溯数据流：坏值从哪来、谁传进来的，一路追到源头，不在症状处打补丁。

### 2. 模式分析

- 在代码库找**能正常工作的同类实现**，逐项对比差异。
- 读参考实现要读全，不跳读。
- 列出依赖：配置、资产、模块、初始化顺序。

### 3. 假设与最小验证

- 一次只提一个假设：「我认为 X 是根因，因为 Y」。
- 用最小改动验证，一次只动一个变量。
- 失败就换新假设，不在旧修复上叠加。

### 4. 修复与验证

- 先有可复现的失败场景，再改代码。
- 只修根因，不顺带重构。
- 修复后重跑原复现步骤 + 检查日志无新增 Warning/Error + 完整编译（若改了反射声明）。
- 记录根因到交付说明，方便写进项目记忆。

## UE 排障工具

| 目的 | 做法 |
|---|---|
| 看运行日志 | `Saved/Logs/PersonalProject.log`；PIE 时可用 `-log` |
| 定位时序 | `UE_LOG(LogTemp, Warning, TEXT("[Tag] ..."))`，带统一前缀 |
| 看 AI/感知 | 控制台 `ShowDebug`、Gameplay Debugger（`'` 键） |
| 看 GAS | 打印 ASC 激活的技能、`HasMatchingGameplayTag`、GE 列表 |
| 看网络 | PIE 用 Listen Server + 1 Client；确认 RPC 端与 `HasAuthority` |
| 看资产解析 | 启动日志搜 `Failed to load` / `CreateExport` / `Failed to find script package` |

## 本项目已验证根因（先查这些）

| 症状 | 根因 | 验证方式 |
|---|---|---|
| 蓝图 `Component class is not set`、变量丢失 | 组件类缺 CoreRedirects 重定向 | 查 `DefaultEngine.ini`，反射路径剥掉 `U`/`A` 前缀 |
| `Failed to find script package for import object 'Package /Script/PersonalProjectUI'` | 模块尚未注册时加载了 UI 模块蓝图（构造函数同步加载） | 对照日志时间戳与 `Engine is initialized`；改运行时 `LoadClass<T>()` |
| DataManager 三张表路径为空 | `DefaultGame.ini` 段名未随模块改名 | 查 `[/Script/PersonalProjectCore.PP_DataManager]` |
| 鼠标视角完全不动 | IMC 优先级被抢，或 HUD 顶层 Widget 非 `HitTestInvisible` | 查 IMC priority / Widget 可见性 |
| 角色没有动画、AnimInstance 为空 | 网格路径误写骨架、蓝图残留旧 AnimBP、构造函数默认值被空值覆盖 | 查 `PostInitializeComponents` 强制设置与蓝图配置 |
| Kwang 躯干拉伸 / 转视角瞬拉 | 加性动画 Base Pose 或 RetargetSource 残留旧骨架 | 跑 `PP.FixAlsKwangRefs` / `PP.ClearRetargetSource`，重导动画 |
| ALS Ensure 冻结 PIE | `IsRagdollingAllowedToStart` 缺 pelvis/spine 骨骼 | 已改软检查；查是否又被还原 |
| 死亡后不复活 | 死亡 GA 蒙太奇回调不可靠 | 查兜底 Timer 是否启动、`bRespawnHandled` 守卫 |
| 复活后不能移动 | InputComponent 未恢复 / `ClientOnRespawn` 未走到 | 查 `HandleRespawn` 统一的客户端输入恢复 |
| 复活后抖动 | 锁定目标 + 残留速度 | 查 `StopMovementImmediately`、速度清零、TeleportPhysics |
| 技能只能放几次 | 同一 GA 被 `StartupAbilities` 与 `CharacterAbilityMap` 重复授予 | 看 ASC 技能实例数、体力是否双倍消耗 |
| 受击后锁定失效 | 受击逻辑里调了 `ReleaseLockOn` | 全局搜 `ReleaseLockOn` 调用点 |
| 武器不显示 / 卡在角色原点 | socket 不存在，`AttachToComponent` 失败 | 查 socket 是否存在 + 骨骼索引是否有效 |
| 武器跟随脚步而非手 | 挂到 `ik_hand_gun`（ALS AnimBP 不驱动该 IK 骨骼） | 改用 `weapon_r` |
| 按 F 装备无响应 | 按键被 GA 消费 | 用 F9 调试键验证，或改走 GA 内的装备逻辑 |
| 大厅进空地图 | `BP_LobbyGameMode` 未配 `HUDClass` | 查 GameMode 构造函数与蓝图默认值 |
| 任务卡住不可提交 | 购买/拾取的道具 ID 与任务目标不一致 | 对 DataTable RowName 与任务条件 |
| 广播了但 UI 不更新 | 没有订阅方，或订阅在错误对象上 | 查 Broadcast 的调用栈与 Subscribe 位置 |

## 危险信号（出现即回到阶段 1）

- 「先快速修一下，之后再查」
- 「改这个试试」
- 「一次改几处再跑」
- 「不验证了，手动看看」
- 「应该是 X」
- 连续 2 次修复失败后还想再试一次
- 每次修复都在不同位置冒出新的共享状态问题

出现以上任一条：停止修改，重新取证。
