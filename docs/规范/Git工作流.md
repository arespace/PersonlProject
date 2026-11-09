# Git 工作流

> 状态：现行 · 最后更新：2026-11-09 · 适用范围：单人开发、UE 5.7、仓库 `PersonalProject`
> 本文只写「怎么做」。每一步都是可以直接照着敲的命令或明确的判断动作。

---

## 1. 分支判据

**唯一判据：这次改动会不会动 `.uasset` / `.umap` / `Config/`。**

| 会动资产或配置 | 做法 |
|---|---|
| 会（蓝图、材质、动画、地图、`DefaultEngine.ini` 等） | **直接在 `main` 上做，不开分支** |
| 不会（纯 `.h` / `.cpp` / `Build.cs` / `docs/`） | 开短命分支 |

判断命令（开工前先看清要改什么）：

```powershell
git status --short
git diff --name-only
```

理由：资产和 `Config/` 在多个分支之间无法合并（见 §4），一旦并行就会产生只能重做的冲突；纯代码文件可以正常 rebase/merge，适合隔离。

**这与「功能大小」无关。** 反例：拆 Core 模块依赖、重排五模块单向依赖——纯代码、体量很大、容易中途编译不过，**必须走短命分支**，因为它需要反复 rebase 到最新 `main` 并单独验证，而不是因为它「大」。反过来，改一个 Blueprint 的小默认值——体量很小，**也直接在 `main` 上做**，因为它动了 `.uasset`。

---

## 2. 在 `main` 上做大功能的保护手段

1. **开工前打回滚点**

```powershell
git status --short          # 必须无输出
git tag pre-<批次名>        # 例：pre-p4-equipslot
git push origin pre-<批次名>
```

2. **每完成一个能编译、能验收的批次提交一次**
   - 「能验收」= 编辑器能打开、PIE 能跑通该批次的目标路径，不是「代码写完了」。
   - 批次越大，回滚成本越高；拆小批次的唯一目的是让 `git revert` 只需撤一小段。

3. **WIP = 1，一次只允许一个在制品**
   - 同一时刻只有一件事处于「改到一半」，其余要么已提交、要么还没开始。
   - **这条最关键**：在 `main` 上直接干活没有分支兜底，一旦同时开着两条改动线，`git status` 就无法表达「哪些改动属于哪次提交」，回滚时必然连带撤回别人的成果；而 `WIP = 1` 让工作区始终等价于「最后一次提交 + 一个逻辑改动」，随时可以整体丢弃或整体提交。

---

## 3. 小功能分支的开合

```powershell
# 开
git switch -c feat/<名字>        # 新功能
git switch -c fix/<名字>         # 修 bug

# 收（在分支上，先把提交 rebase 到最新 main）
git fetch origin
git rebase origin/main

# 合（在 main 上，只允许快进）
git switch main
git merge --ff-only feat/<名字>

# 立刻删
git branch -d feat/<名字>
git push origin main
```

规则：

- **只允许 `--ff-only`**，不产生 merge commit；rebase 冲突就地解决后再 `git rebase --continue`。
- 合并后**立刻删分支**，不留「以后可能还要用」的分支。
- **分支活过三天就作废**：改回 `main` 上做（按 §1 判断是否属于资产改动）。短命分支的价值在于「几小时内完成」，超过三天必然落后 `main`，rebase 成本超过收益。

---

## 4. 资产冲突的处理

`.uasset` / `.umap` 是二进制，**冲突不可合并**。

遇到时：

1. **停下**，不要点「自动解决」，不要用 `git checkout --theirs` 之类猜。
2. 确认两边各自改了什么（可让编辑器各开一次对比，或看谁改的是真需求）。
3. **选定一边保留，另一边在选定的版本上手工重做**。
4. 重做完再提交，提交信息里写清「XXX 重做于 YYY 之上」。

预防：这是 §1 判据存在的唯一原因——只要「会动资产就不开分支」，这类冲突根本不会产生。

---

## 5. 提交规范

- **一次编译通过 = 一次提交**。编译不过就不要提交。
- **一次提交只做一件事**。混了两件事就 `git add -p` 拆开，或放弃这次提交重新分组。
- **资产改动与代码改动分两条提交**。典型场景：C++ 改了默认值、蓝图里也同步改了，先提交代码，再提交资产。
- **信息格式**：`类型(范围): 做了什么`

```
feat(weapon): 装备时按 DT_Items 的 WeaponConfig 挂载网格
fix(inventory): 修复 OnRep_Equipped 重复通知外观
refactor(equip): 装备槽统一为 EPP_EquipSlot
chore(repo): .gitignore 改为黑名单式
docs(git): 新增 Git 工作流规范
```

- **类型限定五种**：`feat` / `fix` / `refactor` / `chore` / `docs`。其它词（`update`、`modify`、`优化`）不用。
- 正文只在「为什么」不明显时补写，写影响面与回滚方式，不写「改了哪些文件」。

---

## 6. 中文编码约定

**提交信息一律走文件，不用 `-m`：**

```powershell
git commit -F C:\Users\<你>\AppData\Local\Temp\msg.txt
```

- 文件必须是 **UTF-8**（本仓库用编辑器/`Write` 工具生成的文件默认就是）。
- 不写 `git commit -m "中文..."`，也不写 PowerShell heres-string 直接传中文。

