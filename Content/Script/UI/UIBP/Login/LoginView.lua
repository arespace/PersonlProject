--
-- DESCRIPTION
--
-- @COMPANY
-- @AUTHOR
-- @DATE 2026/09/20 18:52:18
--

---@type WBP_LoginView_C
local M = UnLua.Class()

--function M:Initialize(Initializer)
--end

--function M:PreConstruct(IsDesignTime)
--end

function M:Construct()
    -- 数据驱动：菜单项由 DataTable (DT_LoginMenu) 配置，这里只负责遍历与展示
    local ObjectClass = UE.UClass.Load('/Game/UI/UIObj/BP_ItemData.BP_ItemData_C')
    local MenuItems = UE.UPP_MenuDataLibrary.GetLoginMenuItems()
    self.Actions = {}
    for i = 1, MenuItems:Num() do
        local item = MenuItems:Get(i)
        self.Actions[i] = item.Action
        local NewObj = NewObject(ObjectClass)
        NewObj.Name = item.Name
        NewObj.Func = self.HandleItemClicked
        NewObj.BinObj = self
        NewObj.Parame = i
        NewObj.IconPath = item.IconPath
        self.ListView_84:AddItem(NewObj)
    end

    -- P0 关键：Lua 调用 C++ 接口读取实时血量，实现 Lua ↔ C++ 通信
    self:RefreshHealth()
end

-- 从 C++ 读取玩家血量并显示（数据回传链路：GAS AttributeSet -> C++ -> Lua -> UI）
function M:RefreshHealth()
    local Health = UE.UPP_GameplayStatsLibrary.GetPlayerHealth(self)
    local MaxHealth = UE.UPP_GameplayStatsLibrary.GetPlayerMaxHealth(self)
    local Text = string.format("HP: %.0f / %.0f", Health, MaxHealth)
    print("[LoginView] C++ -> Lua HP =", Text)

    -- HPText 是 WBP_LoginView 里绑定的 TextBlock；未放置时不报错，仅打印
    if self.HPText then
        self.HPText:SetText(Text)
    end
end

-- 实时刷新（节流 0.5s）；需 WBP_LoginView 开启 Tick 才会触发
function M:Tick(MyGeometry, InDeltaTime)
    self.HealthRefreshAcc = (self.HealthRefreshAcc or 0) + InDeltaTime
    if self.HealthRefreshAcc < 0.5 then
        return
    end
    self.HealthRefreshAcc = 0
    self:RefreshHealth()
end

function M:HandleItemClicked(index)
    local action = self.Actions[index]
    print("[LoginView] clicked index=", index, " action=", action)

    if action == "NewGame" then
        -- 进入游戏：出栈 → NativeDestruct → UIManager:PopWidget → 输入模式切回 Gameplay
        self:RemoveFromParent()
    elseif action == "Quit" then
        -- 退出游戏（QuitGame 在 UKismetSystemLibrary 上，不在 UGameplayStatics）
        UE.UKismetSystemLibrary.QuitGame(self, self:GetOwningPlayer(), UE.EQuitPreference.Quit, false)
    end
end

return M
