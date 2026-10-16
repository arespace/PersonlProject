// Chen Tianbao uses GAS to create online battle games

#include "HUD/PP_HUD.h"

#include "Framework/PP_UIViewModel.h"
#include "Framework/PP_WidgetPool.h"
#include "HUD/PP_HUDWidget.h"
#include "Widgets/PP_DamageNumberWidget.h"
#include "Widgets/PP_ScoreboardWidget.h"
#include "Widgets/PP_PauseMenuWidget.h"
#include "Widgets/PP_DeathScreenWidget.h"
#include "Widgets/PP_NotificationWidget.h"
#include "Quest/PP_QuestListWidget.h"
#include "Quest/PP_QuestTrackerWidget.h"
#include "Quest/PP_QuestQuery.h"
#include "GameFramework/PlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/PP_InventoryWidget.h"
#include "Framework/PP_UIManager.h"
#include "Framework/PP_UIUserWidget.h"
#include "Inventory/PP_InventoryQuery.h"
#include "Engine/GameInstance.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "Utils/PP_BlueprintLibrary.h"

APP_HUD::APP_HUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

template<typename TWidget>
TWidget* APP_HUD::GetOrCreateWidget(TObjectPtr<TWidget>& WidgetPtr, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetPtr && WidgetClass && PlayerOwner)
	{
		WidgetPtr = CreateWidget<TWidget>(PlayerOwner, WidgetClass);
		if (WidgetPtr)
		{
			AddToViewportByLayer(WidgetPtr);
		}
	}
	return WidgetPtr;
}

template<typename TWidget>
void APP_HUD::PopAndClearWidget(TObjectPtr<TWidget>& WidgetPtr)
{
	if (WidgetPtr)
	{
		if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
		{
			Mgr->PopWidget(WidgetPtr);
		}
		WidgetPtr = nullptr;
	}
}

void APP_HUD::AddToViewportByLayer(UPP_UIUserWidget* Widget)
{
	if (Widget)
	{
		// ZOrder 唯一来源：Widget 自身的 OwningLayer（蓝图配置）
		Widget->AddToViewport(static_cast<int32>(Widget->GetOwningLayer()));
	}
}

void APP_HUD::BeginPlay()
{
	Super::BeginPlay();

	// SeamlessTravel 后从 Lobby UI-Only 模式恢复为游戏输入
	if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
	{
		Mgr->SetInputModeGameplay();
	}

	InitializeViewModel();
	CreateHUDWidgets();
	BindViewModelDelegates();

	// 订阅任务完成事件（EventBus 广播 → 通知栏显示），任务系统不再直接依赖 HUD
	if (UPP_EventBus* EventBus = UPP_EventBus::Get(this))
	{
		QuestCompletedEventHandle = EventBus->Subscribe<FPP_QuestCompletedEvent>(
			[this](const FPP_QuestCompletedEvent& Event)
			{
				OnQuestCompletedEvent(Event);
			});
	}
}

