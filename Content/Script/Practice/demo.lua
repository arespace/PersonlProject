print("===== 06 元表 Metatable =====")

-- 元表：给一个 table 附加"额外行为"的表。
-- 你项目里的 UnLua.Class() 底层就用到了元表。

local a = { value = 10 }
local b = { value = 20 }

-- __add：让两个表能用 + 相加
local mt = {
    __add = function(left, right)
        return left.value + right.value
    end,
}
setmetatable(a, mt)
setmetatable(b, mt)
print(a + b)    -- 输出: 30

-- __index：访问"不存在的字段"时，转去另一个表里找（相当于默认值/继承）
local defaults = { Name = "默认名字", HP = 100 }
local obj = setmetatable({ Name = "自定义名字" }, { __index = defaults })
print(obj.Name)    -- 输出: 自定义名字（自己有，直接用）
print(obj.HP)      -- 输出: 100（自己没有，去 defaults 里找）

-- __newindex：给"不存在的字段"赋值时触发
local guarded = setmetatable({}, {
    __newindex = function(t, k, v)
        print("你想给 " .. k .. " 赋值为 " .. tostring(v) .. "，但被拦截了")
    end,
})
guarded.anything = 123    -- 触发 __newindex，字段不会被真的写入
print(guarded.anything)   -- 输出: nil

-- 常见套路：__index 指向自己，实现"方法"
local Shape = {}
Shape.__index = Shape
function Shape:area()
    return "不知道面积"
end

local square = setmetatable({}, Shape)
print(square:area())   -- 输出: 不知道面积
