print("===== 02 表 Table =====")

-- table 是 Lua 唯一的数据结构，既能当【数组】也能当【字典（键值对）】。
-- 你项目里的 LoginData 就是一个 table。

-- 当作数组（注意：索引从 1 开始，不是 0！）
local arr = { "a", "b", "c" }
print(arr[1], arr[2], arr[3])    -- 输出: a b c
print(#arr)                      -- # 取长度，输出: 3

-- 当作字典
local dict = {
    Name = "新游戏",
    Index = 1,
    IconPath = "/Game/UI/Icon/qingWa.qingWa",
}
print(dict.Name, dict.Index)     -- 点号访问
print(dict["IconPath"])          -- 方括号+字符串，等价于上面的点号

-- 增、改、删
dict.Desc = "这是一条描述"        -- 增加新字段
dict.Name = "改名了"              -- 修改已有字段
dict.Index = nil                  -- 赋 nil = 删除字段
print(dict.Desc, dict.Name, dict.Index)
-- 输出: 这是一条描述 改名了 nil

-- 遍历数组：ipairs（按顺序，遇到 nil 就停）
for i, v in ipairs(arr) do
    print("arr[" .. i .. "] = " .. v)
end

-- 遍历字典：pairs（无序，适合键值对）
for k, v in pairs(dict) do
    print(k, "=", v)
end
