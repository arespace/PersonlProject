// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Lobby/PP_CharacterPreviewDragWidget.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "Characters/PP_CharacterData.h"
#include "PP_CharacterSelectWidget.generated.h"

class UTextBlock;

UCLASS()
class PERSONALPROJECTUI_API UPP_CharacterSelectWidget : public UPP_CharacterPreviewDragWidget
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

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> PrevCharacterButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> NextCharacterButton;

	/** 详情面板角色名（读 DT_Characters 的 DisplayName，不再暴露 RowName / 资产名） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Character")
	TObjectPtr<UTextBlock> CharacterNameText;

	/** 详情面板角色描述（读 DT_Characters 的 Description，用来填实中部区域） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Character")
	TObjectPtr<UTextBlock> CharacterDescText;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Character Selected"))
	void BP_OnCharacterSelected(FName CharacterId);

private:
	UFUNCTION() void OnConfirmClicked();
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnPrevCharacterClicked();
	UFUNCTION() void OnNextCharacterClicked();

	/** 通知 LobbyHUD 切换预览角色 */
	void NotifyPreviewUpdate(FName CharacterId);

	FName CurrentlySelected;

	/** 当前选中角色在 AvailableCharacters 中的索引 */
	int32 CurrentIndex = 0;
};
