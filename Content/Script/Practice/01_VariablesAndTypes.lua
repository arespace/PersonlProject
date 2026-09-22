print("===== 01 变量与类型 =====")

-- Lua 的变量默认是【全局】的，加 local 才是【局部】。
-- 习惯：尽量都用 local。

local a = 10                 -- number（整数）
local b = 3.14               -- number（小数）
local c = "hello"            -- string
local d = 'world'            -- string（单引号也行）
local e = true               -- boolean
local f = nil                -- nil 表示"没有值"
local g = { 1, 2, 3 }        -- table（数组）
local h = function() end     -- function

-- 用 type() 查看类型
print(type(a), type(b), type(c), type(e), type(f), type(g), type(h))
-- 输出: number number string boolean nil table function

-- nil 的用途：表示"空"，给 table 字段赋 nil 相当于删除该字段
local t = { x = 1, y = 2 }
t.x = nil
print(t.x)       -- 输出: nil

-- 多个变量同时赋值（先算右边，再赋给左边）
local m, n = 1, 2
m, n = n, m      -- 一行交换两个值
print(m, n)      -- 输出: 2 1

-- 字符串拼接用 .. （两个点）；数字和字符串不能直接相加
local name = "小白"
local age = 18
print("我叫" .. name .. "，今年" .. age .. "岁")
