// 物品提示框控件实现

#include "Inventory/PP_ItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameplayEffect.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Engine/DataTable.h"

UPP_ItemTooltipWidget::UPP_ItemTooltipWidget()
{
    OwningLayer = EPP_UILayer::Overlay;
    bConsumesAllInput = false;
}

void UPP_ItemTooltipWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UPP_ItemTooltipWidget::SetupTooltip(UPP_InventoryComponent* InInventory,
                                          const FPP_ItemInfo& InItemInfo,
                                          int32 InStackCount,
                                          int32 InMaxStack)
{
    CachedInfo = InItemInfo;

    // 名称 + 稀有度颜色
    if (NameText)
    {
        NameText->SetText(InItemInfo.DisplayName);
    }
    ApplyRarityColor(InItemInfo.Rarity);

    // 类型标签
    if (TypeText)
    {
        FString TypeStr;
        switch (InItemInfo.ItemType)
        {
        case EPP_ItemType::Material:    TypeStr = TEXT("材料"); break;
        case EPP_ItemType::Equipment:   TypeStr = TEXT("装备"); break;
        case EPP_ItemType::Consumable:  TypeStr = TEXT("消耗品"); break;
        case EPP_ItemType::QuestItem:   TypeStr = TEXT("任务物品"); break;
        case EPP_ItemType::Currency:    TypeStr = TEXT("货币"); break;
        default:                        TypeStr = TEXT("未知"); break;
        }

        if (InItemInfo.ItemType == EPP_ItemType::Equipment && InItemInfo.EquipSlot != EPP_EquipSlot::None)
        {
            // 通过枚举反射获取槽位中文名
            const UEnum* EnumPtr = StaticEnum<EPP_EquipSlot>();
            FString SlotStr = FString::Printf(TEXT(" - %s"), *EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(InItemInfo.EquipSlot)).ToString());
            TypeStr += SlotStr;
        }
        TypeText->SetText(FText::FromString(TypeStr));
    }

    // 描述
    if (DescriptionText)
    {
        DescriptionText->SetText(InItemInfo.Description);
    }

    // 价格
    if (PriceText)
    {
        PriceText->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "PriceFmt", "购买: {0}G  出售: {1}G"),
            FText::AsNumber(InItemInfo.BuyPrice),
            FText::AsNumber(InItemInfo.SellPrice)));
    }

    // 堆叠
    if (StackText)
    {
        if (InMaxStack > 0)
        {
            StackText->SetText(FText::Format(
                NSLOCTEXT("Tooltip", "StackFmt", "{0}/{1}"),
                FText::AsNumber(InStackCount),
                FText::AsNumber(InMaxStack)));
            StackText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            StackText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // 属性列表（仅装备有 EquipEffectClass）
    if (StatsContainer)
    {
        StatsContainer->ClearChildren();
        if (InItemInfo.EquipEffectClass)
        {
            TArray<FPP_ModifierSummary> Mods = UPP_ItemUILibrary::GetModifierSummaries(InItemInfo.EquipEffectClass);
            BuildStatsList(Mods);
        }
    }

    // 装备对比（仅当悬停装备且槽位已有装备时）
    if (CompareContainer)
    {
        CompareContainer->ClearChildren();
        if (InItemInfo.ItemType == EPP_ItemType::Equipment &&
            InItemInfo.EquipSlot != EPP_EquipSlot::None &&
            InInventory)
        {
            BuildComparison(InInventory, InItemInfo);
        }
    }
}

void UPP_ItemTooltipWidget::UpdatePosition(FVector2D MousePosition)
{
    // 偏移10px避免遮挡鼠标
    SetPositionInViewport(MousePosition + FVector2D(10.f, 10.f));
}

void UPP_ItemTooltipWidget::BuildStatsList(const TArray<FPP_ModifierSummary>& Modifiers)
{
    if (!StatsContainer) return;

    for (const FPP_ModifierSummary& Mod : Modifiers)
    {
        UTextBlock* StatText = NewObject<UTextBlock>(StatsContainer);
        StatText->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "StatFmt", "{0}: {1}"),
            Mod.AttributeName,
            FText::AsNumber(Mod.ModifierValue)));
        StatText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        StatsContainer->AddChildToVerticalBox(StatText);
    }
}

void UPP_ItemTooltipWidget::BuildComparison(UPP_InventoryComponent* Inventory, const FPP_ItemInfo& HoveredInfo)
{
    if (!CompareContainer || !Inventory) return;

    const FPP_EquippedItem Equipped = Inventory->GetEquippedItemInSlot(HoveredInfo.EquipSlot);
    if (Equipped.ItemID.IsNone()) return;

    const FPP_ItemInfo* EquippedInfo = Inventory->FindItemInfo(Equipped.ItemID);
    if (!EquippedInfo || !EquippedInfo->EquipEffectClass) return;

    TArray<FPP_ModifierSummary> Better, Worse, NewMods, LostMods;
    UPP_ItemUILibrary::CompareModifiers(
        EquippedInfo->EquipEffectClass,
        HoveredInfo.EquipEffectClass,
        Better, Worse, NewMods, LostMods);

    // 标题
    UTextBlock* Header = NewObject<UTextBlock>(CompareContainer);
    Header->SetText(NSLOCTEXT("Tooltip", "CompareHeader", "— 对比 —"));
    Header->SetColorAndOpacity(FSlateColor(FLinearColor::Gray));
    CompareContainer->AddChildToVerticalBox(Header);

    // 更优: 绿色
    for (const FPP_ModifierSummary& M : Better)
    {
        UTextBlock* T = NewObject<UTextBlock>(CompareContainer);
        T->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "BetterFmt", "{0}: +{1} ▲"),
            M.AttributeName, FText::AsNumber(M.ModifierValue)));
        T->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
        CompareContainer->AddChildToVerticalBox(T);
    }

    // 更差: 红色
    for (const FPP_ModifierSummary& M : Worse)
    {
        UTextBlock* T = NewObject<UTextBlock>(CompareContainer);
        T->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "WorseFmt", "{0}: {1} ▼"),
            M.AttributeName, FText::AsNumber(M.ModifierValue)));
        T->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
        CompareContainer->AddChildToVerticalBox(T);
    }

    // 新增: 蓝色
    for (const FPP_ModifierSummary& M : NewMods)
    {
        UTextBlock* T = NewObject<UTextBlock>(CompareContainer);
        T->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "NewFmt", "{0}: +{1} (新增)"),
            M.AttributeName, FText::AsNumber(M.ModifierValue)));
        T->SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.5f, 1.f)));
        CompareContainer->AddChildToVerticalBox(T);
    }

    // 失去: 灰色
    for (const FPP_ModifierSummary& M : LostMods)
    {
        UTextBlock* T = NewObject<UTextBlock>(CompareContainer);
        T->SetText(FText::Format(
            NSLOCTEXT("Tooltip", "LostFmt", "{0}: {1} (失去)"),
            M.AttributeName, FText::AsNumber(M.ModifierValue)));
        T->SetColorAndOpacity(FSlateColor(FLinearColor::Gray));
        CompareContainer->AddChildToVerticalBox(T);
    }
}
