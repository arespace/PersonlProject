# 魂类 4 人联机游戏 — 功能实现规划

更新日期：2026-11-03
项目路径：`.`（仓库根目录）
当前分支：`poc/als-integration`
模块：Core / PersonalProject / Inventory / Quest / UI（不新增模块，不改 Build.cs）

---

## 一、功能编号与优先级

| ID | 功能名称 | 优先级 | 预估工时 | 核心模块 |
|---|---|---|---|---|
| F1 | 完美闪避（i-frames） | P0 | 0.5d | Core |
| F2 | 敌人 AI 升级 | P0 | 2d | PersonalProject + Core |
| F3 | 篝火休息重置区域敌人 | P0 | 1d | PersonalProject + Core |
| F4 | 格挡 / 弹反 / 处决 | P0 | 2d | Core + PersonalProject |
| F5 | 武器系统（装备槽 + 技能组切换） | P1 | 2d | Inventory + Core + PersonalProject |
| F6 | 角色差异化 GA | P1 | 0.5d | PersonalProject（纯蓝图） |
| F7 | 篝火间传送 | P1 | 1d | PersonalProject + Core |
| F8 | 精英怪 + 多 Boss | P1 | 1d | PersonalProject（C++ 基类 + 蓝图配置） |
| F9 | 存档落地 + 属性点升级 | P2 | 3d | Core + PersonalProject |
| F10 | Boss 场地门 + 结算 | P2 | 1d | PersonalProject + UI |

---

## 二、每个功能的改动清单

### F1 · 完美闪避（i-frames）

**目的**：翻滚期间短暂无敌，由 GameplayTag 驱动，不硬编码。

新增 GameplayTag（Core / PPTags.h + PPTags.cpp）：
- `Status.Invulnerable`

改现有类（Core / PP_BaseCharacter.cpp::CanBeDamaged）：
```cpp
bool APP_BaseCharacter::CanBeDamaged() const
{
    if (!GetAbilitySystemComponent()) return true;
    if (GetAbilitySystemComponent()->HasMatchingGameplayTag(PPTags::Status::Invulnerable)) return false;
    return true;
}
```

蓝图配置（零 C++）：
- GA_PP_Roll / 翻滚蒙太奇 → RollStartNotify `AddTag(Status.Invulnerable)`，RollEndNotify `RemoveTag`（约 0.35s）

**影响面**：所有伤害路径最终都走 CanBeDamaged，一次改动全站生效。

---

### F2 · 敌人 AI 升级

**目的**：从"永远不结束的 SearchForTarget 循环"升级成感知/脱战/冷却/打断的完整战斗节奏。

新增类：
- `UPP_AIPerceptionComponent`（Core）：视线 LineTrace + 视野锥 + 高度差索敌；Leash 脱战计算；仇恨粘性（短时间不换目标）
- `IPP_Resettable` 接口（Core / PP_InteractableInterface.h 同文件或新文件）：`virtual void ResetToSpawn() = 0`

改现有类：
- `UPP_SearchForTarget`（PersonalProject / GA）：GA 只管攻击执行（MoveTo + Attack），感知交给 AIPerceptionComponent；攻击结束显式冷却（1.5~2.5s，Tag 触发下一轮）；监听 `Events.Enemy.HitReact` 取消待发攻击
- `APP_EnemyCharacter`（PersonalProject）：BeginPlay 记录 SpawnLocation；实现 `IPP_Resettable::ResetToSpawn()`

蓝图配置：
- AI Controller BP 绑上 AIPerceptionComponent；配 SearchRange/LeashRange/VisionAngle/AttackCooldown

---

### F3 · 篝火休息重置区域敌人

**目的**：篝火休息时区域内所有敌人归位满血（魂类核心循环）。

新增 GameplayTag：
- `Event.Enemy.Reset`

新增类：
- `UPP_EnemyRespawnManager`（PersonalProject，GameMode 级 ActorComponent）：按区域（BoxVolume 或 Bonfire 球体半径）维护敌人池；收到 Event.Enemy.Reset 时遍历调用 ResetToSpawn()

改现有类：
- `APP_BonfireActor::Server_Rest`：休息成功后 Broadcast `Event.Enemy.Reset`，半径由 Bonfire.ResetRadius 配置

依赖：**F2 的 IPP_Resettable 接口必须先落地**（敌人实现它）。

---

### F4 · 格挡 / 弹反 / 处决

**目的**：按住右键格挡减伤，精准帧松开触发弹反硬直，弹反成功后可处决。

新增 GameplayTag（Core）：
- `Status.Blocking`、`Status.ParryWindow`、`Status.ParrySuccess`、`Event.Combat.ParrySuccess`

