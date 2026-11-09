# 提交前代码审查

## 审查方式

1. `git status --short` + `git diff`（含 staged）确定范围。
2. 对改动文件读完整上下文，不只看 diff 片段。
3. 按 P0 → P1 → P2 顺序报告；风格问题只在隐藏缺陷时提。
4. 每条问题给「文件:行 + 为什么坏 + 具体改法」，不写泛泛建议。

## P0 · 功能链路断裂（直接导致功能无效）

对改动中每个**新增接口 / 新增组件 / 新增广播 / 新增字段**，必须同时存在另一端：

| 断点类型 | 必查 |
|---|---|
| 新增 `IPP_*` 接口 | 有没有实现类写 `_Implementation`？有没有调用方？接口 `BlueprintNativeEvent` 是否只用 `Execute_Xxx` 调用 |
| 新增 ActorComponent | 有没有 `CreateDefaultSubobject` 或运行时 `NewObject` + `RegisterComponent`？只挂蓝图不算 |
| 新增 EventBus Broadcast | 有没有订阅方（HUD / Widget / 组件）？订阅方是否在 EndPlay 解绑 |
| 新增 `FindComponentByClass` | 找不到时是否有 fallback（自动创建或 warning），不能静默返回 null |
| 新增 GameplayTag | Add 与 Remove 是否成对？只有 Add 没有 Remove = 永久状态 |
| GA 改动 | 所有分支是否最终 `EndAbility`？未结束会让 ASC 卡死，同 GA 永久激活失败 |
| RPC / Server 函数 | 是否有 `HasAuthority()` 守卫？客户端调用是否有 Server 转发链路 |
| 新增蓝图字段 | 类默认值是否合理？必须手配时是否附蓝图配置 checklist |
| 新增 UPROPERTY 引用 | 是否 `UPROPERTY()` 且成员用 `TObjectPtr<>`？否则会被 GC 回收 |

## P1 · 编译 / 链接风险（一编就炸）

- 新文件放对模块了吗？源文件由 UBT 自动收集，**不需要**在 `Build.cs` 登记；需要登记的是跨模块依赖。
- 跨模块 `include` 的目标模块是否已经在 `Build.cs` 的 Public/PrivateDependency 中？
- include 顺序：UE 头 → 项目头 → 第三方。
- 模块依赖方向是否被破坏：`Core ← Inventory ← Quest ← PersonalProject ← UI`，Core 不得依赖任何项目模块。
- `UCLASS(Abstract)` 是否含纯虚函数（C2259）；`USTRUCT` 是否有 `GENERATED_BODY()`。
- `UFUNCTION` / `UPROPERTY` 是否重复定义或缺失反射宏。
- 接口方法是否被直接调用（应 `Execute_`）；具体类才可直调。
- 两个分支同时修改 `PPTags.h` 同一命名空间是否会造成合并冲突（不同 namespace 可自动合并）。
- 构造函数是否同步加载了后加载模块（UI）的蓝图；应改运行时 `LoadClass<T>()`。

## P2 · 运行时崩溃 / 稳定性

- `check()` 是否可能在合法 null 场景触发？改 `if (!IsValid(...)) return` + `UE_LOG`。
- Timer / Delegate 是否在 `EndPlay` / 析构清理（`ClearTimer` / `RemoveAll`）。
- `NewObject<UClass>(Outer, NAME_XXX)` 的 Outer 是否是该 Actor / Component，而不是 `GetWorld()`。
- 复制字段是否声明 `Replicated` 且有 `DOREPLIFETIME`；ServerOnly GA 是否被客户端调用。
- 搜索 / LineTrace / 伤害入口是否有 `HasAuthority()` 或 `IsLocallyControlled()` 判定。
- `UActorComponent` 是否误用 `HasAuthority()`（应 `GetOwner()->HasAuthority()`）。
- 私有 `UPROPERTY` 需要蓝图访问时是否加了 `meta=(AllowPrivateAccess="true")`。
- 客户端表现（`SetMaterial` / `SetStaticMesh`）是否被误当服务端状态使用。
- 死亡 / 复活 / 阶段切换是否依赖蒙太奇回调完成（应加 Timer 兜底与重复执行守卫）。

## 项目特定规则

- GameplayTag 必须 `PPTags.h` 声明 + `PPTags.cpp` 定义，声明带 `PERSONALPROJECTCORE_API`，完整路径 `PPTags.<Ns>.<Tag>`。
- 新增 UCLASS 的 `Category` 使用 `PP|` 前缀，否则蓝图面板难搜。
- UI 只经 `IPP_UIGateway` / 其它接口 + EventBus 访问业务，不 Cast 具体业务组件。
- 跨系统通知优先 EventBus；强因果时序的直接调用需注释说明豁免。
- 删除类 / 文件前确认 `Content` 无资产引用（`C++ 零引用 ≠ 资产零引用`）。

## 输出格式

```markdown
## 审查结果 · <commit hash 或「未提交改动」> · <改动简述>

### P0 功能断点（N 项）
- [文件:行] 问题描述 → 建议改法（必要时给代码）

### P1 编译风险（N 项）
- [文件:行] 问题描述 → 建议改法

### P2 稳定性（N 项）
- [文件:行] 问题描述 → 建议改法

### 蓝图配置 Checklist（必须手动配）
- [资产路径] 字段名 = 值（不配会触发 P0）
```

没有问题的等级写「0 项」，不省略章节。蓝图配置清单的写法见 `blueprint-config.md`。
