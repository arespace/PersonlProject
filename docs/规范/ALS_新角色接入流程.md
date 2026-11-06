# ALS 新角色接入流程

> 适用条件：Paragon 或任何 UE5 角色资产，有完整 Skeleton + SkeletalMesh。
> 以下以 Shinbi 为例，Kwang 是第一个接入的（已完成），照此流程复制即可。

---

## 总览

一个完整角色接入 ALS 动画体系，需要**四大阶段**：

```
┌─────────────────────────────────────────────────────────┐
│ 阶段 A：编辑器 GUI（手动操作，约 15 分钟）              │
│  1. 创建 Retarget Target + IK Rig                        │
│  2. Bulk Export 136 个 ALS 动画资产到新骨架              │
└──────────────────────┬──────────────────────────────────┘
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 阶段 B：C++ 工具（6 条控制台命令，约 2 分钟）             │
│  PP.RetargetAlsAnimBP_<角色> → PP.FixBlendSpaces_<角色>   │
│  → PP.FixAlsRefs_<角色> → PP.FixLayers_<角色>             │
│  → PP.FixTurnRefs_<角色> → PP.ClearRetargetSource_<角色>  │
└──────────────────────┬──────────────────────────────────┘
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 阶段 C：角色接入（编辑器 GUI，约 5 分钟）                │
│  1. DT_Characters 加一行 Player_<角色>                  │
│  2. 壳蓝图骨架 + Preview Mesh 修正（如果需要）           │
└──────────────────────┬──────────────────────────────────┘
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 阶段 D：PIE 验证（约 3 分钟）                             │
│  进 Lobby → 转视角 → 走路/跑/跳 → 躯干不拉长              │
└─────────────────────────────────────────────────────────┘
```

---

## 阶段 A：编辑器 GUI 准备（手动）

### A-1. 确认输入资产

在 Content Browser 搜以下资产，确认它们的路径（后面要用）：

| 资产 | 搜什么 | 记下的路径片段 |
|---|---|---|
| **目标 Skeleton** | `*_Skeleton`（不带 SKM_ 前缀） | `/Game/ParagonXxx/.../Xxx_Skeleton.Xxx_Skeleton` |
| **目标 SkeletalMesh** | `SKM_*`（或 `SK_*`） | `/Game/ParagonXxx/.../SKM_Xxx.SKM_Xxx` |

### A-2. 创建 IK Rig（如果还没有）

IK Rig 是 Retarget Target 的前置依赖，告诉 UE 源骨架（ALS 曼尼昆）和目标骨架的骨骼怎么对应。

1. Content Browser → 右键 → **Animation → IK Rig**
2. **Skeleton** 选目标骨架（比如 Shinbi_Skeleton）
3. 命名 `IKR_<角色名>`（如 `IKR_Shinbi`）→ 保存路径建议和 Skeleton 同级
4. 双击打开 → 在 Skeleton Tree 根骨骼右键 → **Add Root**
5. 给肢体链建 FK/IK：ALS 只用简单的下肢 IK + 上肢 FK，不需要 Full Body
   - 建议链：`Spine`（pelvis→head）、`LeftArm`、`RightArm`、`LeftLeg`、`RightLeg`
6. Save

### A-3. 创建 Retarget Target

Retarget Target 提供**源骨架 → 目标骨架**的完整骨骼映射，是 Bulk Export 重定向的核心。

1. Content Browser → 右键 → **Animation → Animation Retargeting → Retarget Target**
2. **Source IK Rig**：选 ALS 曼尼昆的 IK Rig（搜 `IKR_Als`，在 `/ALS/` 下）
3. **Target IK Rig**：刚才 A-2 步建的
4. 命名 `RTG_AlsTo<角色名>`（如 `RTG_AlsToShinbi`）
5. 双击打开 → 确认 **Target 面板 Character Mesh = 目标角色的网格**
6. 确认骨骼映射：ALS 和 Paragon 骨架命名风格很像（pelvis/spine_01/.../neck_01/head），应该大部分自动对好
7. **Chain Mapping** 里检查每条链对应关系，常见 Warning 可以忽略（见「常见 Warning」一节）
8. Save

### A-4. 批量导出 136 个 ALS 动画序列/蒙太奇

这一步**只能编辑器 GUI 做**——UE 没提供 Bulk Export 重定向的公共 C++ API。

**为什么是 136 个**：ALS 插件里大约 127 个动画序列（`A_Als_*`）+ 9 个蒙太奇（`AM_Als_*`），混合空间和分层蓝图不走这个流程（由阶段 B 的 C++ 工具处理）。

**操作**：

1. **先建目录**：`Content/PersonalProject/Animations/Als<角色名>`（如 `AlsShinbi`）

