// Fill out your copyright notice in the Description page of Project Settings.


#include "Service/LuaWorldService.h"

bool ULuaWorldService::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void ULuaWorldService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LuaInit();
}

void ULuaWorldService::PostInitialize()
{
	Super::PostInitialize();
	LuaPostInit();
}

void ULuaWorldService::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	LuaOnWorldBeginPlay(&InWorld);
}

void ULuaWorldService::Deinitialize()
{
	LuaDeinit();
	Super::Deinitialize();
}

UUserWidget* ULuaWorldService::GetWidgetByPath(FString Path)
{
	UClass* WidgetClass = LoadObject<UClass>(nullptr, *Path);
	if (WidgetClass) {
		UUserWidget* Widget = NewObject<UUserWidget>(GetWorld(), WidgetClass);
		return Widget;
	}
	return nullptr;
}

//ULevelStreamingDynamic* ULuaWorldService::LoadLevelByPath(FString Path, FVector Pos, FRotator Rotator)
//{
//	UWorld* World = GetWorld();
//	bool Ret;
//	ULevelStreamingDynamic* level = ULevelStreamingDynamic::LoadLevelInstance(World, Path, Pos, Rotator, Ret, "");
//	return level;
//}
