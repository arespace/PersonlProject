# 武器外观系统 Bug 修复复盘

> 状态：背景资料 · 最后更新：2026-11-06 · 取代关系：无（历史排障记录）
> 日期：2026-11-06 | 分支：poc/als-integration
> 模块：PersonalProject（Characters）+ PersonalProjectInventory（装备槽复制）
> 目标：装备武器时隐藏角色自带烘焙武器，改为显示对应武器的静态网格

---

## 一、Bug 全景图（按发现顺序）

```
装备武器 → 自带武器消失 + 长剑出现
    │
    ├── 1. 编译报错 BlueprintReadOnly on private ── 缺 Meta AllowPrivateAccess
    ├── 2. 编译报错 HasAuthority 找不到 ─────────── UActorComponent 无此方法
    ├── 3. 按 F 完全没反应 ─────────────────────── 接口方法被直接调用 → 断言
    ├── 4. 隐藏后材质变灰、几何仍在 ────────────── M_HiddenWeapon 缺 SkeletalMesh 标记
    ├── 5. 材质槽索引取不到材质 ────────────────── BeginPlay 时网格资产还没赋值
    ├── 6. 服务器生效但客户端看不到 ────────────── SetMaterial 不复制 + Listen Server 双世界
    ├── 7. 长剑出现在身体中间 ──────────────────── socket 名不存在，挂载静默失败
    ├── 8. 长剑停在脚边不跟手 ──────────────────── ik_hand_gun 是 IK 骨骼，ALS AnimBP 不驱动
    ├── 9. 改代码后行为不变 ────────────────────── 函数内 static 数组在 Live Coding 下不重建
    ├── 10. 装备后再按 F 没反应 ────────────────── F 被 GA 消费，BP 事件收不到
    ├── 11. 迁移 Map 后武器不显示 ──────────────── TMap 值类型变更导致键名丢失
    └── 12. 换武器偏移量失效 ───────────────────── 模型枢轴在剑尖，偏移=剑长
```

---

## 二、每个 Bug 的完整技术细节

### Bug 1：UHT 报 `BlueprintReadOnly should not be used on private members`

**现象**：编译直接失败，UHT 报 6 处 `PP_PlayerCharacter.h` 的 `UPROPERTY` 违规。

**根因**：`UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)` 写在 `private:` 区段，UHT 不允许蓝图访问私有成员。

**修复**：每个属性补 `meta = (AllowPrivateAccess = "true")`。

---

### Bug 2：`C3861: "HasAuthority": 找不到标识符`

**现象**：`PP_InventoryComponent.cpp` 编译失败。

**根因**：`UPP_InventoryComponent` 继承 `UActorComponent`，没有 `HasAuthority()`（那是 `AActor` 的方法）。

**修复**：改为 `GetOwner() && GetOwner()->HasAuthority()`。

---

### Bug 3：按 F 完全没反应（断言中断）

**现象**：PIE 按 F，输出日志出现断点断言，外观代码一行都没执行。

**根因**：`UPP_InventoryComponent` 里直接调用了 `EquipmentUser->OnEquipmentChanged(...)`。`IPP_EquipmentUser::OnEquipmentChanged` 是 `BlueprintNativeEvent`，直接调用会触发：
`Do not directly call Event functions in Interfaces. Call Execute_OnEquipmentChanged instead.`

**修复**：装备/卸下两处都改为 `IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, SlotType, ItemID)`。

---

### Bug 4：自带武器变灰但几何还在

**现象**：换材质后武器没消失，而是变成引擎默认灰格。

**根因**：`M_HiddenWeapon` 没勾 `Used with Skeletal Mesh`，骨骼网格不受支持时回退到 `WorldGridMaterial`。

**修复**（资产层）：Blend Mode = `Masked`，Opacity Mask 常量 `0`，Usage Flags 勾 `Used with Skeletal Mesh`。

---

### Bug 5：材质槽索引取不到材质（`slot=17 原材质=None`）

**现象**：隐藏无效，还原后也是灰材质。

**根因**：`BeginPlay` 时角色的骨骼网格还没被外观逻辑赋值，`GetNumMaterials()` 返回 0，缓存里全是 `nullptr`。

**修复**：
- 材质槽识别改为惰性：`SetBuiltInWeaponVisible` 里发现槽位列表为空时调用 `PP_AutoDetectWeaponMaterialSlots` 兜底
- 还原材质优先从网格资产 `USkeletalMesh::GetMaterials()` 取，不依赖运行时缓存

---

### Bug 6：服务器生效，客户端窗口看不到

**现象**：日志显示 `slot=17 应用=M_HiddenWeapon`、`SetWeaponMesh=SM_Longsword` 全部成功，但 PIE 客户端视口毫无变化。

**根因链**：

```
PIE Listen Server = 同一进程内 server world + client world，同一 Pawn 有两份
    → Server_EquipWeapon 是 Server RPC，只在服务器执行
    → OnEquipmentChanged 只有服务器那份 Pawn 收到
    → SetMaterial / SetStaticMesh 都不参与网络复制
    → 客户端 Pawn 完全没变
```

**修复**：
- `UPP_InventoryComponent::OnRep_WeaponSlots` 里按 `SlotType` 比对 `OldSlots` 与当前槽位，对变化的槽位回调 `Execute_OnEquipmentChanged`，让客户端也走一遍外观逻辑
- `APP_PlayerCharacter::OnEquipmentChanged_Implementation` 的 GA 段前加 `if (!HasAuthority()) return;`，避免客户端重复 `GiveAbility`

---

### Bug 7：长剑出现在身体中间

**现象**：长剑垂直插在角色原点，不跟手也不随动画。

