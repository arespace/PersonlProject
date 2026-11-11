# C++ 基础巩固：从语言基础到 UE 代码规范

> 状态：现行 · 最后更新：2026-11-11
> 定位：面向 C++ 面试的**语言基础题**，同时把每条语法落到本项目的 UE C++ 写法上。
> 与 [学习路线.md](学习路线.md) 的分工：那份 30 题（A8 / B14 / C8）考「项目与引擎事实」，这份考「语言本身」；重合处只引用题号，不重复出题。
> 用法：一次一个单元，30–60 分钟。先读「必须掌握」，再自问「面试问法」，最后看「项目对照」。
> 边界：不涉及模板元编程、完美转发与移动语义深挖、并发与内存模型、C++20 协程与 Concepts、allocator、SIMD、编译期计算、渲染/网络底层。清单见 [§17](#17-以后再说现在不学)。

---

## 0. 怎么推进

1. **按单元走，不按章通读**。每单元控制在 30–60 分钟：读 15 分钟、口述 15 分钟、对照项目代码 15 分钟。
2. **先自答再看答案**。自测题在 [§15](#15-自测题36-题先答后看)，答案在 [§16](#16-答案与评分标准)。在会话里逐题作答也可以，由我批改。
3. **每单元只留一句"能说出口"的话**。说不出来就是没学会，回到「必须掌握」重读，不要往下翻。
4. 单元 1–10 是纯语言；单元 11–12 是把语言规则映射到 UE。**先语言后 UE**，顺序不要反。

| 节奏 | 安排 |
| --- | --- |
| 每天 1 个单元 | 12 天走完一遍，适合面试前两周 |
| 每周 3 个单元 | 4 周走完，适合长期巩固 |
| 只补薄弱项 | 按 [§1](#1-单元清单) 的「一句话自检」挑单元，不按顺序 |

---

## 1. 单元清单

| # | 单元 | 主题 | 建议时长 | 自测题 | 与 30 题的关系 |
| --- | --- | --- | --- | --- | --- |
| 1 | 值、类型与初始化 | 内置类型、四种初始化、作用域、整数提升 | 40 min | Q1–Q3 | 无 |
| 2 | 指针、引用与 const | 指针 vs 引用、const 三种位置、const 成员函数 | 45 min | Q4–Q6 | 关联 A6（接口指针） |
| 3 | 内存与生命周期 | 栈/堆、new/delete、RAII、智能指针 | 50 min | Q7–Q9 | 关联 B10（软引用懒加载） |
| 4 | 构造、析构、拷贝与移动 | 初始化列表、三/五/零法则、虚析构 | 50 min | Q10–Q12 | 无 |
| 5 | 继承、多态与虚函数 | vtable/vptr、override、纯虚、切片 | 50 min | Q13–Q15 | 关联 A6、B4 |
| 6 | 类型转换 | 四种 cast、RTTI、UE 的 `Cast` | 35 min | Q16–Q18 | 无 |
| 7 | STL 容器与算法 | 容器选择、复杂度、迭代器失效、lambda | 50 min | Q19–Q21 | 关联 A7（EventBus 快照） |
| 8 | 字符串与文本 | std::string、编码、FString/FName/FText | 35 min | Q22–Q24 | 无 |
| 9 | 异常、断言与常见陷阱 | 异常安全、noexcept、check/ensure/verify、UB | 40 min | Q25–Q27 | 无 |
| 10 | 编译与链接基础 | 声明/定义、ODR、前向声明、链接错误 | 40 min | Q28–Q30 | 关联 A6（模块依赖） |
| 11 | UObject 与 GC 基础 | 为什么不能 new、UPROPERTY 保引用、TObjectPtr/TWeakObjectPtr | 50 min | Q31–Q33 | 关联 A8、B10 |
| 12 | UPROPERTY/UFUNCTION 与 UE 约定 | 反射宏、网络说明符、UE 容器、命名与所有权 | 50 min | Q34–Q36 | 关联 B6、B7、B4 |

> 每个单元的「项目对照」都指向本仓库真实文件与行号，可直接跳读；行号会随代码改动漂移，以函数名为准。

---

## 2. 单元 1：值、类型与初始化

### 必须掌握

1. **定长类型**：UE 的 `int32` / `uint8` / `int64` 是定长别名；标准 `int` 只保证至少 16 位。跨平台存档、网络协议、DataTable 一律用定长类型。
2. **四种初始化**：
   - 默认初始化 `int a;`（函数内内置类型**不初始化**，全局/静态才零初始化）
   - 拷贝初始化 `int a = 3;`
   - 直接初始化 `int a(3);`
   - 列表初始化 `int a{3};`（禁止窄化，现代 C++ 首选）
3. **类内初始化**：`int32 Count = 0;` / `float Radius{100.f};` 让成员永远有确定初值。
4. **作用域与存储期**：块作用域（`{}`）、文件作用域、命名空间；`static` 局部变量在**首次执行到声明时**初始化，且只初始化一次（C++11 起线程安全）。
5. **整数提升与混合运算**：`char` / `short` / `bool` 参与算术先提升为 `int`；有符号与无符号混算，有符号一侧被转成无符号。
6. **溢出**：无符号溢出按 2^N 回绕（有定义）；有符号溢出是 **UB**，不能依赖。
7. **除法与取模**：整数除法向零截断（`-7 / 2 == -3`）；`%` 的结果符号跟随被除数。
8. **`auto`**：默认丢弃顶层 const 和引用；需要只读视图写 `const auto&`。

### 常见面试问法

- 函数内的 `int a;` 和全局的 `int a;` 有什么区别？
- `int` 一定是 4 字节吗？为什么 UE 代码里到处是 `int32`？
- `-1 < 1u` 的结果是什么？为什么？
- `int x = 3.9;` 和 `int x{3.9};` 有什么区别？
- `static` 修饰局部变量是什么意思？（全局/函数的含义见单元 10）
- `i++` 和 `++i` 的区别是什么？

### 易错处

- 函数内局部内置类型未初始化，读到的值不确定（调试版可能"看着正常"）。
- `{}` 初始化发生窄化会**编译报错**，`=` 初始化会静默截断——用 `{}` 能把错误提前。
- `for (unsigned i = n - 1; i >= 0; --i)` 在 `n == 0` 时下溢成巨大值，死循环。
- 无符号减法 `a - b` 在 `b > a` 时回绕，比较结果反直觉。
- 把枚举当整数随便算：`enum class` 不隐式转 `int`，这是有意的保护。

### 项目对照

- [PP_ItemTypes.h](../../Source/PersonalProjectCore/Public/Data/PP_ItemTypes.h) 的 `enum class EPP_ItemType : uint8`（L17–25）：`enum class` 是强类型枚举，不隐式转整数、不会和别的枚举混用；指定 `uint8` 固定大小，能直接进 `UPROPERTY` 和 DataTable。
- [PP_ItemData.h](../../Source/PersonalProjectCore/Public/Data/PP_ItemData.h) 的 `int32 MaxStackCount = 99;`（L47）：类内初始化，避免"忘记赋值"。
- 同文件 `FPP_ItemInstance() : Count(0) {}`（L96）：构造函数兜底，保证 `Count` 永远有值。
- [PP_BaseCharacter.h](../../Source/PersonalProjectCore/Public/Characters/PP_BaseCharacter.h) 的 `float RespawnDelay{3.f};`（L105）与 `bool bAlive = true;`（L133）：两种类内初始化写法在项目里都在用，语义一致。

### 一句话自检

能说清「函数内 `int x;` 读出来是什么」和「`-1 < 1u` 为什么为真」。

---

## 3. 单元 2：指针、引用与 const

### 必须掌握

1. **指针**：存地址，可为空、可改指向、可做算术（数组上下文）；解引用空指针或悬垂指针是 UB。
2. **引用**：对象的别名，必须初始化、不可重新绑定、没有"空引用"；但引用可以是**悬垂**的。
3. **const 的三种位置**：
   - `const T* p` / `T const* p`：指向常量，不能通过 `p` 改对象
   - `T* const p`：常量指针，不能改指向，但能改对象
   - `const T* const p`：两者都不能改
4. **const 成员函数**：承诺不修改成员（`mutable` 成员例外），可被 const 对象调用；const 与非 const 版本可以重载。
5. **传参约定**：大对象用 `const T&`（只读、不拷贝）；需要修改用 `T&`；"可能为空/可重绑定"用指针。
6. **返回约定**：返回值安全（拷贝/移动/省略）；返回引用或指针必须保证对象活得比调用方久。
7. **顶层 / 底层 const**：`const int*` 是底层 const（指向的对象 const）；`int* const` 是顶层 const（指针本身 const）。拷贝时顶层 const 被忽略。

### 常见面试问法

- 指针和引用的区别？什么时候必须用指针？
- `const int* p`、`int const* p`、`int* const p` 分别限制了什么？
- const 成员函数为什么能参与重载？`mutable` 用在哪？
- 为什么大对象传参用 `const&`？按值传会发生什么？
- 函数返回引用什么时候是危险的？
- 引用占不占内存？（实现层面通常用指针实现，标准不保证）

### 易错处

- 返回局部变量或临时对象的引用/指针，调用方拿到即悬垂。
- 非 const 引用不能绑定临时对象（`void F(int&)` 传字面量编译不过），要改成 `const int&`。
- 在 const 成员函数里改成员会编译失败；确实要改（缓存、计数）才用 `mutable`。
- 用 `if (p)` 判空，但 `p` 是未初始化/已释放的野指针——判空不等于有效。
- 用 `const_cast` 去掉 const 再写**真正 const 的对象**，是 UB。

### 项目对照

- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 的 `const FPP_ItemInfo* FindItemInfo(FName ItemID) const`（L45–58）：返回**只读指针**，调用方拿不到写权限；末尾 `const` 表示查询不修改组件状态。
- [PP_InventoryComponent.h](../../Source/PersonalProjectInventory/Public/Inventory/PP_InventoryComponent.h) 的 `TArray<FPP_ItemInstance> GetItems() const { return Items; }`（L100–101）：返回**副本**而不是 `const TArray&`，避免 UI 持有内部数组后因后续增删而悬空；代价是一次拷贝，这里是刻意取舍。
- 同文件 `FActiveGameplayEffectHandle ApplyEquipEffect(const FPP_ItemInfo& ItemInfo);`（L213）：大结构按 const 引用传，避免拷贝，也表明函数内不修改配置。

### 一句话自检

能在 30 秒内说清 `const T* const p` 哪部分不可改，并解释项目里 `GetItems()` 为什么返回副本。

---

## 4. 单元 3：内存与生命周期

### 必须掌握

1. **存储期**：自动（栈，离开作用域销毁）、静态（全局/`static`，程序期间存在）、动态（堆，手动或智能指针管理）、线程（`thread_local`）。
2. **栈 vs 堆**：栈分配几乎无成本、大小有限、连续；堆分配慢、可大块、需要显式释放。
3. **new / delete**：`new` = 分配内存 + 调用构造；`delete` = 调用析构 + 释放内存。`new[]` 必须配 `delete[]`。
4. **malloc / free**：只分配/释放原始内存，不调用构造/析构，也不做类型检查。
5. **三类内存错误**：泄漏（忘释放）、悬垂（释放后继续用）、重复释放（double free，UB）。
6. **RAII**：资源（内存、文件、锁、句柄）绑定到对象生命周期，析构自动释放；这是 C++ 管理资源的核心思想。
7. **智能指针**：
   - `unique_ptr`：独占所有权，不可拷贝、可移动，`make_unique`
   - `shared_ptr`：引用计数共享所有权，控制块存计数，`make_shared`
   - `weak_ptr`：不增加计数，`lock()` 提升为 `shared_ptr`，用于打破循环引用
8. **线程安全边界**：`shared_ptr` 的引用计数是原子的，但**指向的对象**不是线程安全的。

### 常见面试问法

- `new/delete` 和 `malloc/free` 的区别？
- 什么是内存泄漏、悬垂指针、double free？
- `unique_ptr` 怎么保证独占？`shared_ptr` 的计数存在哪？
- `shared_ptr` 循环引用会怎样？`weak_ptr` 为什么能解决？
- `shared_ptr` 是线程安全的吗？
- 什么是 RAII？举一个项目里的例子。

### 易错处

- `new[]` 用 `delete` 释放（UB），智能指针管理数组要用 `unique_ptr<T[]>`。
- 用同一个裸指针构造两个 `shared_ptr`，产生两个控制块 → 重复释放。
- 长期持有 `shared_ptr::get()` 返回的裸指针，计数归零后悬垂。
- 先 `new` 再传给函数，函数抛异常 → 泄漏（用 `make_unique` 可避免）。
- 把 `weak_ptr` 当普通指针直接解引用（必须先 `lock()` 并判空）。

### 项目对照

- [PP_WidgetPool.h](../../Source/PersonalProjectUI/Public/Framework/PP_WidgetPool.h) 的 `UPROPERTY() TArray<TObjectPtr<UUserWidget>> Available;`（L39–44）与 `TWeakObjectPtr<UUserWidget> OwnerWidget;`（L50）：池**强引用**可用 Widget（池负责它们的生命周期），对拥有者用**弱引用**，避免池反向拖住拥有者造成泄漏。
- [PP_PlayerCharacter.h](../../Source/PersonalProject/Public/Characters/PP_PlayerCharacter.h) 的 `TWeakObjectPtr<APP_SoulPickupActor> ActiveSoul;`（L189–190）：掉落的魂可能被销毁或拾取，弱引用天然处理"对象没了"的情况。
- UE 的 UObject **不用** `new/delete`，也不该用 `shared_ptr` 管理，统一由 GC 负责——细节见单元 11。

### 一句话自检

能说清 `new` 比 `malloc` 多做了什么，以及为什么池对拥有者要用弱引用。

---

## 5. 单元 4：构造、析构、拷贝与移动

### 必须掌握

1. **构造顺序**：基类 → 成员（**按声明顺序**，与初始化列表书写顺序无关）→ 函数体。
2. **初始化列表 vs 函数体赋值**：`const` 成员、引用成员、没有默认构造的成员**必须**用初始化列表；列表是"初始化"，函数体里是"赋值"，语义和效率都不同。
3. **析构顺序**：与构造相反；多态基类的析构**必须** `virtual`，否则通过基类指针 `delete` 是 UB（派生部分不析构）。
4. **拷贝构造 / 拷贝赋值**：编译器默认生成逐成员拷贝（浅拷贝）；持有资源（指针、句柄）时必须深拷贝或禁用。
5. **三法则**：需要自定义析构、拷贝构造、拷贝赋值中的任何一个，通常三个都要。**五法则**再加移动构造、移动赋值。**零法则**优先：用能自动管理资源的成员，不手写。
6. **`= default` / `= delete`**：显式要默认行为，或显式禁止拷贝。
7. **移动语义基础**：`std::move` 只是把左值**转成右值引用**，本身不搬任何东西；真正的"搬"发生在移动构造/移动赋值里。被移动后的对象处于"有效但未指定"状态，只能销毁或重新赋值。
8. **RVO/NRVO**：返回局部对象通常直接在调用方构造，不发生拷贝；不要为了"优化"写 `return std::move(x)`（反而阻止 RVO）。

### 常见面试问法

- 为什么基类析构函数要声明为 virtual？
- 什么时候必须使用成员初始化列表？
- 编译器什么时候会自动生成拷贝构造函数？
- 浅拷贝会带来什么问题？怎么解决？
- 三法则 / 五法则 / 零法则分别是什么？
- `std::move` 到底做了什么？移动后原对象还能用吗？

### 易错处

- 初始化列表顺序写反：实际顺序由**声明顺序**决定，可能用到还没初始化的成员。
- 拷贝赋值没处理自赋值（`a = a`），先释放再拷贝会丢掉数据。
- 有裸指针成员却用默认拷贝 → 两个对象析构同一块内存。
- 移动后继续读源对象的值（状态未指定）。
- 对 `const` 对象 `std::move`，实际退化为拷贝（无法绑定到非 const 右值引用）。

### 项目对照

- [PP_ItemData.h](../../Source/PersonalProjectCore/Public/Data/PP_ItemData.h) 的 `FPP_ItemInstance(FName InItemID, int32 InCount) : ItemID(InItemID), Count(InCount) {}`（L97）：值类型、可拷贝，所以能直接放进 `TArray`。
- 同文件 `FPP_EquippedItem() : Slot(EPP_EquipSlot::None) {}`（L122）：保证枚举成员有确定初值，避免"未初始化枚举"。
- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 的 `FPP_EquippedItem NewEquip; ... EquippedItems.Add(NewEquip);`（L295–299）：`Add` 是**拷贝一份**进数组，之后改 `NewEquip` 不会影响已存元素——这就是值语义。

### 一句话自检

能解释「基类析构不 virtual 时 `delete basePtr` 会发生什么」，以及 `std::move` 不是"搬内存"。

---

## 6. 单元 5：继承、多态与虚函数

### 必须掌握

1. **public 继承表达 is-a**；能组合就不要继承。私有/保护继承少见，面试只需知道访问权限被收紧。
2. **动态绑定条件**：通过**指针或引用**调用**虚函数**。对象直接调用、按值传递都不触发多态。
3. **vtable / vptr**：每个多态类有一张虚函数表（编译期生成），每个对象里有一个指向它的 vptr（构造时设置）；虚调用 = 通过 vptr 找到函数地址再调用。
4. **`override`**：签名不匹配时直接编译报错，防止"以为覆盖了其实是新函数"。**`final`** 禁止继续覆盖或继承。
5. **纯虚 `= 0`**：抽象类不能实例化；接口类通常全是纯虚函数 + 虚析构。
6. **构造/析构期间**：虚调用只派发到**当前正在构造/析构的类**，不会到派生类。
7. **对象切片**：派生类对象按值赋给基类对象，只拷贝基类部分，虚函数行为也退化为基类。
8. **多重继承**：接口多继承常见（一个实现 + 若干纯虚接口）；菱形继承用虚继承解决二义性（了解即可）。

### 常见面试问法

- 多态是怎么实现的？虚函数表在哪、什么时候建立？
- 构造函数里调用虚函数会调用到哪个版本？
- 重载（overload）、重写（override）、隐藏（hide）的区别？
- 为什么基类析构要 virtual？不写会怎样？
- 抽象类和接口有什么区别？
- 对象切片是什么？

### 易错处

- 忘记 `virtual`，通过基类指针调用时静态绑定到基类版本。
- 签名差一个 `const` 或参数类型，又没写 `override`，变成"隐藏"而不是"重写"。
- 用基类**值**接收派生对象，发生切片。
- 在构造/析构里调用虚函数，期待子类行为（不会发生）。
- 多继承下用 `static_cast` 在接口指针之间乱转（应使用 `Cast` 或明确转换路径）。

### 项目对照

- [PP_BaseCharacter.h](../../Source/PersonalProjectCore/Public/Characters/PP_BaseCharacter.h) 的 `class APP_BaseCharacter : public AAlsCharacter, public IAbilitySystemInterface`（L22–23）：一个实现继承（复用 ALS 角色能力）+ 一个接口继承（对外暴露 ASC 查询）。
- 同文件 `virtual bool CanBeDamaged() const;`（L40）：调用方按基类指针调用，派生类可改伤害规则。
- [PP_InteractableInterface.h](../../Source/PersonalProjectCore/Public/Interfaces/PP_InteractableInterface.h) 的 `virtual void Interact(APlayerController* Interactor) = 0;`（L28）：纯虚接口，`Cast` 到接口后统一调用，不关心具体 Actor 类型。
- [PP_PlayerCharacter.h](../../Source/PersonalProject/Public/Characters/PP_PlayerCharacter.h) 的 `virtual UAnimMontage* SelectRollMontage_Implementation() override;`（L173）：`override` 保证确实覆盖了 ALS 的虚函数，签名写错会立刻报错。

### 一句话自检

能画出「对象 → vptr → vtable → 函数地址」四步，并解释构造期间虚调用的落点。

---

## 7. 单元 6：类型转换

### 必须掌握

1. **隐式转换**：数值提升/转换、派生类指针 → 基类指针（安全）、非 `explicit` 单参构造。`explicit` 用来阻止意外的隐式构造。
2. **`static_cast`**：相关类型之间的编译期转换（数值、`void*`、上行转换）。下行转换不做运行时检查，转错就是 UB。
3. **`dynamic_cast`**：多态类型的安全下行/交叉转换；指针失败返回 `nullptr`，引用失败抛 `std::bad_cast`；依赖 RTTI，有运行时开销。
4. **`const_cast`**：只增删 `const` / `volatile`；修改**真正 const** 的对象是 UB。
5. **`reinterpret_cast`**：按位重新解释，最危险；只在底层内存布局确定时使用。
6. **C 风格转换** `(T)x`：会依次尝试 `const_cast` / `static_cast` / `reinterpret_cast`，看不出意图，新代码禁止。
7. **UE 的 `Cast<T>`**：不是 C++ `dynamic_cast`，而是基于 `UClass` 继承链判断，失败返回 `nullptr`，开销更小；调用前仍需判空。

### 常见面试问法

- 四种 cast 的区别和使用场景？
- `dynamic_cast` 失败怎么表现？为什么要求类型是多态的？
- `const_cast` 一般用在哪？去掉 const 后写入会怎样？
- `static_cast` 做向下转换安全吗？
- C 风格转换有什么问题？
- UE 的 `Cast<T>` 和 `dynamic_cast` 是一回事吗？

### 易错处

- 用 `static_cast` 做不安全的向下转换，类型不对时拿到无效对象。
- `const_cast` 掉真正 const 对象的 const 再写入（UB）。
- `reinterpret_cast` 后的指针未对齐或生命周期已结束。
- 忘记 `explicit`，导致隐式构造在函数调用处悄悄发生。
- 把 UE 的 `Cast` 与 `dynamic_cast` 混为一谈（前者走 `UClass` 链，后者走 RTTI）。

### 项目对照

- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 的 `static_cast<int32>(Slot)`（L363）：`enum class` 不隐式转整数，日志里必须显式转。
- [PP_ShopComponent.cpp](../../Source/PersonalProjectInventory/Private/Shop/PP_ShopComponent.cpp) 的 `reinterpret_cast<const FPP_ShopItem*>(Pair.Value)` 与 `const_cast<FPP_ShopItem*>(Row)`（L52、L56）：DataTable 行指针的既有写法——**知道布局才敢用**，且返回的指针生命周期不能超过表本身。
- [PP_AlsRetargetTool.cpp](../../Source/PersonalProject/Private/Editor/PP_AlsRetargetTool.cpp) 的 `Cast<UBlueprint>(Asset)`（L269）：UE 类型系统转换，失败返回 `nullptr`，后面必须判空再使用。

### 一句话自检

能一句话说清四种 cast 的边界，并指出项目里 `const_cast` 那处的风险。

---

## 8. 单元 7：STL 容器与算法

### 必须掌握

1. **容器选择**：
   - `vector`：连续内存、随机访问 O(1)、尾部增删均摊 O(1)，默认首选
   - `deque`：双端队列，头尾插入都快，内存分块
   - `list`：双向链表，任意位置插入/删除 O(1)，但缓存不友好、不支持随机访问
   - `map` / `set`：红黑树，有序，查找 O(log n)
   - `unordered_map` / `unordered_set`：哈希表，平均 O(1)，无序，需要哈希函数
2. **扩容**：`vector` 满时按倍数（常见 1.5 或 2）重新分配并搬移元素，均摊 O(1)；`reserve` 可提前避免反复扩容。
3. **迭代器失效**：
   - `vector` 扩容 → 所有指针、引用、迭代器失效
   - `vector::erase` → 被删元素及其后全部失效；返回下一个有效迭代器
   - `map/set::erase` → 只失效被删元素
4. **erase-remove 惯用法**：`v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());`
5. **范围 for 中不要改容器结构**：增删元素会让底层迭代器失效。
6. **lambda**：`[=]` 按值捕获（副本默认 const，`mutable` 可改）、`[&]` 按引用捕获（生命周期风险）、`[x, &y]` 混合捕获。
7. **常用算法**：`find` / `find_if` / `sort` / `stable_sort` / `accumulate` / `any_of` / `transform`；谓词优先用 lambda。
8. **`map::operator[]` vs `find`**：`operator[]` 在 key 不存在时会**插入默认值**，只读查询用 `find` 或 `at`。

### 常见面试问法

- `vector` 和 `list` 怎么选？`map` 和 `unordered_map` 怎么选？
- `vector` 扩容后原来的指针还能用吗？
- 遍历容器时删除元素怎么写才对？
- 迭代器失效有哪几种情况？
- lambda 捕获引用有什么风险？
- `map[key]` 和 `map.find(key)` 的区别？

### 易错处

- 范围 for 里 `erase`，迭代器失效后继续自增。
- 保存了 `vector` 元素的指针/引用，之后 `push_back` 触发扩容 → 悬垂。
- `map[key]` 做只读查询，意外插入了默认值并改变了容器。
- lambda `[&]` 捕获局部变量，回调在函数返回后才执行 → 悬垂引用。
- `container.size() - 1` 在空容器上回绕成巨大无符号数。

### 项目对照

- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 的 `AddItem` 范围 for（L87–98）：只修改元素值、不增删容器，安全。
- 同文件 `RemoveItem` 倒序遍历 + `RemoveAt(i)`（L141–153）：从后往前删，避免删除后下标错位；这是"边遍历边删"最稳的写法之一。
- 同文件 `EquippedItems.IndexOfByPredicate([Slot](const FPP_EquippedItem& E) { return E.Slot == Slot; })`（L338–341）：谓词查找返回下标，`INDEX_NONE` 表示没找到。
- [PP_EventBus.h](../../Source/PersonalProjectCore/Public/Events/PP_EventBus.h) 的 `Broadcast`（L104–118）：先把回调**拷贝成快照**再遍历，避免回调里退订导致正在遍历的 `TMap` 被修改。这和 STL 的迭代器失效是同一类问题。

### 一句话自检

能说出「遍历中删除」的两种正确写法，以及 `vector` 扩容为什么会让旧指针失效。

---

## 9. 单元 8：字符串与文本

### 必须掌握

1. **`std::string`**：字节序列（通常 UTF-8），`c_str()` 保证 `\0` 结尾，`size()` / `length()`、`substr` / `find` / `npos`。
2. **编码基础**：`char` 通常是 UTF-8 字节；`wchar_t` 宽度平台相关；UE 的 `TCHAR` 在 Windows 上是 `wchar_t`（UTF-16）。
3. **UE 三种字符串**：
   - `FString`：可变、可拼接、UTF-16，用于构造/修改/日志
   - `FName`：不可变、大小写不敏感、全局池化，比较几乎 O(1)，适合 ID、资源名、行名
   - `FText`：本地化文本，面向玩家显示，不用于逻辑比较
4. **`TEXT("...")`**：生成 `TCHAR` 字面量；`*FString` 得到 `const TCHAR*`。
5. **常用转换**：`FString::FromInt`、`FString::Printf`、`FString::SanitizeFloat`、`FCString::Atoi`、`FName(*Str)`、`Name.ToString()`。
6. **比较**：`FName` 直接用 `==`（忽略大小写）；`FString` 用 `Equals`（可指定是否忽略大小写）；`FText` 不直接比较。
7. **日志格式**：`UE_LOG` 的 `%s` 要的是 `TCHAR*`，传 `FString` 必须加 `*`。

### 常见面试问法

- `FString` / `FName` / `FText` 的区别和使用场景？
- 为什么 `UE_LOG` 的 `%s` 要写 `*ItemID.ToString()`？
- `std::string` 和 `FString` 怎么互转？
- UTF-8 和 UTF-16 的区别（基础层面）？
- 为什么不用 `FString` 当 Map 的 key？
- `FName` 为什么比较快？

### 易错处

- 把 `FName` 当可显示文本（不可本地化，且显示的是内部索引形式）。
- `%s` 直接传 `FString` 忘了解引用 `*`，输出乱码或崩溃。
- 在循环里频繁拼接 `FString`，每次都可能重新分配。
- 循环里反复用字符串构造 `FName`（每次都要查/插全局表）。
- 用 `FString` 做 ID 比较，既慢又受大小写/本地化影响。

### 项目对照

- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 的 `UE_LOG(LogPP, Warning, TEXT("[Inventory] AddItem: 未找到物品配置 %s"), *ItemID.ToString())`（L76）：`FName → FString → const TCHAR*` 的完整链路。
- [PP_ItemData.h](../../Source/PersonalProjectCore/Public/Data/PP_ItemData.h) 的 `FName ItemID`（L27）与 `FText DisplayName` / `FText Description`（L31、L35）：**标识用 FName，展示用 FText**，正好对应两种语义。
- [PP_InventoryComponent.cpp](../../Source/PersonalProjectInventory/Private/Inventory/PP_InventoryComponent.cpp) 用 `NAME_None` 表示"空槽/无物品"（L284、L333）：这是 FName 的空值约定，不要用空字符串代替。

### 一句话自检

能说清「什么时候用 FName、什么时候用 FText」，并解释日志里那个 `*` 的作用。

---

## 10. 单元 9：异常、断言与常见陷阱

### 必须掌握

1. **异常基础**：`throw` 抛出对象，`catch` 按类型匹配，栈展开时自动调用已构造对象的析构（RAII 才可靠）。
2. **异常安全三档**：
   - 基本保证：不泄漏、对象仍有效
   - 强保证：失败时回滚到操作前状态（事务语义）
   - 不抛保证：`noexcept`，失败直接终止
3. **析构函数默认 `noexcept`**：析构里抛异常会触发 `std::terminate`。
4. **`noexcept` 的实际影响**：`vector` 扩容时优先使用 `noexcept` 的移动构造，否则退化为拷贝。
5. **UE 默认关闭 C++ 异常**（`bEnableExceptions = false`）：错误处理用返回 `bool`、输出参数、日志、断言，而不是 try/catch。
6. **断言三件套**：
   - `check(expr)`：开发期致命错误，Shipping 中被编译掉
   - `ensure(expr)`：失败时记录一次调用栈并继续执行（非致命）；由 `DO_GUARD_SLOW` 控制，Shipping 中通常也被编译掉
   - `verify(expr)`：表达式在 Shipping 中仍会求值（用于"即使不检查也必须执行"的场景）
7. **常见 UB 清单**：悬垂指针/引用、数组越界、未初始化读取、有符号溢出、除零、返回局部引用、对象切片、重复释放、迭代器失效后使用。

### 常见面试问法

- 什么是异常安全？三个等级分别是什么？
- 为什么析构函数不能抛异常？
- `noexcept` 有什么用？为什么影响 `vector` 扩容？
- UE 为什么不用异常？那错误怎么处理？
- `check` / `ensure` / `verify` 的区别？
- 什么是 UB？举三个例子。

### 易错处

- `catch (std::exception e)` 按值捕获导致切片，应写 `catch (const std::exception& e)`。
- 析构函数里 `throw`，程序直接终止。
- 用异常做常规控制流，性能和可读性都差。
- 在 UE 项目里写 try/catch（默认配置下编译失败或被禁用）。
- 用 `check` 做 Shipping 也必须成立的校验（Shipping 里它不存在）。

### 项目对照

- [PP_EventBus.cpp](../../Source/PersonalProjectCore/Private/Events/PP_EventBus.cpp) 的 `UPP_EventBus::Get`（L6–15）：三段判空后返回 `nullptr`，调用方判空使用——典型的 UE "能返回空就不抛"风格。
- [PP_NPCActor.cpp](../../Source/PersonalProjectQuest/Private/Dialogue/PP_NPCActor.cpp) L68 注释：直接经接口指针调用 BlueprintNativeEvent 会命中 `check(0 && "Do not directly call Event functions in Interfaces")`，必须走 `Execute_`。
- [PP_BaseCharacter.cpp](../../Source/PersonalProjectCore/Private/Characters/PP_BaseCharacter.cpp) L205 注释提到 ALS 的 `ensure(AnimationInstance.IsValid())`：非致命断言，记录后继续跑。

### 一句话自检

能区分 `check` / `ensure` / `verify` 在 Shipping 下的行为，并说出三个 UB 例子。

---

## 11. 单元 10：编译与链接基础

### 必须掌握

1. **四个阶段**：预处理（展开宏、插入头文件）→ 编译（每个 `.cpp` 生成一个 `.obj`）→ 链接（合并符号、解析引用）→ 生成可执行文件/库。
2. **声明 vs 定义**：声明告诉编译器"存在这个东西"（类型、签名），定义分配存储或给出函数体；可以多次声明，只能一次定义。
3. **ODR（单一定义规则）**：同一实体在整个程序中只能有一个定义；`inline` 函数、模板、类定义允许在多个翻译单元出现相同定义。
4. **头文件规则**：`#pragma once` 防重复包含；头文件里只放声明、`inline` 函数、模板；能前向声明就不 `#include`。
5. **前向声明**：`class UFoo;` 只能用于指针/引用；定义对象、访问成员、`sizeof` 都需要完整类型，否则 C2027。
6. **链接属性**：
   - `static` 修饰全局变量/函数 → 内部链接，每个翻译单元一份
   - `static` 修饰局部变量 → 静态存储期（单元 1）
   - `extern` → 声明外部符号
   - `inline` → 允许多个翻译单元出现同一份定义
7. **模板为什么放头文件**：模板在实例化时才生成代码，编译器必须看到完整定义。
8. **常见错误**：LNK2019（声明了但没定义/没链接库）、LNK2005（重复定义）、C2027（不完整类型）、循环 include。

### 常见面试问法

- 声明和定义的区别？
- 头文件里为什么不能直接定义全局变量？`inline` 解决什么问题？
- `static` 修饰全局变量、函数、局部变量分别是什么含义？
- 为什么模板的实现要写在头文件？
- LNK2019 一般是什么原因？LNK2005 呢？
- 前向声明能用在什么场景？

### 易错处

- 头文件里定义非 `inline` 全局变量 → 多个 `.cpp` 包含后重复定义。
- 前向声明后直接访问成员或 `sizeof` → C2027。
- 循环 include 没有用前向声明打断。
- 只声明没定义，编译通过、链接才报错（LNK2019）。
- 模板实现放在 `.cpp` 里，别的翻译单元实例化时找不到定义。

### 项目对照

- [PP_AttributeSet.h](../../Source/PersonalProjectCore/Public/AbilitySystem/PP_AttributeSet.h) L6–8 的注释：`ATTRIBUTE_ACCESSORS` 展开的宏要调用 `UAbilitySystemComponent` 成员，只前向声明会因**类型不完整**报 C2027，所以必须 include。
- 同文件 `class PERSONALPROJECTCORE_API UPP_AttributeSet`（L21–22）：模块导出宏，跨模块引用时决定符号导出/导入；这属于链接层面的约定。
- 五个模块的 `Build.cs` 依赖方向为什么不能成环，见 [学习路线.md](学习路线.md) A6——那里讲架构，这里只讲"为什么成环会在编译/链接阶段炸"。

### 一句话自检

能解释「编译过了但链接报 LNK2019」通常缺什么，以及前向声明为什么不能定义对象。

---

## 12. 单元 11：UObject 与 GC 基础

### 必须掌握

1. **UObject 由引擎创建**：`NewObject<T>()`、`CreateDefaultSubobject<T>()`（构造函数里创建子对象）、`SpawnActor<T>()`（Actor）。**不能 `new` / `delete`**：UObject 的构造流程、内存分配、GC 标记都不在用户代码里。
2. **GC 是标记清除**：从根集合（Root Set）出发，沿引用图标记可达对象，未标记的回收。根包括 GameInstance、World、PlayerController 等引擎持有的对象。
3. **`UPROPERTY()` 让引用"可见"**：反射系统知道这个成员引用了一个 UObject，GC 才会把它当作强引用遍历。裸指针成员不加 `UPROPERTY`，GC 不知道你在引用它。
4. **`TObjectPtr<T>`**：UE5 的对象指针包装，编辑器和运行时提供访问追踪，支持可选 GC 屏障；但它**仍然需要 `UPROPERTY`** 才是强引用。
5. **`TWeakObjectPtr<T>`**：弱引用，不阻止回收；用 `IsValid()` 判有效、`Get()` 取裸指针。
6. **`IsValid(Obj)` vs `Obj != nullptr`**：前者同时判断空和"已标记待销毁"，后者不够。
7. **`AddToRoot` / `RemoveFromRoot`**：手动把对象加入/移出根集合，非必要不要用。
8. **普通 C++ 对象**（非 UObject）才用 `new/delete`、`unique_ptr`、`shared_ptr`。

### 常见面试问法

- UE 里为什么不能 `new` UObject？
- `UPROPERTY` 除了暴露给编辑器还有什么作用？
- `TObjectPtr` 和裸指针有什么区别？为什么 UE5 要引入它？
- `TWeakObjectPtr` 用在哪？和 `weak_ptr` 有什么异同？
- GC 什么时候触发？为什么会出现"对象突然没了"？
- `IsValid` 和 `!= nullptr` 有什么区别？

### 易错处

- 成员裸指针持有 UObject 且不加 `UPROPERTY` → 被 GC 回收后悬垂。
- 用 `!= nullptr` 代替 `IsValid`，对象已标记销毁但仍非空。
- 对 UObject 使用 `shared_ptr`（生命周期被两套机制管理）。
- 在非 UObject 类里长期持有 UObject 裸指针。
- 以为加了 `TObjectPtr` 就自动保命（还要 `UPROPERTY`）。

### 项目对照

- [PP_WidgetPool.h](../../Source/PersonalProjectUI/Public/Framework/PP_WidgetPool.h) 的 `UPROPERTY() TArray<TObjectPtr<UUserWidget>> Available;`（L39–44）：池强引用可用 Widget，保证它们不被 GC。
- 同文件 `TWeakObjectPtr<UUserWidget> OwnerWidget;`（L50）：弱引用拥有者，不延长其生命周期。
- [PP_LockOnComponent.h](../../Source/PersonalProject/Public/Characters/PP_LockOnComponent.h) 的 `TWeakObjectPtr<AActor> LockOnTarget;`（L77–78）与 `LockOnTarget.IsValid()`（L43）：目标可能死亡销毁，弱引用 + 有效性检查是标准做法。
- [PP_PlayerCharacter.h](../../Source/PersonalProject/Public/Characters/PP_PlayerCharacter.h) 的 `UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> LockOnAction;`（L216–218）：资产引用必须由 `UPROPERTY` 保引用。
- 为什么 ASC 挂 PlayerState 而不是 Pawn，见 [学习路线.md](学习路线.md) A8——那里讲归属，这里讲"谁持有谁才不会被回收"。

### 一句话自检

能说清「UPROPERTY 为什么能保引用」和「TObjectPtr 不能替代 UPROPERTY」这两句。

---

## 13. 单元 12：UPROPERTY / UFUNCTION 与 UE 约定

### 必须掌握

1. **`UPROPERTY` 的四件事**：让 GC 看见引用、序列化、暴露给编辑器和蓝图、参与网络复制。说明符决定具体开启哪些。
2. **常用说明符**：`EditAnywhere` / `EditDefaultsOnly` / `VisibleAnywhere`、`BlueprintReadOnly` / `BlueprintReadWrite`、`Replicated` / `ReplicatedUsing`、`Config`、`Transient`。
3. **`UFUNCTION`**：
   - `BlueprintCallable`：蓝图可调用
   - `BlueprintPure`：无副作用，蓝图节点无执行引脚
   - `BlueprintImplementableEvent`：C++ 只声明，蓝图实现
   - `BlueprintNativeEvent`：C++ 提供默认实现（写 `_Implementation`），蓝图可覆盖
4. **网络说明符**：`Server`（客户端调用、服务器执行）、`Client`（服务器调用、拥有者客户端执行）、`NetMulticast`；配 `Reliable` / `Unreliable`。
5. **接口调用**：`BlueprintNativeEvent` 的接口函数在 C++ 侧必须用 `Execute_函数名(对象, 参数...)`，不能直接经接口指针调用。
6. **UE 容器 vs STL**：
   - `TArray` ≈ `vector`：`Num` / `Add` / `Emplace` / `RemoveAt` / `RemoveAtSwap` / `IndexOfByPredicate`
   - `TMap` ≈ `unordered_map`：`Find` / `FindOrAdd` / `Contains` / `Remove`
   - `TSet` ≈ `unordered_set`：`Contains` / `Add` / `Remove`
   - 差异：UE 容器可参与反射（`UPROPERTY`）、有 `RemoveAtSwap`（O(1) 但不保序）、默认按值语义且与 GC 集成。
7. **命名约定**：
   - `U`：UObject 派生类；`A`：Actor 派生类；`F`：结构体/普通类；`E`：枚举；`I`：接口；`T`：模板
   - 布尔成员 `b` 前缀（`bAlive`）；复制回调 `OnRep_` 前缀；访问器 `Get/Set/Is`
8. **所有权约定**：`UPROPERTY` 强引用（拥有）；`TWeakObjectPtr` 观察；裸指针只用于"别人负责生命周期"或函数参数；函数参数不要写 `TObjectPtr`。

### 常见面试问法

- `UPROPERTY` 的作用是什么？不加会怎样？
- `BlueprintNativeEvent` 和 `BlueprintImplementableEvent` 的区别？C++ 里怎么调用？
- `Server` / `Client` / `NetMulticast` 分别是什么？`Reliable` 的代价？
- `TArray` 和 `std::vector` 有什么区别？
- 为什么 UE 用 `U/A/F/E/I` 前缀？
- 什么时候用 `TObjectPtr`，什么时候用裸指针？

### 易错处

- `BlueprintNativeEvent` 在 C++ 里直接调用（应走 `Execute_` 或调 `_Implementation`）。
- `ReplicatedUsing` 指定的函数忘了加 `UFUNCTION()`。
- `TArray<TObjectPtr<T>>` 成员没写 `UPROPERTY`，GC 看不见。
- 把 `TObjectPtr` 用在函数参数/返回值上（约定用裸指针或引用）。
- 结构体里的 UObject 指针只给内层加了 `UPROPERTY`，外层容器没加。

### 项目对照

- [PP_InventoryComponent.h](../../Source/PersonalProjectInventory/Public/Inventory/PP_InventoryComponent.h) 的 `UPROPERTY(ReplicatedUsing = OnRep_Equipped, BlueprintReadOnly, Category = "Inventory") TArray<FPP_EquippedItem> EquippedItems;`（L186–188）：一条声明同时做了复制、回调、蓝图只读。
- 同文件 `UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|Equipment") void Server_EquipItem(FName ItemID);`（L110–111）：客户端发起、服务器执行。
- [PP_InteractableInterface.h](../../Source/PersonalProjectCore/Public/Interfaces/PP_InteractableInterface.h) 的 `UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void ResetToSpawn();` + `virtual void ResetToSpawn_Implementation() {}`（L47–49）：C++ 默认实现 + 蓝图可覆盖。
- [PP_PlayerCharacter.h](../../Source/PersonalProject/Public/Characters/PP_PlayerCharacter.h) 的 `TMap<TObjectPtr<USkeletalMesh>, TObjectPtr<UAnimMontage>> RollMontageByMesh;`（L207–208）：UE 容器 + 对象指针 + `UPROPERTY`，三者缺一不可。

### 一句话自检

能说出 `UPROPERTY` 的四个作用，并解释 `BlueprintNativeEvent` 为什么必须走 `Execute_`。

---

## 14. 与 30 题题库的边界

| 本单元 | 30 题中的相关题 | 分工 |
| --- | --- | --- |
| 单元 2、5、12 | A6 | 那份讲「五模块为什么单向依赖、UI 网关为什么放 UI 模块」；这份讲接口、虚函数、链接符号的语言机制 |
| 单元 3、11 | A8、B10 | 那份讲 ASC/背包挂 PlayerState、软引用懒加载策略；这份讲 GC 可达性、智能指针与弱引用 |
| 单元 7 | A7 | 那份讲 EventBus 的边界与订阅生命周期；这份讲「遍历时改容器」的迭代器失效 |
| 单元 5、12 | B4 | 那份讲 `SelectRollMontage` 扩展点设计；这份讲 `override` 与 `BlueprintNativeEvent` 的规则 |
| 单元 12 | B6、B7 | 那份讲属性复制与 RPC 的取舍；这份讲 `ReplicatedUsing` / `Server` / `Reliable` 这些宏的含义 |
| 单元 1、4、6、8、9、10 | 无 | 30 题未覆盖的纯语言补强 |

> 两份文档都答不上来时，**先补这份**：语言层不清楚，项目层的解释只能靠背。

---

## 15. 自测题（36 题，先答后看）

用法：合上文档，每题用 3–5 句口述或写下要点，再对照 [§16](#16-答案与评分标准)。**不要先看答案。**

### 单元 1

- **Q1** 函数内的 `int a;` 和全局的 `int a;` 分别是什么值？`static int b;` 在函数内呢？
- **Q2** `int x = -1; unsigned y = 1;` 那么 `x < y` 是真还是假？为什么？
- **Q3** `int a = 3.9;` 和 `int b{3.9};` 有什么区别？

### 单元 2

- **Q4** `const char* const p = "hi";` 中哪部分不能改？如果只写 `const char* p`，哪部分能改？
- **Q5** 函数返回 `const std::string&`，但返回的是函数内局部变量，会发生什么？
- **Q6** 项目里 `GetItems()` 返回 `TArray<FPP_ItemInstance>` 而不是 `const TArray<...>&`，为什么？

### 单元 3

- **Q7** `new/delete` 和 `malloc/free` 的三个主要区别。
- **Q8** 两个对象互相持有 `shared_ptr` 会怎样？`weak_ptr` 为什么能解决？
- **Q9** `TWeakObjectPtr<AActor>` 和 `AActor*` 作为成员，差别是什么？

### 单元 4

- **Q10** 成员初始化列表的执行顺序由什么决定？列表写反会怎样？
- **Q11** 哪三类成员必须用初始化列表？
- **Q12** 基类析构函数不是 virtual，通过基类指针 `delete` 派生类对象会怎样？

### 单元 5

- **Q13** 虚函数实现动态绑定的机制是什么？对象内存里多了什么？
- **Q14** 构造函数里调用虚函数会调用到哪个版本？为什么？
- **Q15** `override` 关键字解决什么问题？不写可能出什么错？

### 单元 6

- **Q16** 用一句话分别说明 `static_cast` / `dynamic_cast` / `const_cast` / `reinterpret_cast` 的用途。
- **Q17** `dynamic_cast` 转换失败时分别怎么表现（指针/引用）？为什么要求类型多态？
- **Q18** `const_cast` 去掉 const 后写入，什么情况下是 UB？

### 单元 7

- **Q19** `vector` 扩容后，之前保存的元素指针、引用、迭代器会怎样？
- **Q20** 遍历 `vector` 删除满足条件的元素，写出两种正确写法。
- **Q21** `map[key]` 和 `map.find(key)` 在只读查询时有什么区别？

### 单元 8

- **Q22** `FString` / `FName` / `FText` 分别适合什么场景？
- **Q23** `UE_LOG(LogTemp, Log, TEXT("%s"), SomeFString)` 为什么不对？正确写法是什么？
- **Q24** `FName` 为什么适合做 Map 的 key？

### 单元 9

- **Q25** 异常安全的三个等级分别是什么？
- **Q26** 为什么析构函数不能抛异常？
- **Q27** `check` / `ensure` / `verify` 在 Shipping 配置下分别是什么行为？

### 单元 10

- **Q28** 声明和定义的区别？只声明不定义会在哪个阶段报错？
- **Q29** 为什么模板的实现通常要放在头文件？
- **Q30** LNK2019 和 LNK2005 分别是什么错误？

### 单元 11

- **Q31** UE 里为什么不能 `new` 一个 UObject？
- **Q32** `UPROPERTY` 对 GC 的作用是什么？成员裸指针不加会怎样？
- **Q33** `TObjectPtr` 和 `TWeakObjectPtr` 分别用在什么场景？

### 单元 12

- **Q34** `BlueprintNativeEvent` 和 `BlueprintImplementableEvent` 的区别？C++ 侧分别怎么调用/实现？
- **Q35** 说出 `TArray` 与 `std::vector` 的三个差异。
- **Q36** `U` / `A` / `F` / `E` / `I` 前缀各表示什么？布尔成员为什么加 `b`？

---

## 16. 答案与评分标准

### 评分

每题按三档记：

| 档位 | 标准 | 记分 |
| --- | --- | --- |
| 完整 | 要点齐全，能说出原因 | 2 |
| 部分 | 记得结论，说不清原因 | 1 |
| 不会 | 答错或空白 | 0 |

- 单单元 6 分满分；**低于 4 分**就重读该单元的「必须掌握」。
- 全部 36 题低于 48 分：先补单元 1–6，再碰 UE 相关单元。
- 单元 11–12 低于 8 分（12 分制）：回去看 [学习路线.md](学习路线.md) 的 A8、B6、B10，语言和项目要一起补。

### 单元 1

**Q1** 函数内 `int a;` 是默认初始化，内置类型值**不确定**；全局 `int a;` 是静态存储期，零初始化；函数内 `static int b;` 也是静态存储期，零初始化，且首次执行到声明时初始化一次。

**Q2** 假（`x < y` 为 false）。有符号与无符号混算时，`x` 被转成无符号，`-1` 变成 `UINT_MAX`，远大于 1。

**Q3** `int a = 3.9;` 是拷贝初始化，静默截断为 3；`int b{3.9};` 是列表初始化，窄化转换**编译报错**。

### 单元 2

**Q4** `const char* const p`：指针本身和指向的内容都不能改。只写 `const char* p`：不能通过 `p` 改内容，但可以让 `p` 指向别处。

**Q5** 返回的是局部对象的引用，函数返回后对象已销毁，引用悬垂；使用它是 UB。编译器可能给警告但不一定报错。

**Q6** 返回引用会把内部数组暴露给调用方，调用方可能在之后增删元素导致引用/指针失效；返回副本牺牲一次拷贝，换取调用方安全。项目里 UI 只读渲染，副本足够。

### 单元 3

**Q7** ① `new` 分配内存后调用构造函数，`malloc` 只分配原始内存；② `delete` 先析构再释放，`free` 只释放；③ `new` 返回正确类型的指针且失败抛 `bad_alloc`，`malloc` 返回 `void*` 且失败返回 `nullptr`。

**Q8** 引用计数永远不归零，两个对象都不会释放（循环引用泄漏）。把其中一方改成 `weak_ptr`：弱引用不增加计数，对象销毁后 `lock()` 返回空。

**Q9** `AActor*` 是裸指针，对象被销毁后变成悬垂指针，无法判断；`TWeakObjectPtr` 不阻止 GC，对象销毁后 `IsValid()` 返回 false，可以安全判空。

### 单元 4

**Q10** 由**成员的声明顺序**决定，与初始化列表书写顺序无关。写反可能导致用未初始化的成员去初始化另一个成员（编译器可能给警告）。

**Q11** `const` 成员、引用成员、没有默认构造函数的类类型成员。

**Q12** UB：只调用基类析构，派生类的析构不执行，派生类持有的资源泄漏。

### 单元 5

**Q13** 每个多态类有一张虚函数表，对象里有一个 vptr 指向它；通过指针/引用调用虚函数时，运行时经 vptr 查表得到实际函数地址。

**Q14** 调用到**当前正在构造的类**的版本，不会派发到派生类——构造派生类时先构造基类，此时派生类部分还没建立。

**Q15** `override` 让编译器检查是否真的覆盖了基类虚函数；不写时签名差一个 `const` 或参数类型就会变成新函数（隐藏），运行时行为不符合预期且不报错。

### 单元 6

**Q16** `static_cast`：相关类型间的编译期转换；`dynamic_cast`：多态类型的安全运行时转换；`const_cast`：增删 const/volatile；`reinterpret_cast`：按位重新解释。

**Q17** 指针失败返回 `nullptr`，引用失败抛 `std::bad_cast`。因为需要运行时类型信息（RTTI）来判断实际类型，只有多态类型才有完整的类型信息。

**Q18** 当对象本身**真正是 const**（如定义在只读段、或本身声明为 const）时，写入是 UB；只有对象原本非 const、只是通过 const 指针/引用访问时，去掉 const 写入才合法。

### 单元 7

**Q19** 全部失效：扩容会重新分配内存并搬移元素，原来的指针、引用、迭代器都指向已释放的旧内存。

**Q20** ① 倒序遍历删除（`for (int i = size - 1; i >= 0; --i)`）；② `it = v.erase(it)` 用返回值继续，或 `erase(std::remove_if(...), v.end())`。

**Q21** `operator[]` 在 key 不存在时会**插入**一个默认值，改变容器；`find` 只查询，返回迭代器，找不到返回 `end()`。

### 单元 8

**Q22** `FString`：需要修改/拼接/格式化的文本；`FName`：ID、资源名、行名等需要快速比较且不区分大小写的标识；`FText`：面向玩家显示、需要本地化的文本。

**Q23** `%s` 需要 `const TCHAR*`，`FString` 不是。正确写法：`*SomeFString`（先确保它是 `FString`，必要时 `.ToString()`）。

**Q24** `FName` 内部是全局池化的索引，比较只需比较索引（几乎 O(1)），不区分大小写，适合做 key；`FString` 比较是逐字符的。

### 单元 9

**Q25** 基本保证（不泄漏、对象有效）、强保证（失败回滚，像事务）、不抛保证（`noexcept`）。

**Q26** 析构通常在栈展开过程中执行，此时再抛异常会导致两个异常同时存在，C++ 直接调用 `std::terminate`；而且析构抛异常会破坏 RAII 的可靠性。

**Q27** `check`：Shipping 中被编译掉，不执行；`ensure`：在 Development 中记录一次后继续执行，Shipping 中由 `DO_GUARD_SLOW` 控制，通常也被编译掉；`verify`：表达式仍会求值，但失败检查在 Shipping 中关闭。

### 单元 10

**Q28** 声明只告诉编译器类型和签名，定义分配存储或提供函数体。只声明不定义：编译通过，**链接阶段**报 LNK2019（未解析的外部符号）。

**Q29** 模板在实例化时才生成具体代码，编译器必须看到完整定义才能实例化；放在 `.cpp` 里其他翻译单元看不到，会链接失败。

**Q30** LNK2019：引用了但找不到定义的符号（没实现、没链接库、签名不匹配）；LNK2005：同一符号被定义多次（头文件里定义了非 inline 全局变量/函数）。

### 单元 11

**Q31** UObject 的构造流程、内存分配、GC 标记和销毁都由引擎管理；`new` 出来的对象没有正确的 UObject 头部、不进入 GC 系统，析构也不会走引擎流程。

**Q32** `UPROPERTY` 让反射系统记录这个引用，GC 遍历引用图时才把它当作强引用。不加 `UPROPERTY` 的裸指针，GC 看不见，对象可能被回收，指针变悬垂。

**Q33** `TObjectPtr`：需要长期持有、由本对象拥有的 UObject 引用（资产、组件、子对象），配 `UPROPERTY` 使用；`TWeakObjectPtr`：只观察、不拥有，如锁定目标、池的拥有者、可能被销毁的 Actor。

### 单元 12

**Q34** `BlueprintImplementableEvent`：C++ 只声明，蓝图必须实现，C++ 直接调用即可；`BlueprintNativeEvent`：C++ 提供 `_Implementation` 默认实现，蓝图可选择覆盖，C++ 侧调用必须走 `Execute_函数名`（接口场景）或直接调函数（UObject 成员场景）。

**Q35** ① `TArray` 是 UE 容器，可参与反射和 `UPROPERTY`，`std::vector` 不能；② `TArray` 用 `Num()` / `Add` / `Emplace` / `RemoveAtSwap`，命名和 STL 不同；③ `TArray` 的 `RemoveAtSwap` 是 O(1) 但不保序，`std::vector::erase` 保序但 O(n)；④ `TArray` 默认与 UE 内存分配器、序列化集成。

**Q36** `U`：UObject 派生类；`A`：Actor 派生类；`F`：结构体或普通 C++ 类；`E`：枚举；`I`：接口。布尔成员加 `b` 前缀是 UE 命名约定（匈牙利式），让布尔语义在名字里可见（`bAlive`、`bCanUseOffHand`）。

---

## 17. 以后再说（现在不学）

| 主题 | 为什么现在不学 | 什么时候再碰 |
| --- | --- | --- |
| 模板元编程（SFINAE、`if constexpr`、变参模板） | 面试基础岗很少深问，项目里也基本是读引擎代码才遇到 | 需要写通用库/读懂引擎模板时 |
| 完美转发与移动语义深挖（`forward`、引用折叠、`noexcept` 细节） | 单元 4 的移动语义基础已够日常使用 | 自己写容器/工厂、性能优化时 |
| 并发与内存模型（`atomic`、`memory_order`、无锁） | UE  gameplay 代码基本单线程（GameThread），并发多在渲染/任务系统 | 做 TaskGraph、异步加载、多线程系统时 |
| C++20 协程与 Concepts | UE 5.7 代码库尚未大规模使用，面试基础题不考 | 引擎升级到大规模采用 C++20 时 |
| allocator 与内存池 | 项目用 UE 自带分配器，手写 allocator 场景极少 | 需要自定义容器/性能剖析时 |
| SIMD 与 intrinsics | 与 gameplay 无关 | 做数学/物理底层优化时 |
| 编译期计算（`constexpr` 深水区、`consteval`） | 基础 `constexpr` 会用即可 | 写配置表编译期校验时 |
| 引擎渲染/网络底层 | 属于引擎专题，不在「C++ 基础」范围 | 按 [学习路线.md](学习路线.md) B3、B6、B7 的触发条件再学 |

> 这张表不是待办清单。**没有具体需求就不要提前学**，先把 12 个单元的自测做到 48 分以上。

---

## 18. 维护规则

- 本文件只做语言基础与 UE 映射，不收录项目架构与引擎机制细节；后者继续放 [学习路线.md](学习路线.md)。
- 新增单元时同步更新 [§1 单元清单](#1-单元清单) 与 [§15 自测题](#15-自测题36-题先答后看) 的编号。
- 项目对照里的行号随代码漂移，**以函数名/类名为准**；发现引用失效时直接修正，不必保留旧行号。
- 本文件新增或移动时，同步更新 [docs/README.md](../README.md) 的索引。

