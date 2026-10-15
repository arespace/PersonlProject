-- 世界服务：宿主是 ULuaWorldService (UWorldSubsystem)，由 UnLua 驱动。
-- 关键前提：UWorldSubsystem 是「每个 World 各一份实例」的——
-- 任何关卡开始播放都会走一遍 LuaOnWorldBeginPlay，所以这里做 UI 操作前必须先按关卡过滤，
-- 否则就是「不管开哪个关卡都弹登录界面」。

-- 只有这些关卡启动时才弹登录界面。用地图短名（不含路径、不带 .umap 后缀）；
-- 想让 PPMap 也弹就加一行 `PPMap = true,`。
local LOGIN_VIEW_MAPS = {
    LobbyMap = true,
}

local LOGIN_VIEW_PATH = '/Game/UI/UIBP/Login/WBP_LoginView.WBP_LoginView_C'

local WorldService = UnLua.Class()

function WorldService:LuaInit()
    print("WorldService LuaInit")
end

function WorldService:LuaPostInit()
    print("WorldService LuaPostInit")
end

function WorldService:LuaOnWorldBeginPlay(InWorld)
    if not InWorld then
        return
    end

    -- 取关卡短名。注意 UWorld:GetMapName() 不是 UFUNCTION，Lua 调不到，只能走 GameplayStatics；
    -- bRemovePrefixString = true 会去掉 PIE 的 UEDPIE_<n>_ 前缀，得到干净的 "LobbyMap"。
    local mapName = UE.UGameplayStatics.GetCurrentLevelName(InWorld, true)
    if not LOGIN_VIEW_MAPS[mapName] then
        print(string.format("[WorldService] 关卡 %s 不在登录界面白名单，跳过", tostring(mapName)))
        return
    end

    local widget = self:GetwidgetByPath(LOGIN_VIEW_PATH)
    if not widget then
        print("[WorldService] WBP_LoginView 加载失败: " .. LOGIN_VIEW_PATH)
        return
    end
    widget:AddToViewport()
end

function WorldService:LuaDeinit()
    print("WorldService LuaDeinit")
end

return WorldService
