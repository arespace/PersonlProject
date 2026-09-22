print("===== 04 函数 =====")

-- 定义函数
local function add(a, b)
    return a + b
end
print(add(2, 3))    -- 输出: 5

-- 多返回值
local function getPair()
    return 1, 2
end
local x, y = getPair()
print(x, y)         -- 输出: 1 2

-- 变长参数 ...
local function sum(...)
    local total = 0
    for _, v in ipairs({...}) do   -- {...} 把变长参数打包成一个 table
        total = total + v
    end
    return total
end
print(sum(1, 2, 3, 4))   -- 输出: 10

-- 闭包：函数能"记住"它定义时外面的局部变量
local function makeCounter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end
local counter = makeCounter()
print(counter())   -- 输出: 1
print(counter())   -- 输出: 2
print(counter())   -- 输出: 3

-- 点号调用 vs 冒号调用（重点！UnLua 里的 M:Construct() 就是冒号调用）
local Person = { name = "小白" }

-- 冒号定义方法：自动带一个隐藏参数 self
function Person:greet()
    print("你好，" .. self.name)
end
-- 上面等价于：
-- function Person.greet(self) print("你好，" .. self.name) end

Person:greet()         -- 冒号调用：自动把 Person 当 self 传入
Person.greet(Person)   -- 点号调用：手动把 Person 当 self 传入
-- Person.greet()      -- 错误：没传 self，函数里的 self 会是 nil
