-- ============================================================
-- Lua 基础练习 - 入口
--
-- 运行方式（在编辑器控制台输入）：
--   lua.dofile Practice.Main                   -- 运行全部
--   lua.dofile Practice.01_VariablesAndTypes   -- 只运行某一节
--   lua.do print("hello")                      -- 执行一句 Lua
-- 输出在 Output Log（LogUnLua）。
-- ============================================================

local topics = {
    "Practice.01_VariablesAndTypes",
    "Practice.02_Tables",
    "Practice.03_ControlFlow",
    "Practice.04_Functions",
    "Practice.05_Strings",
    "Practice.06_Metatables",
    "Practice.07_OOP",
}

for _, moduleName in ipairs(topics) do
    package.loaded[moduleName] = nil   -- 清掉缓存，保证每次都能重新运行
    require(moduleName)
    print()
end

print("===== 全部练习运行完毕 =====")