新增类（Core）：
- `UPP_CombatStateComponent`：Blocking 窗口（按住右键）、ParryWindow（松开后 0.2s）、ParrySuccess（弹反硬直 0.8s）

改现有类（Core / PP_BaseCharacter）：
- `OnTakeAnyDamage` 伤害入口：Blocking Tag → 伤害减半；ParryWindow 内受伤 → ParrySuccess + Broadcast Event.Combat.ParrySuccess + 调敌人 `IPP_Staggerable::PlayStaggered()`

新增接口（Core）：
- `IPP_Staggerable`：`virtual void PlayStaggered()`（敌人/Boss 实现：播放硬直蒙太奇 + 停 AI）

新增 GA（PersonalProject 蓝图）：
- `GA_PP_Execution`：订阅 Event.Combat.ParrySuccess → 激活处决蒙太奇；Boss 只打 30% 血量 + 播放处决，精英/小怪可即死

蓝图配置：
- GA_PP_Blocking：按住右键 → Add Blocking Tag；松开 → Remove Tag + 启动 ParryWindow（0.2s GE）

---

### F5 · 武器系统（装备槽 + 技能组切换）

**目的**：换武器 → 切技能组（不同武器给 GA 不同 Tag，激活链不同）。

新增 GameplayTag：
- `Event.Inventory.EquipmentChanged`

改现有类（Inventory / PP_InventoryComponent）：
- 加 `TArray<FPP_EquipmentSlot> EquipmentSlots`（PrimaryWeapon / SecondaryWeapon / Helmet / Chest / Accessory）
- 加 `EquipItem()` / `UnequipItem()` → Broadcast Event.Inventory.EquipmentChanged

新增数据（Core / PP_ItemTypes.h）：
- `FPP_WeaponData`：ItemID / AbilityTags 数组 / 伤害乘数 / 处决蒙太奇名

改现有类（PersonalProject / PP_PlayerCharacter）：
- `OnEquipmentChanged`（实现 Core 的 IPP_EquipmentUser 接口）：根据槽位 ItemID 查 FPP_WeaponData → RemoveAbility(旧) + GiveAbility(新)

---

### F6 · 角色差异化 GA

**目的**：4 个角色真的不同（不同武器 GA、不同数值、不同蒙太奇）。

**C++ 改动：零**。

蓝图配置：
- 新建 GA_PP_Sword_Primary / GA_PP_Sword_Secondary（近战）、GA_PP_Bow_Primary / GA_PP_Bow_Secondary（远程）、GA_PP_Staff_Primary / GA_PP_Staff_Secondary（法师）、GA_PP_Spear_Primary / GA_PP_Spear_Secondary（长柄）
- 4 个角色 BP_PP_PlayerCharacterA/B/C/D 各自的 ASC 里给不同的 GA 集合（不同 Tag 不同数值不同蒙太奇）
- 差异化维度：GE_CoolDown 数值、GE_Damage BaseMagnitude、GE_Cost 数值、蒙太奇、动画蓝图分支

依赖：**F5 武器系统**（因为差异化最终由"装备什么武器决定激活什么 GA"驱动）。

---

### F7 · 篝火间传送

**目的**：激活的篝火可互相传送。

新增 GameplayTag：
- `Event.Bonfire.Activated`

新增类（PersonalProject / GameMode 级 ActorComponent）：
- `UPP_LevelManager`：维护 ActivatedBonfires（BonfireID → Transform）；收到 Event.Bonfire.Activated 时添加

改现有类：
- `APP_BonfireActor::Server_Rest`：Broadcast Event.Bonfire.Activated
- `APP_PlayerState`：加 SavedBonfires（TMap<FName, FTransform>）+ CurrentLevelID；序列化
- `APP_BaseCharacter::TeleportToBonfire(FName BonfireID)`（Core）：查 PlayerState.SavedBonfires → TeleportPhysics 瞬移

依赖：**F3 篝火重置**（共享 Event.Bonfire.Activated + ActivatedBonfires 机制）。

---

### F8 · 精英怪 + 多 Boss

**目的**：介于小怪和 Boss 之间的精英单位；多个 Boss 蓝图子类各自有独立配置。

新增类（PersonalProject）：
- `APP_EliteCharacter`（继承 APP_EnemyCharacter）：BeginPlay 加 `Status.Elite` Tag；GoldDropAmount = 100（敌人默认 10、Boss 默认 500）；可覆盖 HandleDeath 掉稀有材料

