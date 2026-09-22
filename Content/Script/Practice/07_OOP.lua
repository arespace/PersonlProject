print("===== 07 用元表模拟面向对象 =====")

-- Lua 没有 class 关键字，用"元表 + __index"模拟类。

-- 定义"类"
local Animal = {}
Animal.__index = Animal

-- 构造函数
function Animal.new(name)
    local obj = setmetatable({}, Animal)
    obj.name = name
    return obj
end

-- 方法（冒号定义，self 是调用它的那个实例）
function Animal:speak()
    print(self.name .. " 发出了声音")
end

local cat = Animal.new("小猫")
cat:speak()    -- 输出: 小猫 发出了声音

-- 继承：子类的元表 __index 指向父类
local Dog = setmetatable({}, { __index = Animal })
Dog.__index = Dog

function Dog.new(name)
    local obj = Animal.new(name)   -- 先按父类构造
    setmetatable(obj, Dog)         -- 再把元表换成子类
    return obj
end

function Dog:speak()               -- 覆盖父类方法
    print(self.name .. " 汪汪叫")
end

local dog = Dog.new("小狗")
dog:speak()    -- 输出: 小狗 汪汪叫

-- 说明：UnLua 里的 UnLua.Class() 就是在帮你封装这套"元表 + __index"机制，
-- 所以你能用 M:Construct()、self.HandleItemClicked 这种写法。