2. **进 ALS 动画源目录**：Content Browser 地址栏粘贴 `/ALS/ALS/Animations` → 回车

3. **筛选类型**：Content Browser 顶部 **Class Filters**（漏斗图标）→ 只勾 **Anim Sequence + Anim Montage**，取消 Blend Space / IK Rig / Pose Asset / Anim BP

4. **全选 + Bulk Export**：
   - 目录面板聚焦 → `Ctrl+A` 全选
   - 右键 → **Asset Actions → Bulk Export**（不是 Export 也不是 Export Selected）

5. **填写 Bulk Export 对话框**（**关键！**）：

   | 字段 | 填什么 | 注意 |
   |---|---|---|
   | Export Path | 步骤 1 建的 `Als<角色名>` 目录 | |
   | Export Type | Export Source Asset | 不要选 Export FBX |
   | Asset Type | 只勾 **Animation** | 取消 Blueprint / IK Rig / Blend Space |
   | **Use Remapped Skeleton** | **勾选！** | 不勾 = 跳过重定向 |
   | Remapped Skeleton | 目标 Skeleton（如 `Shinbi_Skeleton`） | |
   | Retarget Asset | A-3 建的 RTG | |
   | **Suffix** | **填角色名**（如 `Shinbi`） | 不要填字面量 "Suffix" |
   | Prefix | 留空 | |

6. **点 Bulk Export** → 等 2~5 分钟（磁盘越快越短）

7. **验证结果**：
   - `Als<角色名>` 目录里应该有 ~136 个资产
   - 名字后缀正确：`A_Als_Walk_ForwardShinbi`（不是 `A_Als_Walk_ForwardSuffix`）
   - 双击打开一个 → 右上角 Skeleton 下拉 = 目标 Skeleton（不是 SK_Als）
   - 预览窗口里骨骼姿势正常

---

## 阶段 B：C++ 工具流水线（6 条命令）

### B-0. 前置条件

1. **热编译**：编辑器里 `Ctrl+Alt+F11`（用 Live Coding，离线 VS 构建会被 UE 拒绝）
2. 等 Output Log 出现 `Live Coding succeeded`

### B-1. 6 条控制台命令（依次执行）

> 命令前缀 `PP.<功能>_<角色名>`。完整列表见文末「命令速查表」。

| 顺序 | 命令 | 作用 | 预期日志 |
|---|---|---|---|
| 1 | `PP.RetargetAlsAnimBP_<角色>` | 复制壳蓝图 + 21 分层蓝图 + 9 混合空间到目标目录，换骨架 + 重映射内部动画引用 | `[RetargetTool] BS_Als_...Shinbi 完成` ×9 + `AB_Als_...Shinbi 完成` ×22 |
| 2 | `PP.FixBlendSpaces_<角色>` | 9 个混合空间骨架强制 → 目标 Skeleton，采样动画 → `*<角色>` | `[BlendSpace] BS_Als_LookShinbi：替换采样动画 N 处，当前骨架=Shinbi_Skeleton` ×9 |
| 3 | `PP.FixAlsRefs_<角色>` | 扫目录内全部资产，残留曼尼昆引用 → `*<角色>` | `[RetargetTool] 修复 N 个资产，共替换 M 处` |
| 4 | `PP.FixLayers_<角色>` | 21 个分层蓝图的 LinkedAnimGraph / LinkedAnimLayer 节点 InstanceClass → `AB_Als_*<角色>` | `[Layers] ...InstanceClass：AB_Als_DefaultKwang_C → AB_Als_DefaultShinbi_C` |
| 5 | `PP.FixTurnRefs_<角色>` | 复制 `AIS_Als_Default` → `AIS_Als_<角色>`，8 处转身序列换重定向版，切换壳蓝图 Settings 引用 | `[FixTurnRefs] 替换动画引用 8 处` |
| 6 | `PP.ClearRetargetSource_<角色>` | 清掉所有序列继承的曼尼昆 RetargetSource + 重建压缩数据 | `[RetargetSource] 已清掉 N 个序列的重定向源` |

### B-2. 每步注意事项

- **每条跑完等日志打完**再跑下一条，不要并发
- 每条跑后会自动 MarkPackageDirty，但**全部跑完才需要 File > Save All**（Save All 之间的命令可以不保存）
- 如果某条报 `MissingAnimations`：说明 `Als<角色>` 目录里缺某些动画，去 ALS 源目录手动 Bulk Export 补漏

### B-3. 常见问题

