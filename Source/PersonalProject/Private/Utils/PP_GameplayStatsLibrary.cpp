// Chen Tianbao uses GAS to create online battle games


#include "Utils/PP_GameplayStatsLibrary.h"

#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UPP_AttributeSet* UPP_GameplayStatsLibrary::ResolvePlayerAttributeSet(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject)) return nullptr;

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!IsValid(PC)) return nullptr;

	APawn* Pawn = PC->GetPawn();
	APlayerState* PS = PC->GetPlayerState<APlayerState>();

	// 优先从 PlayerState 取 ASC（与 UPP_UIViewModel::Initialize 一致）
	UAbilitySystemComponent* ASC = nullptr;
	if (PS)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	// PlayerState 上没有则回退到 Pawn
	if (!ASC && Pawn)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	if (!ASC) return nullptr;

	// GetAttributeSet 返回 const 指针，转成非 const 以便调用取值接口
	return const_cast<UPP_AttributeSet*>(Cast<UPP_AttributeSet>(ASC->GetAttributeSet(UPP_AttributeSet::StaticClass())));
}

float UPP_GameplayStatsLibrary::GetPlayerHealth(const UObject* WorldContextObject)
{
	const UPP_AttributeSet* AttributeSet = ResolvePlayerAttributeSet(WorldContextObject);
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float UPP_GameplayStatsLibrary::GetPlayerMaxHealth(const UObject* WorldContextObject)
{
	const UPP_AttributeSet* AttributeSet = ResolvePlayerAttributeSet(WorldContextObject);
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}