蓝图配置（零 C++）：
- BP_Elite_Swordman / BP_Elite_Archer
- BP_Boss_Golem / BP_Boss_Swamp（各自独立配 BossName / BossPhaseThresholds / BossGoldMultiplier / 不同 GA）

**Boss 基类（PP_BossCharacter）已完成**：无需改动，不同 Boss BP 蓝图子类各自配参数即可。

---

### F9 · 存档落地 + 属性点升级

**目的**：GameInstance 启动时读档还原 PlayerState，金币换属性点。

新增类（Core）：
- `UPP_SaveGameManager`（GameInstance 单例 ActorComponent）：Save(FPP_SaveGamePayload) / Load() → 底层调用已有 PP_LocalSaveGame 类；Steam Cloud 以后只改这里一个地方

新增数据（Core）：
- `FPP_PlayerUpgrades`：Strength / Vigor / Dexterity / Intelligence（各 int32）
- 配表 DT_UpgradeStatMapping：StatName → AttributeName → 每级+值

改现有类：
- `APP_PlayerState`（PersonalProject）：加 Upgrades + AvailableUpgradePoints；AddUpgradePoints() / ApplyUpgrade()
- `APP_PlayerCharacter::BeginPlay`：从 PlayerState 读 Upgrades，通过 GE_ApplyUpgrades 应用到 AttributeSet
- `APP_GameInstance`（PersonalProject）：OnStartGame 时 Load()；OnExitGame 时 Save()

---

### F10 · Boss 场地门 + 结算

**目的**：进 Boss 场地触发关门，Boss 死亡自动开门 + 显示结算。

新增 GameplayTag：
- `Boss.ArenaArmed`、`Event.Boss.Dead`、`Event.Settlement.Show`

新增 Actor（PersonalProject）：
- `APP_BossArenaTrigger`（BoxVolume）：进场地 → 给 Boss 加 Boss.ArenaArmed Tag（解锁 AI 搜索）+ 关 ArenaGate
- `APP_BossArenaGate`：订阅 Event.Boss.Dead → 解锁碰撞 + 播放开门动画

改现有类：
- `APP_BossCharacter::HandleDeath`：先 Super，再 Broadcast Event.Boss.Dead + Event.Settlement.Show（payload: BossName, TimeToKill, GoldReward）

新增 Widget（UI）：
- `UPP_SettlementWidget`：订阅 Event.Settlement.Show → 显示通关时间 / 总击杀 / 金币 / 掉落，3 秒后淡出

---

## 三、依赖关系图

```
F1 · 完美闪避 ────────────────────────────────────────────────────┐
                                                                  │
F2 · AI 升级 ── IPP_Resettable 接口落地 ──┐                      │
                                            ├── F3 · 篝火重置 ── F7 · 篝火传送
F4 · 格挡弹反 ── （完全独立）             │                      │
                                            │                      │
F5 · 武器系统 ── GA 标签差异化基础 ── F6 · 角色差异化 GA          │
                                                                  │
F8 · 精英怪 ── （纯蓝图 + 基类）                                ── F10 · Boss 场地门 + 结算
F10 · Boss 场地门 ── Event.Boss.Dead 依赖 Boss 基类              │
                                                                  │
F9 · 存档落地 ── 最后做（等前面所有 Finalize 回调确定）            │
```

---

## 四、并行分组建议

> **Trae 里开多个独立对话窗口**，每个对话写一个分组。分组之间**零文件交叉**，零接口依赖，各自独立 commit。

### 🟢 Group 1（对话窗口 A）— Core 基类改动

| 功能 | 改动文件 |
|---|---|
| F1 完美闪避 | Core / PPTags.h + PPTags.cpp（加 Status.Invulnerable） |
| （F4 格挡弹反的 Core 部分可后续合并进来） | Core / PP_BaseCharacter.cpp::CanBeDamaged |

**独立 commit**：`feat/core-invulnerable-i-frames`

---

### 🟢 Group 2（对话窗口 B）— 敌人 AI 全链路

| 功能 | 改动文件 |
|---|---|
| F2 AI 升级 | Core / 新增 UPP_AIPerceptionComponent、IPP_Resettable 接口 |
| F3 篝火重置 | PersonalProject / PP_BonfireActor.cpp、新增 UPP_EnemyRespawnManager |
|  | PersonalProject / PP_EnemyCharacter.cpp（实现 IPP_Resettable） |
|  | Core / PPTags.h + PPTags.cpp（加 Event.Enemy.Reset） |

**注意**：Group 2 需要先从 Group 1 拉到 IPP_Resettable 接口（Core 层）再继续。所以 **Group 1 和 Group 2 不能完全并行**，Group 1 先提交一个小 commit，Group 2 再拉下来接着写。

