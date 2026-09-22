// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "PP_CharacterSelectWidget.generated.h"

class UDataTable;
class APP_LobbyHUD;

UCLASS()
class PERSONALPROJECT_API UPP_CharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FName GetSelectedCharacter() const { return CurrentlySelected; }

	/** 蓝图调用：选择角色（由角色按鈕点击时调用） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Character")
	void SetSelectedCharacter(FName CharacterId);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UGridPanel> CharacterGrid;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Character")
	TArray<FName> AvailableCharacters;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Character")
	TSubclassOf<UUserWidget> CharacterEntryClass;

	/** 角色数据表引用（可选；有效时动态填充 AvailableCharacters） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Character")
	TObjectPtr<UDataTable> CharacterDataTable;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> PrevCharacterButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> NextCharacterButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UImage> CharacterPreviewImage;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Character Selected"))
	void BP_OnCharacterSelected(FName CharacterId);

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION() void OnConfirmClicked();
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnPrevCharacterClicked();
	UFUNCTION() void OnNextCharacterClicked();

	/** 通知 LobbyHUD 切换预览角色 */
	void NotifyPreviewUpdate(FName CharacterId);

	/** 安全获取 LobbyHUD */
	APP_LobbyHUD* GetLobbyHUD() const;

	FName CurrentlySelected;

	/** 当前选中角色在 AvailableCharacters 中的索引 */
	int32 CurrentIndex = 0;

	bool bIsDraggingPreview = false;
	FVector2D LastDragMousePosition;
	bool bPreviewImageReady = false;
};