**Q: `PP.FixLayers_<角色>` 报 `找不到 AB_Als_XxxKwang<角色>`？**
A: 壳蓝图如果是手动 Content Browser 复制的（不是命令复制），UE 会自动把内部 InstanceClass 重定向到之前角色的版本。工具里已经有 `StripKnownSuffixes()` 处理这种情况，但 KnownRoleSuffixes 数组里如果没加你的新角色后缀，就会漏处理。在 C++ 里加一行 `TEXT("<角色名>")` 到 KnownRoleSuffixes 数组，热编译重跑 `PP.FixLayers_<角色>`。

**Q: 壳蓝图手动复制后编译报 `Linked instance node ... targets instance class ... incompatible`？**
A: 壳蓝图的 Skeleton 没改成目标 Skeleton。双击壳蓝图 → 右上角 Skeleton 下拉选目标 Skeleton → Compile → Save。

---

## 阶段 C：角色接入（DT_Characters）

Content Browser 搜 `DT_Characters`（或你项目里角色数据表的名字）→ 打开 → 复制一个已有角色行 → 改成：

| 字段 | 值 |
|---|---|
| Row Name | `Player_<角色名>` |
| DisplayName | 角色显示名 |
| CharacterMesh | 目标 SkeletalMesh（如 `SKM_Shinbi`） |
| AnimBlueprintClass | 刚生成的壳蓝图（如 `AB_Als_Shinbi`，在 `Als<角色名>` 目录里） |

File > Save All。

---

## 阶段 D：PIE 验证

1. **PIE 启动**：`Play` → 等 Lobby 加载
2. **选角界面** → 选你的新角色 → 进入 Lobby
3. **必测 6 项**：

| 测试 | 操作 | 通过标准 |
|---|---|---|
| 躯干拉长 | 站立不动 + 快速左右转视角 | 不拉长、不扭曲 |
| 走路 | W 前进 | 双脚不穿插、步伐连贯 |
| 跑步 | W + Shift | 同上 |
| 跳跃 | Space | 起跳滞空落地姿态正常 |
| 原地转身 | 站立 + 快速视角转 90°/180° | 触发转身蒙太奇、姿势自然 |
| 上下身混合 | 右键瞄准 + W 走路 | 上身瞄准、下身走路不冲突 |

4. **看日志**：`LogTemp` 里不应该有 Error 级别的 ALS 相关报错。`No Inertialization node found`（AnimGraphNode_Slot_2/3）是 ALS 原生行为，无害可忽略。

---

## 命令速查表

### 通用模板

```
PP.<功能>_<角色名>
```

| 功能 | 命令前缀 | 作用 |
|---|---|---|
| 复制壳蓝图 + 分层蓝图 + 混合空间 + 重映射内部动画引用 | `PP.RetargetAlsAnimBP_` | 阶段 B-1 第 1 条 |
| 混合空间骨架 + 采样动画修复 | `PP.FixBlendSpaces_` | 阶段 B-1 第 2 条 |
| 残留曼尼昆引用扫描 | `PP.FixAlsRefs_` | 阶段 B-1 第 3 条 |
| 分层蓝图 InstanceClass 修复 | `PP.FixLayers_` | 阶段 B-1 第 4 条 |
| 转身数据资产修复 | `PP.FixTurnRefs_` | 阶段 B-1 第 5 条 |
| 清 RetargetSource | `PP.ClearRetargetSource_` | 阶段 B-1 第 6 条 |

### 已注册命令

Kwang（默认 ctx，不带后缀也能跑）：
```
PP.RetargetAlsAnimBP              PP.FixBlendSpaces                PP.FixAlsKwangRefs
PP.FixLayersKwang                 PP.FixTurnRefs                   PP.ClearRetargetSource
```

Shinbi：
```
PP.RetargetAlsAnimBP_Shinbi       PP.FixBlendSpaces_Shinbi         PP.FixAlsShinbiRefs
PP.FixAlsShinbiLayers             PP.FixTurnRefs_Shinbi            PP.ClearRetargetSource_Shinbi
```

诊断/工具性命令：
```
PP.FindTurnRefs                   # 反查转身序列引用（只读）
PP.CleanPlayerBPDefaults          # 清理 BP_PP_PlayerCharacter 遗留默认值
PP.CheckAlsKwangBones             # 诊断躯干骨骼位移（Kwang 专用）
```

### 新增角色命令的方式

在 `PP_AlsRetargetTool.cpp` 里做三件事：

