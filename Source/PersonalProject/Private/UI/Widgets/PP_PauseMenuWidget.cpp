// Chen Tianbao uses GAS to create online battle games

#include "UI/Widgets/PP_PauseMenuWidget.h"

#include "UI/PP_HUD.h"
#include "Game/PP_CrashGameMode.h"
#include "Game/PP_SessionManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h"

UPP_PauseMenuWidget::UPP_PauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Modal;
	bConsumesAllInput = true;
}

void UPP_PauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 在 NativeOnInitialized 中绑定（仅调用一次，避免 NativeConstruct 的重复绑定问题）
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UPP_PauseMenuWidget::OnResumeClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UPP_PauseMenuWidget::OnQuitClicked);
	}
}

void UPP_PauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPP_PauseMenuWidget::ResumeGame()
{
	// 通过HUD隐藏暂停菜单
	if (APP_HUD* HUD = Cast<APP_HUD>(GetOwningPlayer()->GetHUD()))
	{
		HUD->HidePauseMenu();
	}
}

void UPP_PauseMenuWidget::OnResumeClicked()
{
	ResumeGame();
}

void UPP_PauseMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[PauseMenu] OnQuitClicked - Quit button pressed!"));

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 先隐藏暂停菜单，恢复输入模式
	SetVisibility(ESlateVisibility::Collapsed);
	PC->SetShowMouseCursor(false);
	PC->SetInputMode(FInputModeGameOnly());

	const ENetMode NetMode = World->GetNetMode();
	UE_LOG(LogTemp, Warning, TEXT("[PauseMenu] OnQuitClicked - NetMode: %d"), static_cast<int32>(NetMode));

	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		// Host: 使用 ServerTravel 将所有玩家带回大厅
		APP_CrashGameMode* GM = Cast<APP_CrashGameMode>(World->GetAuthGameMode());
		if (GM)
		{
			GM->ReturnToLobby();
		}
	}
	else
	{
		// Client: 断开连接返回主菜单
		UGameInstance* GI = World->GetGameInstance();
		if (GI)
		{
			UPP_SessionManager* SM = GI->GetSubsystem<UPP_SessionManager>();
			if (SM)
			{
				SM->DestroyCurrentSession();
			}
		}
		PC->ConsoleCommand(TEXT("Disconnect"));
	}
}
