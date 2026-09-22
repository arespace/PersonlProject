# Lua 基础练习

这里是纯 Lua（不依赖 UE / UnLua）的基础语法练习，每个文件讲一个主题，带中文注释和 `print` 输出。

## 目录

| 文件 | 内容 |
|------|------|
| `Main.lua` | 入口，按顺序运行下面全部主题 |
| `01_VariablesAndTypes.lua` | 变量、类型、type()、local、字符串拼接 |
| `02_Tables.lua` | 表：数组 / 字典、增删改查、#、ipairs / pairs |
| `03_ControlFlow.lua` | if / elseif、for、while、repeat、break |
| `04_Functions.lua` | 函数、多返回值、变长参数、闭包、冒号 vs 点号 |
| `05_Strings.lua` | 字符串：拼接、format、查找、替换、分割 |
| `06_Metatables.lua` | 元表：__index、__newindex、运算符重载 |
| `07_OOP.lua` | 用元表模拟类与继承 |

## 怎么运行

这些文件是纯 Lua，有两种跑法：

### 1. 在 UnLua 里跑（推荐）

在编辑器里按 `` ` `` 打开控制台（如果提示 `no available lua env found`，就先进 PIE 再执行），输入：

```
lua.dofile Practice.Main                    -- 运行全部练习
lua.dofile Practice.01_VariablesAndTypes    -- 只运行某一节
lua.do print("hello")                       -- 直接执行一句 Lua
```

输出会显示在 **Output Log**（LogUnLua）。

> 说明：`lua.dofile` 会先清掉该模块的缓存再 `require`，所以每次执行都会重新跑一遍；`Main.lua` 内部对每个主题也做了同样的清缓存处理。

### 2. 用本地 Lua 5.4 直接跑

这些文件只用 `print`，不依赖 UE，也可以直接用 Lua 5.4 解释器运行：

```
lua Practice/Main.lua
```
