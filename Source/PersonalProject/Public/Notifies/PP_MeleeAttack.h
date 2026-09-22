// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PP_MeleeAttack.generated.h"

UCLASS()
class PERSONALPROJECT_API UPP_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

private:
	
	UPROPERTY(EditAnywhere, Category = "Crash|Debugs")
	bool bDrawDebugs = true;

	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	FName SocketName{"FX_Trail_01_R"};

	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	float SocketExtensionOffset{40.f};

	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	float SphereTraceRadius{60.f};

	TArray<FHitResult> PerformSphereTrace(USkeletalMeshComponent* MeshComp) const;
	void SendEventsToActors(USkeletalMeshComponent* MeshComp, const TArray<FHitResult>& Hits) const;
};
