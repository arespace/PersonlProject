// Copyright Personal Project. All Rights Reserved.

#include "Characters/PP_CharacterPreviewActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"

APP_CharacterPreviewActor::APP_CharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	PreviewPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewPivot"));
	PreviewPivot->SetupAttachment(RootComp);
	PreviewPivot->SetRelativeLocation(FVector(0.f, 0.f, -20.f));

	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(PreviewPivot);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComp);
	SceneCapture->SetRelativeLocation(FVector(0.f, 250.f, 270.f));
	SceneCapture->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->bCaptureEveryFrame = true;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->FOVAngle = 50.f;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->ShowOnlyActors.Add(this);

	PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("PreviewRT"));
	PreviewRenderTarget->RenderTargetFormat = RTF_RGBA8;
	PreviewRenderTarget->InitAutoFormat(PreviewRenderTargetSize, PreviewRenderTargetSize);
	PreviewRenderTarget->ClearColor = FLinearColor(0.05f, 0.05f, 0.1f, 1.0f);
	PreviewRenderTarget->bAutoGenerateMips = false;
	PreviewRenderTarget->UpdateResourceImmediate(false);
	SceneCapture->TextureTarget = PreviewRenderTarget;
}

void APP_CharacterPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	if (SceneCapture)
	{
		SceneCapture->SetRelativeLocation(FVector(0.f, 250.f, 70.f));
		SceneCapture->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		SceneCapture->FOVAngle = 50.f;
		SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		SceneCapture->bCaptureEveryFrame = true;
		SceneCapture->bCaptureOnMovement = false;
		SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		SceneCapture->ShowOnlyActors.Empty();
		SceneCapture->ShowOnlyActors.Add(this);
		SceneCapture->TextureTarget = PreviewRenderTarget;
	}
}

void APP_CharacterPreviewActor::SetPreviewCharacter(const FCharacterDataRow& CharacterData)
{
	if (!PreviewMesh) return;

	USkeletalMesh* Mesh = CharacterData.CharacterMesh.LoadSynchronous();
	if (!Mesh) return;

	PreviewMesh->SetSkeletalMesh(Mesh);
	PreviewMesh->SetVisibility(true);
	PreviewMesh->SetHiddenInGame(false);
	PreviewMesh->bVisibleInSceneCaptureOnly = false;
	PreviewMesh->bHiddenInSceneCapture = false;
	PreviewMesh->MarkRenderStateDirty();

	if (UClass* AnimClass = CharacterData.AnimBlueprintClass.LoadSynchronous())
	{
		PreviewMesh->SetAnimInstanceClass(AnimClass);
	}
}

void APP_CharacterPreviewActor::RotatePreview(float YawDelta, float /*PitchDelta*/)
{
	if (!PreviewPivot) return;
	FRotator CurrentRotation = PreviewPivot->GetRelativeRotation();
	CurrentRotation.Yaw += YawDelta;
	CurrentRotation.Pitch = 0.f;
	PreviewPivot->SetRelativeRotation(CurrentRotation);
}
