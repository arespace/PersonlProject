print("===== 05 字符串 =====")

local s = "hello world"

-- 取长度
print(#s)                -- 输出: 11

-- 拼接
print("前缀-" .. s .. "-后缀")

-- string.format 格式化
print(string.format("我叫%s，今年%d岁", "小白", 18))
print(string.format("保留两位小数：%.2f", 3.14159))

-- 大小写转换
print(string.upper(s))   -- 输出: HELLO WORLD
print(string.lower(s))   -- 输出: hello world

-- 查找子串（返回两个值：起始位置、结束位置；找不到返回 nil）
local startPos, endPos = string.find(s, "world")
print(startPos, endPos)    -- 输出: 7 11

-- 替换（返回两个值：替换后的字符串、替换次数）
local replaced = string.gsub(s, "world", "Lua")
print(replaced)          -- 输出: hello Lua

-- 分割：Lua 没有自带 split，用 gmatch 循环逐个取
for word in string.gmatch(s, "%a+") do
    print("单词:", word)
end

-- 转义字符
print("换行：\n第二行")
print("制表符：\t对齐")