**根因**：BP 里配的挂点名 `VB_Kwang_weapon` 在 `Kwang_Skeleton` / `KwangSunrise` 里**都不存在**。`AttachToComponent` 遇到不存在的 socket 不报错，只是把组件留在角色原点、相对变换全 0。

**修复**：`PP_AttachWeaponMeshToSocket` 增加回退链，逐个检测候选挂点。

---

### Bug 8：长剑停在脚边，只随脚步轻微移动

**现象**：回退到 `ik_hand_gun` 后，剑在脚边，位移幅度很小。

**根因**：`ik_hand_gun` 是 Paragon 的 **IK 骨骼**，由原版动画蓝图驱动。项目用 ALS 的 AnimBP，没人写它的 `ComponentSpaceTransform`，所以它永远停在 bind pose（脚边），只跟随根骨骼/骨盆起伏。

**修复**：候选链改成优先真实手骨。注意 `hand_r` 在 Kwang 骨架里是**骨骼不是 socket**，而 `USceneComponent::DoesSocketExist` 只认 socket，会把骨骼判为不存在 —— 判定必须写成：

```cpp
const bool bExists = MeshComp->DoesSocketExist(Candidate)
    || MeshComp->GetBoneIndex(Candidate) != INDEX_NONE;
```

---

### Bug 9：改代码后运行行为完全不变

**现象**：源码里候选数组顺序已改成 `hand_r` 第一，运行时日志里 `ik_hand_gun` 仍是第一。

**根因**：数组写成了函数内 `static const FName FallbackSockets[]`。Live Coding 只替换函数体、**不重新执行静态初始化**，所以跑的还是旧数据。类成员同理。

**修复**：改用普通局部数组。规则：**Live Coding 环境下的调试数据一律不要用 `static`**。

---

### Bug 10：装备成功后再按 F 没反应

**现象**：第一次按 F 装备正常，之后无论按 F 还是其它键都无法卸下，长剑一直握在手里。

**根因**：`F` 在 `PP_PlayerController` 里绑定的是 `PrimaryAction` 攻击技能。装备后 `GA_PP_Primary` 被授予并抢占输入，BP 里那个 `F → Server Equip Weapon` 的事件再也收不到输入。日志里从此没有任何 `Server_EquipWeapon` / `Server_UnequipWeapon`。

**修复**：新增 `F9` 调试键（不占用技能键）：已装备则卸下，未装备则自动补一件再装上。

**顺带发现**：日志 `匹配能力数=2`，说明 `GA_PP_Primary` 有两个实例 —— `StartupAbilities` 里原本就有一个，装备武器时又 `GiveAbility` 一次，攻击会双触发。后续需把默认攻击 GA 从 `StartupAbilities` 移除。

---

### Bug 11：Map 值类型变更后武器不显示

**现象**：`WeaponMeshMap` 从 `TMap<FName, UStaticMesh>` 改成 `TMap<FName, FPP_WeaponMeshVisual>` 后，BP 里条目还在、值也迁移过来了，但运行时日志报
`WeaponMeshMap 未找到 ItemID=Longsword（Map 当前 1 条）`。

**根因**：UE 反序列化时只把值迁进了新结构体，**Map 的键名丢失**，变成 `None`。

**修复**（BP 层）：把该条目的键手动改回 `Longsword`，然后 Compile + 保存 BP。

**经验**：改 `TMap` 的值类型会导致 BP 里键名丢失，迁移前先记录键名；排查时加一条打印所有现存键的日志最省时间。

---

### Bug 12：换一把不同长度的武器，偏移量就不对了

**现象**：`Location.Z = 120` 能对齐长剑，换成别的长度武器就穿模或悬空。

**根因**：`SM_Longsword` 的**枢轴在剑尖**，而挂载是「枢轴对齐骨骼原点」。旋转是绕枢轴转的，剑尖被钉在手腕上，剑柄只能绕剑尖画圈，永远到不了手心 —— 所以单靠旋转调不出来。`Z=120` 实际是「剑尖到握把的距离」，即剑长，长度一变自然失效。

**修复（方案 B）**：`WeaponMeshMap` 的值类型改为 `FPP_WeaponMeshVisual`，每把武器带自己的 `Mesh / RelativeLocation / RelativeRotation / RelativeScale`，`SetWeaponMesh` 按 ItemID 取配置应用。新增武器各自配一套，互不影响。

**备选方案**：在资产里把枢轴移到握把（治本，但每把素材都要改）；或在骨架里为每把武器建专用 socket（Epic 的标准做法）。

---

## 三、其他踩坑记录

### 编译

- Live Coding 激活时命令行编译一律被拒：`Unable to build while Live Coding is active`
- 只改 `.cpp` 函数体 → 编辑器内 `Ctrl+Alt+F11`
- 新增/修改 `UFUNCTION` / `UPROPERTY` / `USTRUCT` → 必须关闭编辑器完整编译

### 局部变量遮蔽

`APP_PlayerCharacter` 里写 `APP_PlayerState* PlayerState = Cast<...>(GetPlayerState())` 会报
`C4458: "PlayerState" 的声明隐藏了类成员`（`APawn::PlayerState`），改名 `PPPlayerState`。

### PIE Listen Server

同一进程内存在 server world + client world，同一 Pawn 有两份 `BeginPlay`，日志会出现两遍。排查「服务器改了但看不见」时先确认自己看的是哪个世界的那份 Pawn。

---

## 四、最终验证流程

1. 出生：握自带刀（`Hide Built in Weapon on Begin Play` 未勾选）
2. 按 `F9`：自带刀消失、长剑出现在右手
3. 再按 `F9`：长剑消失、自带刀恢复
4. 重复 2-3 可反复切换，背包数量正确增减