**独立 commit**：`feat/enemy-ai-search-reset`

---

### 🟢 Group 3（对话窗口 C）— 武器系统 + 角色差异化

| 功能 | 改动文件 |
|---|---|
| F5 武器系统 | Inventory / PP_InventoryComponent.cpp |
|  | Core / 新增 FPP_WeaponData 结构体、PPTags（加 Event.Inventory.EquipmentChanged） |
|  | PersonalProject / PP_PlayerCharacter.cpp（实现 IPP_EquipmentUser） |
| F6 角色差异化 GA | PersonalProject（纯蓝图配置） |

**独立 commit**：`feat/weapon-equipment-ga-switch`

**和 Group 1 / Group 2 零文件交叉**，可以完全并行。

---

### 🟢 Group 4（对话窗口 D）— Boss 场地门 + 结算

| 功能 | 改动文件 |
|---|---|
| F10 Boss 场地门 + 结算 | PersonalProject / 新增 BossArenaTrigger、BossArenaGate（独立 Actor） |
|  | PersonalProject / PP_BossCharacter.cpp（加 Event.Boss.Dead / Event.Settlement.Show Broadcast） |
|  | UI / 新增 UPP_SettlementWidget |

**独立 commit**：`feat/boss-arena-gate-settlement`

**和 Group 1 / Group 2 / Group 3 零文件交叉**，可以完全并行。

---

### 🟡 Group 5（对话窗口 E）— 格挡弹反

| 功能 | 改动文件 |
|---|---|
| F4 格挡 / 弹反 / 处决 | Core / 新增 UPP_CombatStateComponent |
|  | Core / PP_BaseCharacter.cpp::OnTakeAnyDamage |
|  | PersonalProject（纯 GA 蓝图：Blocking / ParryWindow / Execution） |
|  | Core / PPTags（加 Status.Blocking / Status.ParryWindow / Status.ParrySuccess / Event.Combat.ParrySuccess） |

**独立 commit**：`feat/block-parry-execution`

和 Group 1 共享 `PP_BaseCharacter.cpp::OnTakeAnyDamage`，但改的是**不同函数**（Group 1 改 `CanBeDamaged`，F4 改 `OnTakeAnyDamage`），Git 会自动合并。为避免冲突，Group 5 在 Group 1 之后提交即可。

---

### 🔴 Group 6（最后单线程）— 存档落地

| 功能 | 改动文件 |
|---|---|
| F9 存档 + 属性点升级 | Core / 新增 UPP_SaveGameManager、FPP_PlayerUpgrades、配表 DT_UpgradeStatMapping |
|  | PersonalProject / PP_GameInstance.cpp、PP_PlayerState.cpp、PP_PlayerCharacter.cpp |

**独立 commit**：`feat/save-load-upgrades`

依赖前面所有功能的 Finalize 回调确定（哪些状态要序列化），最后单独做。

---

## 五、并行启动时间线

```
Day 1 上午  09:00  ── Group 1 开始写 ── 10:00 commit ── Group 2 拉下来继续
Day 1 上午  09:00  ── Group 3 开始（和 Group 1 完全并行）
Day 1 上午  09:00  ── Group 4 开始（和 Group 1/3 完全并行）
Day 1 下午  14:00  ── Group 5 开始（在 Group 1 之后拉）
Day 1 下午  17:00  ── 全部 Group 1~5 commit 合到主分支，统一 PIE 验证
Day 2         ── Group 6 单独跑
```

---

## 六、每次 commit 后必须做的事

1. **全模块编译**：关编辑器 → 命令行 Build.bat → 确认 Result: Succeeded
2. **本地 PIE 验证**：
   - 新功能正常触发
   - 没有影响任何已有功能（锁定、ALS 移动、GAS 攻击、Boss 血条、篝火、掉魂、复活）
3. **Git merge 冲突检查**：如果并行 commit 同一个文件不同位置，Git 会自动 merge；如果冲突，手动解决后再 PIE

---

## 七、总结：谁可以并行、谁必须串行

| 关系 | 功能对 |
|---|---|
| ✅ 完全并行 | Group 1+Group 3 同时开 |
| ✅ 完全并行 | Group 1+Group 4 同时开 |
| ✅ 完全并行 | Group 3+Group 4 同时开 |
| ✅ 顺序不严格 | Group 5 在 Group 1 之后（共享 BaseCharacter.cpp） |
| ❌ 不能并行 | Group 2 依赖 Group 1 的 IPP_Resettable 接口（先 Group 1 commit） |
| ❌ 最后单独做 | Group 6（存档落地） |
