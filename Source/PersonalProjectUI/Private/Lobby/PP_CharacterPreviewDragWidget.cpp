// 大厅 UI — 角色预览拖拽公共基类实现

#include "Lobby/PP_CharacterPreviewDragWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"

void UPP_CharacterPreviewDragWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 预览图画刷已绑定且 RT 不变，无需每帧轮询
	if (bPreviewImageReady || !CharacterPreviewImage)
	{
		return;
	}

	if (APP_LobbyHUD* HUD = GetLobbyHUD())
	{
		if (UTextureRenderTarget2D* RT = HUD->GetPreviewRenderTarget())
		{
			FSlateBrush NewBrush;
			NewBrush.SetResourceObject(RT);
			NewBrush.ImageSize = FVector2D(RT->SizeX, RT->SizeY);
			CharacterPreviewImage->SetBrush(NewBrush);
			bPreviewImageReady = true;
		}
	}
}

FReply UPP_CharacterPreviewDragWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CharacterPreviewImage)
	{
		const FGeometry ImageGeometry = CharacterPreviewImage->GetCachedGeometry();
		if (ImageGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			bIsDraggingPreview = true;
			LastDragMousePosition = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return FReply::Unhandled();
}

FReply UPP_CharacterPreviewDragWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingPreview)
	{
		const FVector2D CurrentPosition = InMouseEvent.GetScreenSpacePosition();
		const FVector2D Delta = CurrentPosition - LastDragMousePosition;
		LastDragMousePosition = CurrentPosition;

		if (APP_LobbyHUD* HUD = GetLobbyHUD())
		{
			HUD->RotatePreviewCharacter(Delta.X * 0.2f, -Delta.Y * 0.2f);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UPP_CharacterPreviewDragWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingPreview)
	{
		bIsDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

APP_LobbyHUD* UPP_CharacterPreviewDragWidget::GetLobbyHUD() const
{
	return APP_LobbyHUD::GetFromWidget(this);
}