```cpp
// 1. 新增 ctx 构造函数
static FRetargetContext MakeWolfCtx()
{
    return {
        TEXT("/Game/PersonalProject/Animations/AlsWolf"),  // DestFolder
        TEXT("Wolf"),                                        // RetargetSuffix
        TEXT("/Game/ParagonWolf/Meshes/Wolf_Skeleton.Wolf_Skeleton")
    };
}

// 2. 6 个包装函数
static void CmdRunWolf()              { Run(MakeWolfCtx()); }
static void CmdFixWolfRefs()          { FixExistingReferences(MakeWolfCtx()); }
static void CmdFixTurnRefsWolf()      { FixTurnSequenceRefs(MakeWolfCtx()); }
static void CmdFixLayersWolf()        { FixAlsKwangLayers(MakeWolfCtx()); }
static void CmdFixBlendSpacesWolf()   { FixKwangBlendSpaces(MakeWolfCtx()); }
static void CmdClearRetargetSrcWolf() { ClearRetargetSource(MakeWolfCtx()); }

// 3. 6 条 FAutoConsoleCommand 注册（照抄 Kwang/Shinbi 的格式改名字）
```

热编译后直接生效，不用改函数体里的任何一行。

---

## 常见 Warning

### IK Retargeter 相关（A-3 步 Retarget Target 时出现）

| Warning | 含义 | 需要处理吗 |
|---|---|---|
| `Root chain too short` + `missing target pelvis None` | 目标骨架可能没有单独的 root 骨骼（只有 pelvis），或 Root Motion 映射找不到 pelvis | **不用处理**——Root Motion 只影响带根运动的蒙太奇（Mantle/Roll），普通 136 个动画序列都用不到 |
| `XXX has a different parent in the template` | 目标骨架某根骨骼的父链结构和 ALS 不完全一致 | **不用处理**——IK Retargeter 能跨不同父链重定向 |
| `Spine chain expanded beyond template by N bones` | 目标骨架比 ALS 多/少 N 节脊柱 | **不用处理**——链式重定向不要求链数严格一致 |

---

## 踩坑清单（按踩过的频率排序）

| 坑 | 症状 | 根因 | 解决 |
|---|---|---|---|
| Bulk Export Suffix 填成字面量 | 资产名字是 `A_Als_WalkForwardSuffix` 不是 `A_Als_WalkForwardShinbi` | 把对话框里 Label 当值填了 | 删错的资产重导，Suffix 字段填角色名 |
| 壳蓝图 SourceAnimBlueprints 里漏 | 21 分层蓝图都复制了，壳蓝图没生成 | C++ 里 SourceAnimBlueprints 数组忘了加壳蓝图路径 | 加 TEXT("/ALS/ALS/Character/AB_Als_Monolithic") 到数组第 0 行 |
| 手动复制壳蓝图后 UE 自动重定向 InstanceClass | `PP.FixLayers_` 报找不到 `AB_Als_XxxKwangShinbi`（双后缀） | UE Content Browser 复制时自动把类引用重定向到已有同名的 Kwang 版 | C++ 工具里加 `StripKnownSuffixes()` 先剥旧后缀再加目标后缀 |
| Bulk Export 勾了 Export FBX 不是 Export Source Asset | 导出的是 .fbx 不是 .uasset | Bulk Export 对话框里 Export Type 选错 | 选 Export Source Asset |
| DuplicateAsset 生成 transient 对象没持久化 | 命令跑完磁盘上 0 个 AB/BS，日志里全是「完成」但 MarkPackageDirty 没落盘 | UE 插件资产 DuplicateAsset 某些情况返回 transient 对象 | Content Browser 手动 Duplicate 绕过 |
| 壳蓝图 Skeleton 没改 | 编译报 `Linked instance node targets instance class incompatible` | 壳蓝图是手动复制的，ProcessAsset 里改 Skeleton 的逻辑没走到它 | 双击壳蓝图 → Skeleton 下拉 → 选目标 Skeleton → Compile → Save |
| AIS 转身数据资产漏 | 躯干拉长（和 Kwang 第一个 bug 症状一样） | `PP.FixTurnRefs_<角色>` 没跑，或跑了但壳蓝图 Settings 引用没切过去 | 重跑 `PP.FixTurnRefs_<角色>`，确认壳蓝图 Settings 指向 `AIS_Als_<角色>` |

---

## 完整角色清单（已完成）

| 角色 | 骨架路径 | 资产后缀 | 目录 | 状态 |
|---|---|---|---|---|
| Kwang | `/Game/ParagonKwang/.../Kwang_Skeleton` | `Kwang` | `AlsKwang` | ✅ |
| Shinbi | `/Game/ParagonShinbi/.../Shinbi_Skeleton` | `Shinbi` | `AlsShinbi` | ✅ |
| Wolf（Shinbi 狼形态） | `/Game/ParagonShinbi/.../Shinbi_Wolf_Skeleton` | `Wolf` | `AlsWolf` | 📋（可按 Shinbi 流程快速复制） |
| ... | ... | ... | ... | 📋 |