void APP_HUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 退订 EventBus 事件（HUD 生命周期短于 GameInstance，必须成对退订）
	if (UPP_EventBus* EventBus = UPP_EventBus::Get(this))
	{
		EventBus->Unsubscribe(QuestCompletedEventHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void APP_HUD::OnQuestCompletedEvent(const FPP_QuestCompletedEvent& Event)
{
	AddNotification(FString::Printf(TEXT("任务完成：%s"), *Event.QuestName));
}

void APP_HUD::InitializeViewModel()
{
	ViewModel = NewObject<UPP_UIViewModel>(this);
	if (ViewModel && PlayerOwner)
	{
		ViewModel->Initialize(PlayerOwner);
	}
}

void APP_HUD::CreateHUDWidgets()
{
	if (!PlayerOwner)
	{
		return;
	}

	// 创建战斗HUD
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UPP_HUDWidget>(PlayerOwner, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->InitializeWidget(ViewModel);
			AddToViewportByLayer(HUDWidget);
		}
	}

	// 创建通知Widget
	if (NotificationWidgetClass)
	{
		NotificationWidget = CreateWidget<UPP_NotificationWidget>(PlayerOwner, NotificationWidgetClass);
		if (NotificationWidget)
		{
			AddToViewportByLayer(NotificationWidget);

			// 强制设置通知 Widget 在视口中的锚点和位置
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NotificationWidget->Slot))
			{
				CanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
				CanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
				CanvasSlot->SetSize(FVector2D(400.f, 300.f));
			}
			else
			{
				// 非 CanvasPanelSlot 路径（如 GameViewportSubsystem），使用 SetPositionInViewport
				NotificationWidget->SetPositionInViewport(FVector2D(1420.f, 60.f), false);
			}

			NotificationWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// 计分板、暂停菜单、死亡画面改为按需创建（Show 时 CreateWidget，Hide 时 PopWidget）
	// 不再预创建隐藏 — 由 UIManager 管理完整 Push/Pop 生命周期

	// 获取任务数据源（挂载在 PlayerState 上，实现 IPP_QuestQuery 接口）；
	// 按接口能力查找组件，UI 不依赖任务组件具体类
	UObject* QuestSource = nullptr;
	if (PlayerOwner)
	{
		if (APlayerState* PS = PlayerOwner->GetPlayerState<APlayerState>())
		{
			QuestSource = UPP_BlueprintLibrary::FindComponentImplementing(PS, UPP_QuestQuery::StaticClass());
		}
	}

	// 创建任务追踪器（HUD右侧常驻显示当前追踪任务）
	if (QuestTrackerWidgetClass)
	{
		QuestTrackerWidget = CreateWidget<UPP_QuestTrackerWidget>(PlayerOwner, QuestTrackerWidgetClass);
		if (QuestTrackerWidget)
		{
			AddToViewportByLayer(QuestTrackerWidget);
			if (QuestSource)
			{
				QuestTrackerWidget->InitializeTracker(QuestSource);
			}
		}
	}

	// 创建任务日志面板（初始隐藏，通过 ToggleQuestLog 按键打开）
	// 注意：先 SetVisibility(Hidden) 再 AddToViewport —— 避免构造期自动推栈时
	// 以可见状态触发 RecalculateInputMode 误切输入模式
	if (QuestListWidgetClass)
	{
		QuestListWidget = CreateWidget<UPP_QuestListWidget>(PlayerOwner, QuestListWidgetClass);
		if (QuestListWidget)
		{
			QuestListWidget->SetVisibility(ESlateVisibility::Hidden);
			AddToViewportByLayer(QuestListWidget);
			if (QuestSource)
			{
				QuestListWidget->InitializeQuestList(QuestSource);
			}
		}
	}

	// 初始化伤害数字对象池
	if (DamageNumberWidgetClass && HUDWidget)
	{
		DamageNumberPool = NewObject<UPP_WidgetPool>(this);
		DamageNumberPool->Initialize(HUDWidget, DamageNumberWidgetClass, DamageNumberPoolSize);
	}

	// 所有 Widget 创建完毕后，强制归一为游戏输入模式
	// （批量创建期间的构造期推栈可能已按栈逻辑重算，此处确保最终状态为 Gameplay）
	if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
	{
		Mgr->SetInputModeGameplay();
	}
}

void APP_HUD::BindViewModelDelegates()
{
	if (!ViewModel)
	{
		return;
	}

	// 绑定伤害接收事件
	ViewModel->OnDamageReceived.AddDynamic(this, &APP_HUD::OnDamageReceived);

	// 绑定玩家死亡事件
	ViewModel->OnPlayerDeath.AddDynamic(this, &APP_HUD::OnPlayerDeath);

	// 绑定玩家复活事件
	ViewModel->OnPlayerRespawn.AddDynamic(this, &APP_HUD::OnPlayerRespawn);
}

void APP_HUD::ShowPauseMenu()
{
	// NativeConstruct → UIManager::PushWidget(Modal, bConsumesAllInput) → 自动 SetInputModeUI
	GetOrCreateWidget(PauseMenuWidget, PauseMenuWidgetClass);
}

void APP_HUD::HidePauseMenu()
{
	PopAndClearWidget(PauseMenuWidget);
}

bool APP_HUD::IsPauseMenuVisible() const
{
	return PauseMenuWidget != nullptr;
}

void APP_HUD::ShowScoreboard()
{
	GetOrCreateWidget(ScoreboardWidget, ScoreboardWidgetClass);
}

void APP_HUD::HideScoreboard()
{
	PopAndClearWidget(ScoreboardWidget);
}

void APP_HUD::ShowDeathScreen(float RespawnTime)
{
	if (GetOrCreateWidget(DeathScreenWidget, DeathScreenWidgetClass))
	{
		DeathScreenWidget->ShowRespawnCountdown(RespawnTime);
	}
}

void APP_HUD::HideDeathScreen()
{
	PopAndClearWidget(DeathScreenWidget);
}

void APP_HUD::ToggleMainHUD()
{
	if (bIsHUDVisible)
	{
		HideMainHUD();
	}
	else
	{
		ShowMainHUD();
	}
}

void APP_HUD::ShowMainHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Visible);
		bIsHUDVisible = true;
		// 输入模式完全由 UIManager 栈逻辑决定（HUDWidget 为 Gameplay 层，不影响输入）
	}
}

