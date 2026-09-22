--
-- DESCRIPTION
--
-- @COMPANY 
-- @AUTHOR 
-- @DATE 2026/09/20 20:46:19
--

---@type WBP_CommonBtn_C
local M = UnLua.Class()

function M:Initialize(Initializer)
    self._data = nil
    self._callBack = nil
    self._binObj = nil
    self._binParame = nil
end

--function M:PreConstruct(IsDesignTime)
--end

function M:Construct()
    self.Button_99.OnClicked:Add(self, self.OnBtnClick)
end

function M:OnListItemObjectSet(Obj)
    self._data = Obj
    self.TextBlock_24:SetText(Obj.Name)
    if Obj.IconPath then
        local texture = UE.UObject.Load(Obj.IconPath)
        self.Image_60:SetBrushFromTexture(texture)
    end
    self._binObj = Obj.BinObj
    self._binParame = Obj.Parame or {}
    self._callBack = Obj.Func
end

-- function M:Tick(MyGeometry, InDeltaTime)
-- end

function M:OnBtnClick()
    if self._callBack then
        self._callBack(self._binObj,self._binParame)
    end
end

return M
