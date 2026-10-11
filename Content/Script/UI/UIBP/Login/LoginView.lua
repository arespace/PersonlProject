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
    -- 数据驱动：菜单项由 DataTable (DT_LoginMenu) 配置，这里只负责遍历与展示。
    -- 数据表和条目类都来自 WBP_LoginView 的属性（C++ 基类 UPP_LoginViewWidget 上声明，
    -- 在 Class Defaults 里指过去）——不再用 UE.UClass.Load('/Game/...') 这类字符串路径：
    -- 字符串路径烘焙器看不见，打进包的构建里资产会丢（登录菜单曾经全空就是这个原因）。
    if not self.LoginMenuTable or not self.MenuItemClass then
        print("[LoginView] 配置缺失：请在 WBP_LoginView 的 Class Defaults 里设置 LoginMenuTable / MenuItemClass")
        self:RefreshHealth()
        return
    end

    local MenuItems = UE.UPP_MenuDataLibrary.GetLoginMenuItems(self.LoginMenuTable)
    self.Actions = {}
    for i = 1, MenuItems:Num() do
        local item = MenuItems:Get(i)
        self.Actions[i] = item.Action
        local NewObj = NewObject(self.MenuItemClass)
        NewObj.Name = item.Name
        NewObj.Func = self.HandleItemClicked
        NewObj.BinObj = self
        NewObj.Parame = i
        -- IconPath 是字符串路径，由 C++ 从表里的 Icon 软引用回填（图标已被软引用带进包）
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
