# 构建与验证

## 1. 前置检查（全过才编译）

1. **编辑器是否在运行**：`Get-Process UnrealEditor -ErrorAction SilentlyContinue`
   - 在运行 → 让用户关闭编辑器，或按 Ctrl+Alt+F11 停止 Live Coding 后再完整编译。
   - 新增/修改反射声明时**必须关闭编辑器与所有 PIE 窗口**，Live Coding 无法生效。
2. **内存**：系统提交上限低于 58GB 时，提示关闭 Rider / clangd / 多余编辑器（曾出现 clangd 单进程占 28GB 被 UBA kill）。
3. **Git 状态**：`git status --short`；有未提交改动只提示，不阻止编译。

## 2. 构建命令

```powershell
& "D:\Ue\UnrealEngine\UE_5.7\Engine\Build\BatchFiles\Build.bat" PersonalProjectEditor Win64 Development -Project="d:\Ue\UnrealEngine\program\PersonalProject\PersonalProject.uproject" -WaitMutex
```

- PowerShell 5 语法：不要用 `&&` / `||`。
- 超时设 600000 ms（10 分钟）。
- 构建目标固定 `PersonalProjectEditor Win64 Development`；不要用 `PersonalProject Win64` 代替（会缺编辑器模块）。

## 3. 结果判定

- 成功：输出包含 `Result: Succeeded`（并给出耗时）。
- 失败：输出包含 `Result: Failed`。只看**第一条 error**，后面的多是级联。
- 构建日志：`C:\Users\72883\AppData\Local\UnrealBuildTool\Log.txt`
- 运行日志：`Saved/Logs/PersonalProject.log`

## 4. 失败分类

| 关键词 | 含义 | 处理 |
|---|---|---|
| `Unable to build while Live Coding is active` | 编辑器开着 | 关编辑器或停止 Live Coding |
| `Low on memory. Kill threshold` | 内存耗尽 | 关 clangd / Rider，增大页面文件 |
| `LNK2019 unresolved external` | 链接缺符号 | 查类是否 UCLASS/USTRUCT 完整、模块 `Build.cs` 是否声明依赖、函数是否只声明未实现 |
| `C2065 / C2509 / C4443` | 标识符或头文件问题 | 先看报错行列的完整标识符（常见是接口名多/少一个 `I`）；循环依赖用前置声明 |
| `C2259 cannot instantiate abstract class` | `UCLASS(Abstract)` 里有纯虚函数 | 改成默认实现 + 注释要求子类 override |
| `UHT-Error` / `Unrecognized type` | 反射声明语法错 | 查 UFUNCTION/UPROPERTY 括号、逗号、类型是否 include |
| `Failed to find script package for import object` | 模块未注册时加载了它的蓝图 | 检查构造函数是否同步加载了后加载模块（UI）的蓝图；改为运行时 `LoadClass<T>()` |
| `CreateExport: Failed to load Class / Parent` | 类重定向或父类丢失 | 查 `DefaultEngine.ini` CoreRedirects，路径要剥掉 `U`/`A` 前缀 |
| `Unity file excluded` | 新文件未进 unity build | Adaptive Build 正常行为，不阻塞 |

## 5. Live Coding 与完整编译分界

| 改动 | 方式 |
|---|---|
| 仅 `.cpp` 函数体（含改日志、改数值、改流程） | 编辑器内 Ctrl+Alt+F11 |
| 新增/修改 `UCLASS`、`USTRUCT`、`UENUM`、`UPROPERTY`、`UFUNCTION` | 关编辑器 → Build.bat |
| 新增默认组件、`CreateDefaultSubobject`、Server RPC | 关编辑器 → Build.bat |
| 修改 `Build.cs`、`.uproject`、模块依赖 | 关编辑器 → Build.bat（必要时重新生成项目文件） |
| 新增 `.h` / `.cpp` 文件 | 关编辑器 → Build.bat |
| 修改 `PPTags.h/.cpp` | 关编辑器 → Build.bat（UHT 需重新扫描） |

注意：函数内 `static` 变量在热更时不会重新初始化；依赖它做缓存的逻辑在 Live Coding 下会保留旧值。

## 6. 按改动类型的验证

| 改动 | 必做验证 |
|---|---|
| 服务端权威逻辑（伤害、掉落、装备、复活） | Listen Server + 1 客户端双端 PIE，确认客户端表现与服务端状态一致 |
| UI / Widget | 客户端窗口检查布局、输入是否穿透（HUD 是否 HitTestInvisible） |
| GAS 技能 | 技能能激活、能 EndAbility、体力只扣一次（授予入口无重复）、死亡/复活后仍可放 |
| 新增 UPROPERTY / 组件 | 关闭编辑器完整编译后重开，确认 Details 面板出现字段；蓝图编译无错 |
| 数据表字段 | 打开对应 DT 确认列存在、RowName 未变；DataManager 日志中表路径非空 |
| 武器外观 | 服务端装备后客户端可见；切换武器后外观跟随 |
| 资产移动/重定向 | 重启编辑器，确认无 `Failed to load`；相关蓝图可正常打开 |

PIE 结束后检查日志：

```powershell
Select-String -Path "d:\Ue\UnrealEngine\program\PersonalProject\Saved\Logs\PersonalProject.log" -Pattern "Error|Warning" | Select-Object -Last 30
```

## 7. 提交前

- 完整编译通过（不是热更通过）。
- 双端 PIE 冒烟通过：进战斗 + 存档链路 + 重开一局，完整定义见 `workflow.md` §5。
- `code-review.md` 审查过一遍。
- 蓝图侧改动已按 `blueprint-config.md` 输出清单并逐项确认。
- 一个批次一次性提交；批次中途不提交，不混入其它会话的文件。
