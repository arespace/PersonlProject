print("===== 03 条件与循环 =====")

-- if / elseif / else（注意：elseif 是一个词，中间没有空格）
local score = 85
if score >= 90 then
    print("优秀")
elseif score >= 60 then
    print("及格")
else
    print("不及格")
end

-- 只有 false 和 nil 算"假"，其余都算"真"（包括 0 和空字符串！）
if 0 then print("0 是真") end
if "" then print("空字符串也是真") end

-- for 数值循环：i 从 1 到 5（含 5）
for i = 1, 5 do
    print("第 " .. i .. " 次")
end

-- 带步长的循环：1, 3, 5
for i = 1, 5, 2 do
    print("步长循环 i =", i)
end

-- while 循环
local n = 1
while n <= 3 do
    print("while n =", n)
    n = n + 1
end

-- repeat ... until（至少执行一次）
local m = 1
repeat
    print("repeat m =", m)
    m = m + 1
until m > 3

-- break 跳出循环（Lua 没有 continue）
for i = 1, 10 do
    if i > 3 then break end
    print("break 演示 i =", i)
end