**含中文的 PowerShell 脚本存为 UTF-8 with BOM。**

根因：**PowerShell 5 默认按 ANSI（本机为 GBK）读取无 BOM 的 `.ps1`**，把脚本里的 UTF-8 中文读成乱码（如「敌人朝向」→「鏁屼汉鏈濆悜」）；和 Git 无关。

因此 **不需要设置 `i18n.commitEncoding` / `i18n.logOutputEncoding`**——它们改变不了「脚本被按错误编码读取」这个成因，设置了只会让人误以为问题已解决。

---

## 7. 禁止事项

| 禁止 | 替代做法 |
|---|---|
| 对已推送的历史 `rebase` / `push --force` | 用 `git revert <sha>` 反向提交；确需改写且已确认无人依赖时，用 `--force-with-lease` 并先告知 |
| 在编辑器运行时切分支 / 拉代码 | 先关编辑器，再切分支 |
| 新增第三方素材包直接 `git add` | 先在 `.gitattributes` 加该目录的 LFS 规则，再 `git add`（否则大文件进普通 Git，push 会失败且历史难清） |
| 同一时间多个窗口提交 | 只允许一个窗口 `git add` / `commit`；其它窗口只读 |

---

## 8. 每批次固定动作

**开工前**

```powershell
git status --short        # 必须无输出，有改动先各自归属并提交
git switch main           # 确认在主线
git pull --ff-only        # 与远端对齐
git tag pre-<批次名>      # 回滚点
git push origin pre-<批次名>
```

并确认 **UE 编辑器已关闭**（否则切分支、改 `Config/`、改反射相关的头文件都会出问题）。

**收工**

```powershell
# 关编辑器 → 完整编译通过
git add <具体文件>         # 不用 git add -A，避免误加本地杂物
git commit -F <UTF-8 文件>
git push origin main
```

验收（能编译 + 目标路径能在 PIE 跑通）没做到就不要推送。

---

## 9. 分支结构示例

> 下面用 2026-11-09 的实际状态作为样本。**其中的 hash 与日期都是快照，仅用于说明结构关系，不要当作当前状态查询**——要查现状一律用 `git log --oneline --graph --all`、`git tag --list`、`git branch -vv`。

### 9.1 分叉图

```
                    7436a05  ← 共同起点（重写历史之前的旧 main）
                       │
      ┌────────────────┴─────────────────┐
      │                                  │
   7f00559                            61e8d7a
   ├─ 分支 backup/pre-squash-20261109   ├─ 分支 main
   └─ tag  archive/pre-squash-20261109  └─ 远端 origin/main
   52 条旧提交，原始 hash                47 条提交，hash 已全部重写
```

### 9.2 关键事实：备份是平行副本，不是旧版本

历史重写（squash）会**重建**每条提交，因此：

- 同一次改动在新旧两条线上的 **hash 不同**，`7f00559` 与 `61e8d7a` 之间没有「新 / 旧」的先后关系；
- `7f00559` **不是** `main` 的祖先——`git merge-base --is-ancestor 7f00559 main` 返回非零；
- 两条线从 `7436a05` 分叉，各自向前，**备份线上的提交不能 `cherry-pick` 到 `main` 后当作「同一提交」对待**（会产生内容重复的提交）。

所以备份的用途只有一个：**万一重写过程出错，能回到原始历史重新来过**。它不是「上一版代码」，不要从它上面取改动、也不要把它合进 `main`。

### 9.3 四条引用各自的用途与删除时机

| 引用 | 类型 | 用途 | 删除时机 |
|---|---|---|---|
| `main` | 分支 | 唯一开发主线，所有日常提交落在这里 | **永不删** |
| `origin/main` | 远端跟踪引用 | 远端镜像，用于 `rev-list` 判断同步、`pull --ff-only` 对齐 | 不手动删（由 `fetch --prune` 维护） |
| `backup/pre-squash-20261109` | 分支 | 重写前的平行副本，可继续在其上工作 | **P4.2 完成、`main` 稳定后**可删（远端同名分支一并删） |
| `archive/pre-squash-20261109` | tag | 不可变锚点，保证这套历史不被回收 | 可长期保留，**确认历史无误后再删** |

### 9.4 分支与 tag 指向同一提交时的区别

`backup/pre-squash-20261109` 与 `archive/pre-squash-20261109` 当前指向同一个提交 `7f00559`，但作用不同：

- **分支是可变指针**：删了可以随时用 `git branch <名字> <sha>` 重开，且会被后续提交推着走；
- **tag 是不可变锚**：它本身保证该提交不被 `git gc` 回收。

因此**精简时删分支、留 tag** 即可——既省掉一个会出现在 `git branch` 里的干扰项，又不会丢失任何提交。反过来只删 tag 留分支也能保住提交，但分支名容易被误当成「活动开发分支」，风险更高。

> 附带约定：分支与 tag **不要同名**。同名会让 `git rev-parse <名字>` 报 `refname ... is ambiguous`，且 `git push origin <名字>` 无法确定推哪个引用命名空间。本仓库曾出现 `backup/pre-squash-20261109` 分支与 tag 同名，已把 tag 改为 `archive/pre-squash-20261109` 消除歧义。
