   local WorldService = UnLua.Class()

   function WorldService:LuaInit()
        print("WorldService LuaInit")
   end

	function WorldService:LuaPostInit();
        print("WorldService LuaPostInit")
   end

	function WorldService:LuaOnWorldBeginPlay()
        print("WorldService LuaOnWorldBeginPlay")
        local path = '/Game/UI/UIBP/Login/WBP_LoginView.WBP_LoginView_C'
        local widget = self:GetwidgetByPath(path)
        widget:AddToViewport()
   end
	function WorldService:LuaDeinit()
        print("WorldService LuaDeinit")
   end

   return WorldService
