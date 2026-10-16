// Copyright Personal Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Characters/PP_CharacterData.h"
#include "PP_CharacterPreviewActor.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * 角色预览Actor - 在大厅角色选择界面中展示3D角色模型
 * 由 LobbyHUD 动态 Spawn，SceneCapture2D 渲染到 RenderTarget 供 Widget 显示
 */
UCLASS()
class PERSONALPROJECTCORE_API APP_CharacterPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	APP_CharacterPreviewActor();
	virtual void BeginPlay() override;

	/** 切换展示的角色模型和动画 */
	UFUNCTION(BlueprintCallable, Category = "PP|Preview")
	void SetPreviewCharacter(const FCharacterDataRow& CharacterData);

	/** 获取预览用RenderTarget（供Widget UImage直接显示） */
	UFUNCTION(BlueprintCallable, Category = "PP|Preview")
	UTextureRenderTarget2D* GetPreviewRenderTarget() const { return PreviewRenderTarget; }

	/** 旋转预览视角（拖拽用） */
	UFUNCTION(BlueprintCallable, Category = "PP|Preview")
	void RotatePreview(float YawDelta, float PitchDelta);

protected:
	/** 旋转轴点 - 角色和拖拽旋转围绕此点 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Preview")
	TObjectPtr<USceneComponent> PreviewPivot;

	/** 角色骨骼网格体组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;

	/** 场景捕获组件 — 渲染到RenderTarget供Widget显示 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Preview")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	/** 预览用渲染目标（C++自动创建，也可在BP中覆盖） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Preview")
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	/** 自动创建的RT分辨率 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Preview")
	int32 PreviewRenderTargetSize = 512;
};
