-- 调试引导层：Lua 侧唯一一处启动 LuaPanda 的地方。
-- 由 UnLua 的 Startup Module Name 配置挂载（见 Config/DefaultUnLuaSettings.ini），
-- 启动时机早于任何业务模块，业务代码里不要再写 require("LuaPanda").start(...)。

local HOST = "127.0.0.1"
local PORT = 8818

local ok, err = pcall(function()
    require("LuaPanda").start(HOST, PORT)
end)

if not ok then
    print("[DebugBootstrap] LuaPanda 启动失败，不影响游戏运行: " .. tostring(err))
end

return true