void APP_HUD::HideMainHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
		bIsHUDVisible = false;
		// 输入模式完全由 UIManager 栈逻辑决定（HUDWidget 为 Gameplay 层，不影响输入）
	}
}

void APP_HUD::SpawnDamageNumber(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical)
{
	if (!DamageNumberPool || !HUDWidget)
	{
		return;
	}

	UUserWidget* PooledWidget = DamageNumberPool->Acquire();
	if (!PooledWidget)
	{
		return;
	}

	UPP_DamageNumberWidget* DamageNumber = Cast<UPP_DamageNumberWidget>(PooledWidget);
	if (DamageNumber)
	{
		DamageNumber->Configure(Damage, WorldLocation, bIsHeal, bIsCritical);
		// 伤害数字为对象池管理的普通 UUserWidget（不进 UIManager 栈），ZOrder 贴近 Gameplay 层之上
		DamageNumber->AddToViewport(static_cast<int32>(EPP_UILayer::Gameplay) + 1);

		// 委托驱动回收：动画完成时自动归还到池，无需 Tick 轮询
		DamageNumber->OnFinished.AddDynamic(this, &APP_HUD::OnDamageNumberFinished);
	}
}

void APP_HUD::AddNotification(const FString& Message)
{
	if (NotificationWidget)
	{
		NotificationWidget->AddMessage(Message);
	}
}

void APP_HUD::OnDamageReceived(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical, AActor* SourceActor)
{
	SpawnDamageNumber(Damage, WorldLocation, bIsHeal, bIsCritical);
}

void APP_HUD::OnPlayerDeath(AActor* DeadActor)
{
	// 默认复活时间5秒，可通过GameMode配置
	ShowDeathScreen(5.f);
}

void APP_HUD::OnPlayerRespawn()
{
	HideDeathScreen();
}

void APP_HUD::OnDamageNumberFinished(UPP_DamageNumberWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	Widget->OnFinished.RemoveDynamic(this, &APP_HUD::OnDamageNumberFinished);

	if (DamageNumberPool)
	{
		DamageNumberPool->Release(Widget);
	}
}

void APP_HUD::ToggleQuestLog()
{
	if (!QuestListWidget) return;

	const bool bIsVisible = QuestListWidget->GetVisibility() == ESlateVisibility::Visible;

	if (bIsVisible)
	{
		QuestListWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		QuestListWidget->RefreshQuestList();
		QuestListWidget->SetVisibility(ESlateVisibility::Visible);
	}

	// 输入模式由 UIManager 根据栈+可见性重算（QuestListWidget 位于 Overlay 层）
	if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
	{
		Mgr->RefreshInputMode();
	}
}

void APP_HUD::ToggleInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		// 关闭背包：通过 UIManager 弹出
		if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
		{
			Mgr->PopWidget(InventoryWidget);
		}
		else
		{
			InventoryWidget->RemoveFromParent();
		}
	}
	else
	{
		// 打开背包：创建并添加到视口
		if (!InventoryWidget && InventoryWidgetClass)
		{
			InventoryWidget = CreateWidget<UPP_InventoryWidget>(PlayerOwner, InventoryWidgetClass);
		}

		if (InventoryWidget)
		{
			// 按接口能力查找库存数据源（避免依赖 UPP_InventoryComponent 具体类）
			UObject* InvSource = nullptr;
			if (PlayerOwner)
			{
				if (APlayerState* PS = PlayerOwner->GetPlayerState<APlayerState>())
				{
					InvSource = UPP_BlueprintLibrary::FindComponentImplementing(PS, UPP_InventoryQuery::StaticClass());
				}
			}

			if (InvSource)
			{
				InventoryWidget->BindInventory(InvSource);
			}

			// ZOrder 取自 Widget 自身 OwningLayer；构造期自动推栈 → RecalculateInputMode
			AddToViewportByLayer(InventoryWidget);
		}
	}
}
