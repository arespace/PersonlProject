// Chen Tianbao uses GAS to create online battle games

#include "UI/PP_HUD.h"

#include "UI/PP_UIViewModel.h"
#include "UI/PP_WidgetPool.h"
#include "UI/PP_HUDWidget.h"
#include "UI/Widgets/PP_DamageNumberWidget.h"
#include "UI/Widgets/PP_ScoreboardWidget.h"
#include "UI/Widgets/PP_PauseMenuWidget.h"
#include "UI/Widgets/PP_DeathScreenWidget.h"
#include "UI/Widgets/PP_NotificationWidget.h"
#include "UI/Quest/PP_QuestListWidget.h"
#include "UI/Quest/PP_QuestTrackerWidget.h"
#include "Quest/Component/PP_QuestComponent.h"
#include "Player/PP_PlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "UI/Inventory/PP_InventoryWidget.h"
#include "UI/PP_UIManager.h"
#include "Inventory/Component/PP_InventoryComponent.h"
#include "Engine/GameInstance.h"

APP_HUD::APP_HUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APP_HUD::BeginPlay()
{
	Super::BeginPlay();

	// SeamlessTravel 后从 Lobby UI-Only 模式恢复为游戏输入
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
		{
			Mgr->SetInputModeGameplay();
		}
	}

	InitializeViewModel();
	CreateHUDWidgets();
	BindViewModelDelegates();
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
			HUDWidget->AddToViewport(ZOrder_CombatHUD);
		}
	}

	// 创建通知Widget
	if (NotificationWidgetClass)
	{
		NotificationWidget = CreateWidget<UPP_NotificationWidget>(PlayerOwner, NotificationWidgetClass);
		if (NotificationWidget)
		{
			NotificationWidget->AddToViewport(ZOrder_Notification);

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

			// TODO: 测试通知显示，验证通过后删除
			NotificationWidget->AddMessage(TEXT("测试通知消息"));
		}
	}

	// 计分板、暂停菜单、死亡画面改为按需创建（Show 时 CreateWidget，Hide 时 PopWidget）
	// 不再预创建隐藏 — 由 UIManager 管理完整 Push/Pop 生命周期

	// 获取 QuestComponent（挂载在 PlayerState 上）
	UPP_QuestComponent* QC = nullptr;
	APP_PlayerState* PS = PlayerOwner ? PlayerOwner->GetPlayerState<APP_PlayerState>() : nullptr;
	if (PS)
	{
		QC = PS->GetQuestComponent();
	}

	// 创建任务追踪器（HUD右侧常驻显示当前追踪任务）
	if (QuestTrackerWidgetClass)
	{
		QuestTrackerWidget = CreateWidget<UPP_QuestTrackerWidget>(PlayerOwner, QuestTrackerWidgetClass);
		if (QuestTrackerWidget)
		{
			QuestTrackerWidget->AddToViewport(ZOrder_CombatHUD + 1);
			if (QC)
			{
				QuestTrackerWidget->InitializeTracker(QC);
			}
		}
	}

	// 创建任务日志面板（初始隐藏，通过 ToggleQuestLog 按键打开）
	if (QuestListWidgetClass)
	{
		QuestListWidget = CreateWidget<UPP_QuestListWidget>(PlayerOwner, QuestListWidgetClass);
		if (QuestListWidget)
		{
			QuestListWidget->AddToViewport(ZOrder_Scoreboard);
			QuestListWidget->SetVisibility(ESlateVisibility::Hidden);
			if (QC)
			{
				QuestListWidget->InitializeQuestList(QC);
			}
		}
	}

	// 初始化伤害数字对象池
	if (DamageNumberWidgetClass && HUDWidget)
	{
		DamageNumberPool = NewObject<UPP_WidgetPool>(this);
		DamageNumberPool->Initialize(HUDWidget, DamageNumberWidgetClass, DamageNumberPoolSize);
	}

	// 所有 Widget 创建完毕、可见性已最终确定后，强制恢复游戏输入模式
	// 防止 AddToViewport 期间 NativeConstruct→PushWidget→RecalculateInputMode
	// 在 SetVisibility(Hidden) 之前误切到 UI 模式
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
		{
			Mgr->SetInputModeGameplay();
		}
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
	if (!PauseMenuWidget && PauseMenuWidgetClass && PlayerOwner)
	{
		PauseMenuWidget = CreateWidget<UPP_PauseMenuWidget>(PlayerOwner, PauseMenuWidgetClass);
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport(ZOrder_PauseMenu);
			// NativeConstruct → UIManager::PushWidget(Modal, bConsumesAllInput) → 自动 SetInputModeUI
		}
	}
}

void APP_HUD::HidePauseMenu()
{
	if (PauseMenuWidget)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->PopWidget(PauseMenuWidget);
			}
		}
		PauseMenuWidget = nullptr;
	}
}

bool APP_HUD::IsPauseMenuVisible() const
{
	return PauseMenuWidget != nullptr;
}

void APP_HUD::ShowScoreboard()
{
	if (!ScoreboardWidget && ScoreboardWidgetClass && PlayerOwner)
	{
		ScoreboardWidget = CreateWidget<UPP_ScoreboardWidget>(PlayerOwner, ScoreboardWidgetClass);
		if (ScoreboardWidget)
		{
			ScoreboardWidget->AddToViewport(ZOrder_Scoreboard);
		}
	}
}

void APP_HUD::HideScoreboard()
{
	if (ScoreboardWidget)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->PopWidget(ScoreboardWidget);
			}
		}
		ScoreboardWidget = nullptr;
	}
}

void APP_HUD::ShowDeathScreen(float RespawnTime)
{
	if (!DeathScreenWidget && DeathScreenWidgetClass && PlayerOwner)
	{
		DeathScreenWidget = CreateWidget<UPP_DeathScreenWidget>(PlayerOwner, DeathScreenWidgetClass);
		if (DeathScreenWidget)
		{
			DeathScreenWidget->AddToViewport(ZOrder_DeathScreen);
		}
	}
	if (DeathScreenWidget)
	{
		DeathScreenWidget->ShowRespawnCountdown(RespawnTime);
	}
}

void APP_HUD::HideDeathScreen()
{
	if (DeathScreenWidget)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->PopWidget(DeathScreenWidget);
			}
		}
		DeathScreenWidget = nullptr;
	}
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
		// 输入模式由 UIManager 根据当前栈顶 bConsumesAllInput 自动计算
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->SetInputModeGameplay();
			}
		}
	}
}

void APP_HUD::HideMainHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
		bIsHUDVisible = false;
		// 输入模式恢复游戏
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->SetInputModeGameplay();
			}
		}
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
		DamageNumber->AddToViewport(ZOrder_CombatHUD + 1);

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
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->SetInputModeGameplay();
			}
		}
	}
	else
	{
		QuestListWidget->RefreshQuestList();
		QuestListWidget->SetVisibility(ESlateVisibility::Visible);
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->SetInputModeGameAndUI();
			}
		}
	}
}

void APP_HUD::ToggleInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		// 关闭背包：通过 UIManager 弹出
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->PopWidget(InventoryWidget);
			}
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
			// 绑定库存组件
			UPP_InventoryComponent* InvComp = nullptr;
			if (PlayerOwner)
			{
				APP_PlayerState* PS = PlayerOwner->GetPlayerState<APP_PlayerState>();
				if (PS)
				{
					InvComp = PS->GetInventoryComponent();
				}
			}

			if (InvComp)
			{
				InventoryWidget->BindInventory(InvComp);
			}

			InventoryWidget->AddToViewport(static_cast<int32>(EPP_UILayer::Overlay));
		}
	}
}
