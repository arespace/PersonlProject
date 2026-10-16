// 大厅 UI — 角色预览拖拽公共基类
// 收纳角色预览图的画刷绑定（RT）与鼠标拖拽旋转逻辑，供角色选择/大厅两个 Widget 复用

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "PP_CharacterPreviewDragWidget.generated.h"

class UImage;
class APP_LobbyHUD;

/**
 * 提供「预览图（RT 画刷）+ 鼠标拖拽旋转预览角色」能力的公共基类。
 * 子类只需继承并在 WBP 中放同名 CharacterPreviewImage 即可获得完整交互。
 */
UCLASS(Abstract)
class PERSONALPROJECTUI_API UPP_CharacterPreviewDragWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 预览图控件（绑定 RT 画刷；WBP 中放同名 Image 即可） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Preview")
	TObjectPtr<UImage> CharacterPreviewImage;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 安全获取大厅 HUD（用于取预览 RT 与旋转预览角色） */
	APP_LobbyHUD* GetLobbyHUD() const;

private:
	/** 预览图画刷是否已绑定 RT（绑定后不再每帧轮询） */
	bool bPreviewImageReady = false;

	/** 是否正在拖拽旋转预览 */
	bool bIsDraggingPreview = false;

	/** 上一帧拖拽的屏幕坐标 */
	FVector2D LastDragMousePosition;
};
