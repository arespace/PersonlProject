// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Framework/PP_LobbyTypes.h"
#include "PP_RoomSettingsWidget.generated.h"

UCLASS()
class PERSONALPROJECTUI_API UPP_RoomSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplySettings(const FRoomSettings& Settings);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UComboBoxString> MapSelector;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<USpinBox> MaxPlayersSpinBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UComboBoxString> GameModeSelector;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION() void OnApplyClicked();
	UFUNCTION() void OnCloseClicked();
};
