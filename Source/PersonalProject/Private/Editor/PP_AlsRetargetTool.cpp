// 编辑器工具：把 ALS 的混合空间和分层动画蓝图批量复制到 Kwang 骨架，
// 并按名字把内部的动画资产引用换成重定向后的资产。
// 用法：在编辑器控制台执行  PP.RetargetAlsAnimBP

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimNode_LinkedAnimGraph.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/Skeleton.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/IAssetRegistry.h"
#endif

namespace PPAlsRetargetTool
{
#if WITH_EDITOR

// 重定向产物的存放目录（与批量导出时填的 Export Path 一致）
static const FString DestFolder = TEXT("/Game/PersonalProject/Animations/AlsKwang");

// 批量导出时给动画资产加的后缀（A_Als_Walk_Forward -> A_Als_Walk_ForwardKwang）
static const FString RetargetSuffix = TEXT("Kwang");

// 目标骨架资产
static const FString TargetSkeletonObjectPath = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang_Skeleton.Kwang_Skeleton");

// 混合空间：批量导出窗口不包含这种类型，需要先复制再换内部采样动画
static const TArray<FString> SourceBlendSpaces =
{
	TEXT("/ALS/ALS/Animations/Air/Lean/BS_Als_Air_Lean"),
	TEXT("/ALS/ALS/Animations/Grounded/Lean/BS_Als_Lean"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Backward"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Forward"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Left_Backward"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Left_Forward"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Right_Backward"),
	TEXT("/ALS/ALS/Animations/Grounded/WalkRun/BS_Als_WalkRun_Right_Forward"),
	TEXT("/ALS/ALS/Animations/View/BS_Als_Look")
};

// 需要处理的分层动画蓝图（源资产是曼尼昆骨架）
static const TArray<FString> SourceAnimBlueprints =
{
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_Locomotion"),
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_Grounded"),
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_Layering"),
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_Head"),
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_Ragdolling"),
	TEXT("/ALS/ALS/Character/AnimationInstances/AB_Als_View"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Stances/AB_Als_Standing"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Stances/AB_Als_Crouching"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Default"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Injured"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Feminine"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Masculine"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Rifle"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Bow"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_PistolOneHanded"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_PistolTwoHanded"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Torch"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Binoculars"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Barrel"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Box"),
	TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_HandsTied")
};

// 记录重定向目录里找不到对应资产的原始名字，跑完后统一打印
static TSet<FString> MissingAnimations;

// 把一条 ALS 原始动画资产换成重定向后的同名资产；不是 ALS 资产或已经换过则返回 nullptr
static UAnimationAsset* FindRetargetedAnimation(UObject* Original)
{
	if (!Original || !Original->IsA(UAnimationAsset::StaticClass()))
	{
		return nullptr;
	}

	// 只处理 ALS 插件目录下的原始资产，避免把已经替换过的资产再套一层后缀
	if (!Original->GetPathName().StartsWith(TEXT("/ALS/")))
	{
		return nullptr;
	}

	const FString NewName = Original->GetName() + RetargetSuffix;
	const FString NewObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestFolder, *NewName, *NewName);

	UAnimationAsset* NewAsset = LoadObject<UAnimationAsset>(nullptr, *NewObjectPath);
	if (!NewAsset)
	{
		MissingAnimations.Add(Original->GetName());
	}
	return NewAsset;
}

// 递归遍历结构体（动画节点的 Node、混合空间的采样数据都是结构体），替换里面的动画资产引用
static int32 ReplaceAssetsInStruct(const UStruct* Struct, void* StructAddress)
{
	int32 ReplacedCount = 0;

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			ReplacedCount += ReplaceAssetsInStruct(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress));
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));

			// 对象数组（例如 PoseAsset 之类）
			if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					if (UAnimationAsset* NewAsset = FindRetargetedAnimation(InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index))))
					{
						InnerObjectProperty->SetObjectPropertyValue(Helper.GetRawPtr(Index), NewAsset);
						++ReplacedCount;
					}
				}
			}
			// 结构体数组（混合空间的 SampleData 就是这种）
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					ReplacedCount += ReplaceAssetsInStruct(InnerStructProperty->Struct, Helper.GetRawPtr(Index));
				}
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			void* ValueAddress = ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress);
			if (UAnimationAsset* NewAsset = FindRetargetedAnimation(ObjectProperty->GetObjectPropertyValue(ValueAddress)))
			{
				ObjectProperty->SetObjectPropertyValue(ValueAddress, NewAsset);
				++ReplacedCount;
			}
		}
	}

	return ReplacedCount;
}

// 处理单个资产：复制到目标目录 + 切换骨架 + 重映射内部所有动画引用 + 重新编译（仅蓝图）
static void ProcessAsset(const FString& SourcePackagePath, IAssetTools& AssetTools, UObject* TargetSkeleton)
{
	const FString SourceName = FPackageName::GetShortName(SourcePackagePath);
	const FString NewName = SourceName + RetargetSuffix;
	const FString NewObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestFolder, *NewName, *NewName);

	// 已存在就直接复用，方便反复执行
	UObject* NewAsset = LoadObject<UObject>(nullptr, *NewObjectPath);
	if (!NewAsset)
	{
		UObject* SourceAsset = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), *SourcePackagePath, *SourceName));
		if (!SourceAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到源资产：%s"), *SourcePackagePath);
			return;
		}

		NewAsset = AssetTools.DuplicateAsset(NewName, DestFolder, SourceAsset);
		if (!NewAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 复制失败：%s"), *NewName);
			return;
		}
	}

	// 1) 切换目标骨架（只有动画蓝图带 TargetSkeleton 属性，混合空间的骨架由采样动画推导）
	if (FObjectProperty* SkeletonProperty = FindFProperty<FObjectProperty>(NewAsset->GetClass(), TEXT("TargetSkeleton")))
	{
		SkeletonProperty->SetObjectPropertyValue_InContainer(NewAsset, TargetSkeleton);
	}
	else if (NewAsset->IsA(UBlueprint::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] %s 上找不到 TargetSkeleton 属性"), *NewName);
	}

	// 2) 需要处理的资产：资产本体 + 蓝图包内的全部图节点（状态机子图也是包内对象）
	TArray<UObject*> ObjectsToProcess;
	ObjectsToProcess.Add(NewAsset);

	if (NewAsset->IsA(UBlueprint::StaticClass()))
	{
		ForEachObjectWithPackage(NewAsset->GetOutermost(), [&ObjectsToProcess](UObject* Object)
		{
			if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
			{
				ObjectsToProcess.Add(GraphNode);
			}
			return true;
		}, true);
	}

	// 3) 替换引用。替换过程会加载新资产，所以先收集好对象再统一处理
	NewAsset->Modify();
	int32 ReplacedCount = 0;
	for (UObject* Object : ObjectsToProcess)
	{
		ReplacedCount += ReplaceAssetsInStruct(Object->GetClass(), Object);
	}

	// 4) 动画蓝图重新编译；混合空间不需要
	if (UBlueprint* Blueprint = Cast<UBlueprint>(NewAsset))
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	NewAsset->MarkPackageDirty();
	NewAsset->GetOutermost()->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] %s 完成：替换动画引用 %d 处"), *NewName, ReplacedCount);
}

static void Run()
{
	UObject* TargetSkeleton = LoadObject<UObject>(nullptr, *TargetSkeletonObjectPath);
	if (!TargetSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到目标骨架：%s"), *TargetSkeletonObjectPath);
		return;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	MissingAnimations.Reset();

	// 顺序很重要：混合空间要先就位，动画蓝图里的混合空间引用才能被替换
	for (const FString& SourcePackagePath : SourceBlendSpaces)
	{
		ProcessAsset(SourcePackagePath, AssetTools, TargetSkeleton);
	}

	for (const FString& SourcePackagePath : SourceAnimBlueprints)
	{
		ProcessAsset(SourcePackagePath, AssetTools, TargetSkeleton);
	}

	if (MissingAnimations.Num() > 0)
	{
		const FString MissingList = FString::Join(MissingAnimations.Array(), TEXT(", "));
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 以下 %d 个动画在 %s 里找不到对应重定向资产：%s"), MissingAnimations.Num(), *DestFolder, *MissingList);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 全部完成。请执行 File > Save All 把新资产落盘。"));
}

// 二次修复：批量导出生成的动画/蒙太奇不在上面的处理清单里，它们内部可能残留曼尼昆骨架的
// 原始动画引用（最典型的是加性动画的 Base Pose = RefPoseSeq、蒙太奇片段、SequenceEvaluator）。
// 这些引用会和 Kwang 骨架混用，运行时表现为「身体拉长/压扁」。
// 用法：在编辑器控制台执行  PP.FixAlsKwangRefs
static void FixExistingReferences()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	MissingAnimations.Reset();

	int32 ProcessedAssets = 0;
	int32 ReplacedTotal = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		// 只处理动画类资产和动画蓝图，IK Rig 等无关资产跳过
		if (!Asset->IsA(UAnimationAsset::StaticClass()) && !Asset->IsA(UBlueprint::StaticClass()))
		{
			continue;
		}

		TArray<UObject*> ObjectsToProcess;
		ObjectsToProcess.Add(Asset);

		// 蓝图（含 AB_Als_Kwang 主 AB）需要扫包内全部对象：
		// 动画节点 struct 的引用在图节点上，而事件图里「Play Slot Animation as Dynamic Montage」
		// 的转身序列引用是引脚默认值（UEdGraphPin::DefaultObject）或蓝图变量（存在 CDO 上），
		// 只收 UEdGraphNode 会漏掉它们，导致转身蒙太奇仍播曼尼昆原版动画（躯干拉长）。
		if (Asset->IsA(UBlueprint::StaticClass()))
		{
			ForEachObjectWithPackage(Asset->GetOutermost(), [&ObjectsToProcess](UObject* Object)
			{
				ObjectsToProcess.Add(Object);
				return true;
			}, true);
		}

		Asset->Modify();
		int32 ReplacedCount = 0;
		for (UObject* Object : ObjectsToProcess)
		{
			ReplacedCount += ReplaceAssetsInStruct(Object->GetClass(), Object);
		}

		if (ReplacedCount > 0)
		{
			// 动画蓝图改过引用必须重编译，否则生成的类里仍是旧引用
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
			{
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
			}

			Asset->MarkPackageDirty();
			Asset->GetOutermost()->MarkPackageDirty();
			++ProcessedAssets;
			ReplacedTotal += ReplacedCount;

			UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 修复 %s：替换 %d 处残留动画引用"), *Asset->GetName(), ReplacedCount);
		}

		// 加性动画的差值是在「当前姿势 - Base Pose」基础上算出来的（RefPoseType 为
		// Selected Animation Frame 时按帧采样 RefPoseSeq）。Base Pose 之前指向曼尼昆骨架，
		// 引擎加载时会直接把这个引用判为无效，加性差值就按整段绝对姿势来算，
		// 表现为转动视角/跑动时躯干被拉长压扁、脖子看不见。
		// 压缩数据的键里包含 Base Pose 的 GUID，换基准后键会变，这里主动按新键重建一次，
		// 避免继续使用旧的烘焙结果。
		if (UAnimSequence* Sequence = Cast<UAnimSequence>(Asset))
		{
			if (Sequence->IsValidAdditive())
			{
				Sequence->CacheDerivedDataForCurrentPlatform();
			}
		}
	}

	if (MissingAnimations.Num() > 0)
	{
		const FString MissingList = FString::Join(MissingAnimations.Array(), TEXT(", "));
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 以下 %d 个 ALS 动画在 %s 里没有对应的 Kwang 资产，需要补导出：%s"),
			MissingAnimations.Num(), *DestFolder, *MissingList);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 引用修复完成：处理 %d 个资产，共替换 %d 处。请执行 File > Save All 落盘。"),
		ProcessedAssets, ReplacedTotal);
}

// 诊断：找出工程里到底哪些资产还在引用曼尼昆原版转身序列。
// 背景：PP.FixAlsKwangRefs 只扫 AlsKwang 目录资产包内的对象，但执行后壳蓝图 AB_Als_Kwang
// 0 处替换、躯干依旧拉长，说明残留引用不在这些包里。最可能藏在：
//   1) 壳蓝图若是插件 AB_Als 的子类，事件图里「Play Slot Animation as Dynamic Montage」
//      的序列引脚默认值存在父类的包（插件目录）里，子包扫描看不到；
//   2) 角色蓝图/其他资产直接引用了原版序列；
//   3) 以软引用（TSoftObjectPtr）形式持有，硬引用扫描漏掉。
// 这里改用资产注册表反查引用者，再逐对象报告到具体属性。只报告，不修改。
// 用法：在编辑器控制台执行  PP.FindTurnRefs

// 递归报告指向目标资产集合的引用（硬引用按指针比对，软引用按路径字符串比对）
static void DumpTurnRefsInStruct(const UStruct* Struct, void* StructAddress, const UObject* OwnerObject,
	const FString& PropertyPath, const TSet<const UObject*>& TargetAssets, const TSet<FString>& TargetSoftPaths, int32& FoundCount)
{
	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		const FString CurrentPath = PropertyPath.IsEmpty() ? Property->GetName() : FString::Printf(TEXT("%s.%s"), *PropertyPath, *Property->GetName());

		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			DumpTurnRefsInStruct(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress),
				OwnerObject, CurrentPath, TargetAssets, TargetSoftPaths, FoundCount);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (const FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					if (const UObject* Value = InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)))
					{
						if (TargetAssets.Contains(Value))
						{
							UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] %s | %s[%d] -> %s"), *OwnerObject->GetPathName(), *CurrentPath, Index, *Value->GetPathName());
							++FoundCount;
						}
					}
				}
			}
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					DumpTurnRefsInStruct(InnerStructProperty->Struct, Helper.GetRawPtr(Index), OwnerObject,
						FString::Printf(TEXT("%s[%d]"), *CurrentPath, Index), TargetAssets, TargetSoftPaths, FoundCount);
				}
			}
		}
		else if (FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
		{
			// 注意顺序：FSoftObjectProperty 派生自 FObjectPropertyBase，必须先判断软引用
			const FString ValuePath = SoftProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(StructAddress)->ToString();
			if (TargetSoftPaths.Contains(ValuePath))
			{
				UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] %s | %s -> (软引用) %s"), *OwnerObject->GetPathName(), *CurrentPath, *ValuePath);
				++FoundCount;
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			if (const UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(StructAddress))
			{
				if (TargetAssets.Contains(Value))
				{
					UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] %s | %s -> %s"), *OwnerObject->GetPathName(), *CurrentPath, *Value->GetPathName());
					++FoundCount;
				}
			}
		}
	}
}

static void FindTurnSequenceRefs()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// 1) 收集插件里的原版转身序列（按名字前缀，避免硬编码路径漏掉子目录）
	FARFilter Filter;
	Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(TEXT("/ALS")));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> SequenceAssets;
	AssetRegistry.GetAssets(Filter, SequenceAssets);

	TSet<const UObject*> TargetAssets;
	TSet<FString> TargetSoftPaths;
	TArray<FName> TargetPackageNames;
	for (const FAssetData& AssetData : SequenceAssets)
	{
		const FString AssetName = AssetData.AssetName.ToString();
		if (!AssetName.StartsWith(TEXT("A_Als_Turn_")) && !AssetName.StartsWith(TEXT("A_Als_Crouch_Turn_")))
		{
			continue;
		}

		if (const UObject* Asset = AssetData.GetAsset())
		{
			TargetAssets.Add(Asset);
		}
		TargetSoftPaths.Add(AssetData.GetObjectPathString());
		TargetPackageNames.AddUnique(AssetData.PackageName);
	}
	UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] 插件原版转身序列 %d 个"), TargetPackageNames.Num());

	// 2) 资产注册表反查：哪些包还依赖这些序列（含硬/软引用）
	TSet<FName> ReferencerPackages;
	for (const FName& PackageName : TargetPackageNames)
	{
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(PackageName, Referencers, UE::AssetRegistry::EDependencyCategory::All);
		for (const FName& Referencer : Referencers)
		{
			if (!TargetPackageNames.Contains(Referencer))
			{
				ReferencerPackages.Add(Referencer);
			}
		}
	}
	for (const FName& Referencer : ReferencerPackages)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] 引用者包：%s"), *Referencer.ToString());
	}

	// 3) 逐包逐对象报告具体引用位置（哪个对象、哪条属性、指向哪个序列）
	int32 FoundCount = 0;
	for (const FName& Referencer : ReferencerPackages)
	{
		UPackage* Package = LoadPackage(nullptr, *Referencer.ToString(), LOAD_None);
		if (!Package)
		{
			continue;
		}

		TArray<UObject*> ObjectsInPackage;
		ForEachObjectWithPackage(Package, [&ObjectsInPackage](UObject* Object)
		{
			ObjectsInPackage.Add(Object);
			return true;
		}, true);

		for (UObject* Object : ObjectsInPackage)
		{
			DumpTurnRefsInStruct(Object->GetClass(), Object, Object, TEXT(""), TargetAssets, TargetSoftPaths, FoundCount);
		}
	}

	// 4) 打印壳蓝图类继承链：若父类是插件 AB_Als_C，说明事件图（含转身序列引脚）在父类包里
	if (UBlueprint* ShellBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/PersonalProject/Animations/AlsKwang/AB_Als_Kwang.AB_Als_Kwang")))
	{
		for (const UClass* Class = ShellBP->GeneratedClass; Class; Class = Class->GetSuperClass())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] AB_Als_Kwang 类链：%s（包 %s）"), *Class->GetPathName(), *Class->GetOutermost()->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[TurnRefs] 完成：共发现 %d 处残留引用"), FoundCount);
}

// 把对象里所有指向 OldAsset 的对象引用（含结构体/数组嵌套）换成 NewAsset。
// 用于把蓝图类默认值里的动画实例设置资产切到 Kwang 副本。
static int32 ReplaceObjectRefInStruct(const UStruct* Struct, void* StructAddress, const UObject* OldAsset, UObject* NewAsset)
{
	int32 ReplacedCount = 0;

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			ReplacedCount += ReplaceObjectRefInStruct(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), OldAsset, NewAsset);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					if (InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)) == OldAsset)
					{
						InnerObjectProperty->SetObjectPropertyValue(Helper.GetRawPtr(Index), NewAsset);
						++ReplacedCount;
					}
				}
			}
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					ReplacedCount += ReplaceObjectRefInStruct(InnerStructProperty->Struct, Helper.GetRawPtr(Index), OldAsset, NewAsset);
				}
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			void* ValueAddress = ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress);
			if (ObjectProperty->GetObjectPropertyValue(ValueAddress) == OldAsset)
			{
				ObjectProperty->SetObjectPropertyValue(ValueAddress, NewAsset);
				++ReplacedCount;
			}
		}
	}

	return ReplacedCount;
}

// 修复躯干拉长：PP.FindTurnRefs 已定位残留引用在插件数据资产 AIS_Als_Default 里——
// 它的 AlsTurnInPlaceSettings_0~7 子对象的 Sequence 仍指向曼尼昆原版转身序列，
// C++ UAlsAnimationInstance 转身时从该资产取序列往 TurnInPlaceStanding 槽播动态蒙太奇，
// 曼尼昆比例混入 Kwang 骨架就是躯干拉长的来源（蓝图包里没有任何引用，扫蓝图永远找不到）。
// 这里复制一份 AIS_Als_Kwang、把 8 处序列换成重定向版，再把 AlsKwang 目录里所有
// 蓝图（主要是 AB_Als_Kwang 的类默认值 Settings）对该资产的引用切到新资产。
// 用法：在编辑器控制台执行  PP.FixTurnRefs，然后 File > Save All
static void FixTurnSequenceRefs()
{
	const FString SourceObjectPath = TEXT("/ALS/ALS/Data/AnimationInstance/AIS_Als_Default.AIS_Als_Default");
	UObject* SourceSettings = LoadObject<UObject>(nullptr, *SourceObjectPath);
	if (!SourceSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("[FixTurnRefs] 找不到源设置资产：%s"), *SourceObjectPath);
		return;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// 1) 复制设置资产到 AlsKwang（已存在则复用，方便反复执行）
	const FString NewName = TEXT("AIS_Als_Kwang");
	const FString NewObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestFolder, *NewName, *NewName);
	UObject* NewSettings = LoadObject<UObject>(nullptr, *NewObjectPath);
	if (!NewSettings)
	{
		NewSettings = AssetTools.DuplicateAsset(NewName, DestFolder, SourceSettings);
		if (!NewSettings)
		{
			UE_LOG(LogTemp, Error, TEXT("[FixTurnRefs] 复制失败：%s"), *NewName);
			return;
		}
	}

	// 2) 替换新资产包内所有 /ALS/ 动画引用（AlsTurnInPlaceSettings_* 子对象的 Sequence）
	MissingAnimations.Reset();
	TArray<UObject*> ObjectsToProcess;
	ForEachObjectWithPackage(NewSettings->GetOutermost(), [&ObjectsToProcess](UObject* Object)
	{
		ObjectsToProcess.Add(Object);
		return true;
	}, true);

	NewSettings->Modify();
	int32 ReplacedCount = 0;
	for (UObject* Object : ObjectsToProcess)
	{
		ReplacedCount += ReplaceAssetsInStruct(Object->GetClass(), Object);
	}
	NewSettings->MarkPackageDirty();
	NewSettings->GetOutermost()->MarkPackageDirty();
	UE_LOG(LogTemp, Warning, TEXT("[FixTurnRefs] %s：替换动画引用 %d 处"), *NewName, ReplacedCount);

	if (MissingAnimations.Num() > 0)
	{
		const FString MissingList = FString::Join(MissingAnimations.Array(), TEXT(", "));
		UE_LOG(LogTemp, Error, TEXT("[FixTurnRefs] 以下 %d 个 ALS 动画在 %s 里没有对应的 Kwang 资产，需要补导出：%s"),
			MissingAnimations.Num(), *DestFolder, *MissingList);
	}

	// 3) 把 AlsKwang 目录里所有蓝图包内指向旧设置资产的引用换成新资产（类默认值 Settings 在 CDO 上）
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset || !Asset->IsA(UBlueprint::StaticClass()))
		{
			continue;
		}

		TArray<UObject*> BlueprintObjects;
		ForEachObjectWithPackage(Asset->GetOutermost(), [&BlueprintObjects](UObject* Object)
		{
			BlueprintObjects.Add(Object);
			return true;
		}, true);

		Asset->Modify();
		int32 SettingsReplaced = 0;
		for (UObject* Object : BlueprintObjects)
		{
			SettingsReplaced += ReplaceObjectRefInStruct(Object->GetClass(), Object, SourceSettings, NewSettings);
		}

		if (SettingsReplaced > 0)
		{
			FKismetEditorUtilities::CompileBlueprint(CastChecked<UBlueprint>(Asset));
			Asset->MarkPackageDirty();
			Asset->GetOutermost()->MarkPackageDirty();
			UE_LOG(LogTemp, Warning, TEXT("[FixTurnRefs] %s：切换设置资产引用 %d 处"), *Asset->GetName(), SettingsReplaced);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[FixTurnRefs] 完成。请执行 File > Save All 落盘。"));
}

// 收尾清理：BP_PP_PlayerCharacter 的 CharacterMesh0 上残留 Paragon 模板时代的默认值
// （SKM_Als 网格 + Greystone_AnimBlueprint_C 动画类，骨架不匹配的脏组合）。
// 运行时构造函数 bForceAlsAnimationBlueprint=true 会强制套 AB_Als_C、RestoreCharacterSelection
// 再按数据表换外观，所以这些默认值永远不生效，但序列化在资产里是脏数据。
// 这里把默认值改成一致的 SKM_Als + AB_Als_C。
// 用法：在编辑器控制台执行  PP.CleanPlayerBPDefaults，然后 File > Save All
static void CleanPlayerBPDefaults()
{
	const FString BlueprintObjectPath = TEXT("/Game/PersonalProject/Characters/PlayerCharacter/BP_PP_PlayerCharacter.BP_PP_PlayerCharacter");
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintObjectPath);
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[CleanPlayerBP] 找不到蓝图：%s"), *BlueprintObjectPath);
		return;
	}

	ACharacter* CDO = Cast<ACharacter>(Blueprint->GeneratedClass->GetDefaultObject());
	USkeletalMeshComponent* MeshComponent = CDO ? CDO->GetMesh() : nullptr;
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[CleanPlayerBP] CDO 上没有 CharacterMesh0 组件"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CleanPlayerBP] 当前默认值：网格=%s 动画类=%s"),
		*GetNameSafe(MeshComponent->GetSkeletalMeshAsset()),
		*GetNameSafe(MeshComponent->GetAnimClass()));

	USkeletalMesh* AlsMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
	UClass* AlsAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
	if (!AlsMesh || !AlsAnimClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[CleanPlayerBP] 加载 ALS 默认资产失败"));
		return;
	}

	CDO->Modify();
	MeshComponent->Modify();
	MeshComponent->SetSkeletalMeshAsset(AlsMesh);
	MeshComponent->SetAnimInstanceClass(AlsAnimClass);

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("[CleanPlayerBP] 已把默认值改为 SKM_Als + AB_Als_C。请 File > Save All 落盘。"));
}

static FAutoConsoleCommand RetargetAlsAnimBPCommand(
	TEXT("PP.RetargetAlsAnimBP"),
	TEXT("把 ALS 混合空间和分层动画蓝图复制到 Kwang 骨架，并重映射所有动画引用"),
	FConsoleCommandDelegate::CreateStatic(&Run)
);

static FAutoConsoleCommand FixAlsKwangRefsCommand(
	TEXT("PP.FixAlsKwangRefs"),
	TEXT("扫描 AlsKwang 目录下全部资产，把残留的曼尼昆动画引用换成对应的 Kwang 重定向资产"),
	FConsoleCommandDelegate::CreateStatic(&FixExistingReferences)
);

static FAutoConsoleCommand FindTurnRefsCommand(
	TEXT("PP.FindTurnRefs"),
	TEXT("反查曼尼昆原版转身序列的全部引用者并报告具体属性位置（只报告不修改）"),
	FConsoleCommandDelegate::CreateStatic(&FindTurnSequenceRefs)
);

static FAutoConsoleCommand FixTurnRefsCommand(
	TEXT("PP.FixTurnRefs"),
	TEXT("复制 AIS_Als_Default 为 AIS_Als_Kwang 并把转身序列换成 Kwang 重定向版，同时切换 AlsKwang 蓝图对该资产的引用"),
	FConsoleCommandDelegate::CreateStatic(&FixTurnSequenceRefs)
);

static FAutoConsoleCommand CleanPlayerBPDefaultsCommand(
	TEXT("PP.CleanPlayerBPDefaults"),
	TEXT("把 BP_PP_PlayerCharacter 的 CharacterMesh0 遗留默认值（Greystone 动画类）改成一致的 SKM_Als + AB_Als_C"),
	FConsoleCommandDelegate::CreateStatic(&CleanPlayerBPDefaults)
);

// 诊断：检查重定向动画里躯干骨骼的位移是否仍来自曼尼昆骨架。
// 用法：PP.CheckAlsKwangBones
static void CheckTorsoTranslations()
{
	const TArray<FString> AssetPaths =
	{
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_Stand_PoseKwang"),
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_IdleKwang"),
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_Run_ForwardKwang"),
		TEXT("/ALS/ALS/Animations/Base/A_Als_Stand_Pose"),
		TEXT("/ALS/ALS/Animations/Base/A_Als_Idle")
	};

	const TArray<FName> Bones =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	for (const FString& AssetPath : AssetPaths)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *AssetPath, *FPackageName::GetShortName(AssetPath));
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *ObjectPath);
		if (!Sequence)
		{
			UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到动画：%s"), *AssetPath);
			continue;
		}

		const USkeleton* Skeleton = Sequence->GetSkeleton();
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] === %s  Skeleton=%s"), *Sequence->GetName(), *GetNameSafe(Skeleton));

		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Skeleton || !Model)
		{
			UE_LOG(LogTemp, Error, TEXT("[RetargetTool] %s 缺少骨架或数据模型"), *Sequence->GetName());
			continue;
		}

		const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();

		// 把 pelvis 以上每根骨骼的局部位移长度累加起来，就是躯干的实际长度。
		// 如果重定向资产的这个值接近曼尼昆原版动画、而不是接近 Kwang 骨架的参考值，
		// 说明重定向时把源骨架的位移（骨骼长度）一起搬过来了。
		double AnimTorsoLength = 0.0;
		double RefTorsoLength = 0.0;

		for (const FName& BoneName : Bones)
		{
			const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			if (BoneIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RetargetTool]    %s：骨架里没有这根骨骼"), *BoneName.ToString());
				continue;
			}

			const FVector RefTranslation = ReferenceSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();

			FVector AnimTranslation = RefTranslation;
			bool bHasKey = false;
			Model->IterateBoneKeys(BoneName, [&AnimTranslation, &bHasKey](const FVector3f& Position, const FQuat4f&, const FVector3f, const FFrameNumber&)
			{
				AnimTranslation = FVector(Position);
				bHasKey = true;
				return false;
			});

			UE_LOG(LogTemp, Warning, TEXT("[RetargetTool]    %s  动画=(%.2f, %.2f, %.2f)  参考=(%.2f, %.2f, %.2f)  差=%.2fcm%s"),
				*BoneName.ToString(),
				AnimTranslation.X, AnimTranslation.Y, AnimTranslation.Z,
				RefTranslation.X, RefTranslation.Y, RefTranslation.Z,
				FVector::Dist(AnimTranslation, RefTranslation),
				bHasKey ? TEXT("") : TEXT(" (无骨骼轨道)"));

			if (BoneName != FName("pelvis"))
			{
				AnimTorsoLength += AnimTranslation.Size();
				RefTorsoLength += RefTranslation.Size();
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool]    躯干链合计（spine_01..head）：动画=%.2fcm  骨架参考=%.2fcm  差=%.2fcm"),
			AnimTorsoLength, RefTorsoLength, AnimTorsoLength - RefTorsoLength);
	}
}

static FAutoConsoleCommand CheckAlsKwangBonesCommand(
	TEXT("PP.CheckAlsKwangBones"),
	TEXT("打印重定向动画里躯干骨骼的位移，判断是否仍混用了曼尼昆骨架的比例"),
	FConsoleCommandDelegate::CreateStatic(&CheckTorsoTranslations)
);

// 递归遍历结构体，把「引用刚好等于 FromAsset」的对象属性换成 ToAsset。
// 和 ReplaceAssetsInStruct 的区别：这里只认一个确定的旧资产，不做名字推导。
static int32 ReplaceSpecificAssetInStruct(const UStruct* Struct, void* StructAddress, UObject* FromAsset, UObject* ToAsset)
{
	int32 ReplacedCount = 0;

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			ReplacedCount += ReplaceSpecificAssetInStruct(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), FromAsset, ToAsset);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));

			if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				if (!FromAsset->IsA(InnerObjectProperty->PropertyClass))
				{
					continue;
				}

				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					if (InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)) == FromAsset)
					{
						InnerObjectProperty->SetObjectPropertyValue(Helper.GetRawPtr(Index), ToAsset);
						++ReplacedCount;
					}
				}
			}
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					ReplacedCount += ReplaceSpecificAssetInStruct(InnerStructProperty->Struct, Helper.GetRawPtr(Index), FromAsset, ToAsset);
				}
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			if (!FromAsset->IsA(ObjectProperty->PropertyClass))
			{
				continue;
			}

			void* ValueAddress = ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress);
			if (ObjectProperty->GetObjectPropertyValue(ValueAddress) == FromAsset)
			{
				ObjectProperty->SetObjectPropertyValue(ValueAddress, ToAsset);
				++ReplacedCount;
			}
		}
	}

	return ReplacedCount;
}

// 撤销上一版「整目录替换」：把 AlsKwang 里所有指向 Paragon Idle 的引用还原回
// A_Als_Stand_PoseKwang。上一版把 22 处加性动画的 Base Pose（RefPoseSeq）也换了，
// 加性差值在运行时是「当前动画姿势 - Base Pose」，换基准等于把所有 Lean / Look /
// Stop / Turn / Land 的差值整体改掉，姿势会直接崩。
// 用法：在编辑器控制台执行  PP.RestoreKwangStandPose
static void RestoreKwangStandPose()
{
	const FString FromObjectPath = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Idle.Idle");
	const FString ToObjectPath = DestFolder + TEXT("/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang");

	UAnimSequence* FromAsset = LoadObject<UAnimSequence>(nullptr, *FromObjectPath);
	UAnimSequence* ToAsset = LoadObject<UAnimSequence>(nullptr, *ToObjectPath);
	if (!FromAsset || !ToAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 加载失败：%s / %s"), *FromObjectPath, *ToObjectPath);
		return;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	int32 ProcessedAssets = 0;
	int32 ReplacedTotal = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		if (!Asset->IsA(UAnimationAsset::StaticClass()) && !Asset->IsA(UBlueprint::StaticClass()))
		{
			continue;
		}

		TArray<UObject*> ObjectsToProcess;
		ObjectsToProcess.Add(Asset);

		// 蓝图的图节点（含状态机子图）都在同一个包里，需要一起扫
		if (Asset->IsA(UBlueprint::StaticClass()))
		{
			ForEachObjectWithPackage(Asset->GetOutermost(), [&ObjectsToProcess](UObject* Object)
			{
				if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
				{
					ObjectsToProcess.Add(GraphNode);
				}
				return true;
			}, true);
		}

		Asset->Modify();
		int32 ReplacedCount = 0;
		for (UObject* Object : ObjectsToProcess)
		{
			ReplacedCount += ReplaceSpecificAssetInStruct(Object->GetClass(), Object, FromAsset, ToAsset);
		}

		if (ReplacedCount > 0)
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
			{
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
			}

			Asset->MarkPackageDirty();
			Asset->GetOutermost()->MarkPackageDirty();
			++ProcessedAssets;
			ReplacedTotal += ReplacedCount;

			UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 还原 %s：%d 处"), *Asset->GetName(), ReplacedCount);
		}

		// 加性动画的 Base Pose 复位后重新烘焙一次
		if (UAnimSequence* Sequence = Cast<UAnimSequence>(Asset))
		{
			if (Sequence->IsValidAdditive())
			{
				Sequence->CacheDerivedDataForCurrentPlatform();
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 还原完成：处理 %d 个资产，共 %d 处。请执行 File > Save All 落盘。"),
		ProcessedAssets, ReplacedTotal);
}

// 只把站立层（AB_Als_StandingKwang）里播放站姿的那一处引用换成 Kwang 自带的 Idle。
// 加性动画的 Base Pose 一律不动——那是叠加计算基准，动了就崩。
// 用法：在编辑器控制台执行  PP.UseKwangIdlePose
static void UseKwangIdlePose()
{
	const FString FromObjectPath = DestFolder + TEXT("/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang");
	const FString ToObjectPath = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Idle.Idle");

	UAnimSequence* FromAsset = LoadObject<UAnimSequence>(nullptr, *FromObjectPath);
	UAnimSequence* ToAsset = LoadObject<UAnimSequence>(nullptr, *ToObjectPath);
	if (!FromAsset || !ToAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 加载失败：%s / %s"), *FromObjectPath, *ToObjectPath);
		return;
	}

	const FString StandingObjectPath = DestFolder + TEXT("/AB_Als_StandingKwang.AB_Als_StandingKwang");
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *StandingObjectPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到站立层：%s"), *StandingObjectPath);
		return;
	}

	TArray<UObject*> ObjectsToProcess;
	ObjectsToProcess.Add(Blueprint);
	ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
	{
		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
		{
			ObjectsToProcess.Add(GraphNode);
		}
		return true;
	}, true);

	Blueprint->Modify();
	int32 ReplacedCount = 0;
	for (UObject* Object : ObjectsToProcess)
	{
		ReplacedCount += ReplaceSpecificAssetInStruct(Object->GetClass(), Object, FromAsset, ToAsset);
	}

	if (ReplacedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 站立层里没有找到站姿引用，未做修改"));
		return;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	Blueprint->GetOutermost()->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 站立层站姿引用替换 %d 处（只动 AB_Als_StandingKwang）。请执行 File > Save All 落盘。"),
		ReplacedCount);
}

static FAutoConsoleCommand RestoreKwangStandPoseCommand(
	TEXT("PP.RestoreKwangStandPose"),
	TEXT("撤销整目录站姿替换：把 AlsKwang 里的 Paragon Idle 引用还原回 A_Als_Stand_PoseKwang"),
	FConsoleCommandDelegate::CreateStatic(&RestoreKwangStandPose)
);

static FAutoConsoleCommand UseKwangIdlePoseCommand(
	TEXT("PP.UseKwangIdlePose"),
	TEXT("只把站立层里播放的站姿换成 Kwang 自带的 Idle（不动加性动画的 Base Pose）"),
	FConsoleCommandDelegate::CreateStatic(&UseKwangIdlePose)
);

// 运行时诊断：PIE 里执行，检查当前姿势里各骨骼的「实测长度」和骨架参考长度的比例。
// 比例明显 ≠ 1.000 才说明网格真的被拉伸了；如果全是 1.000，那视角变化时的怪异
// 只是加性层（Look/Lean）给出的大幅旋转，不是骨骼拉长。
// 用法：在游戏内控制台执行  PP.CheckLiveTorsoBones
static void CheckLiveTorsoBones()
{
	const TArray<FName> Bones =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
		if (!Mesh || !Mesh->GetSkeletalMeshAsset())
		{
			continue;
		}

		const USkeleton* Skeleton = Mesh->GetSkeletalMeshAsset()->GetSkeleton();
		if (!Skeleton)
		{
			continue;
		}

		const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();

		UE_LOG(LogTemp, Warning, TEXT("[TorsoCheck] 网格=%s  动画类=%s  组件缩放=%s"),
			*Mesh->GetSkeletalMeshAsset()->GetName(),
			*GetNameSafe(Mesh->GetAnimInstance() ? Mesh->GetAnimInstance()->GetClass() : nullptr),
			*Mesh->GetComponentScale().ToString());

		for (const FName& BoneName : Bones)
		{
			const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			if (BoneIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("[TorsoCheck]    %s：骨架里没有这根骨骼"), *BoneName.ToString());
				continue;
			}

			const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
			if (ParentIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("[TorsoCheck]    %s：根骨骼，跳过"), *BoneName.ToString());
				continue;
			}

			const float RefLength = static_cast<float>(ReferenceSkeleton.GetRefBonePose()[BoneIndex].GetTranslation().Size());
			const FVector BoneLocation = Mesh->GetBoneLocation(BoneName, EBoneSpaces::WorldSpace);
			const FVector ParentLocation = Mesh->GetBoneLocation(ReferenceSkeleton.GetBoneName(ParentIndex), EBoneSpaces::WorldSpace);
			const float LiveLength = static_cast<float>(FVector::Dist(BoneLocation, ParentLocation));
			const float Ratio = RefLength > KINDA_SMALL_NUMBER ? LiveLength / RefLength : 1.0f;

			UE_LOG(LogTemp, Warning, TEXT("[TorsoCheck]    %s  参考=%.2fcm  实测=%.2fcm  比例=%.3f%s"),
				*BoneName.ToString(), RefLength, LiveLength, Ratio,
				FMath::Abs(Ratio - 1.0f) > 0.02f ? TEXT("  <== 真的被拉伸") : TEXT(""));
		}
	}
}

static FAutoConsoleCommand CheckLiveTorsoBonesCommand(
	TEXT("PP.CheckLiveTorsoBones"),
	TEXT("PIE 中检查当前姿势里躯干骨骼的实测长度比例，判断视角变化时是否真的被拉伸"),
	FConsoleCommandDelegate::CreateStatic(&CheckLiveTorsoBones)
);

// 和 ReplaceAssetsInStruct 同样的遍历方式，但只收集被引用的动画资产，不做替换
static void CollectAnimAssets(const UStruct* Struct, void* StructAddress, TArray<UObject*>& OutAssets)
{
	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectAnimAssets(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), OutAssets);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));

			if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					if (UObject* Object = InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)))
					{
						OutAssets.Add(Object);
					}
				}
			}
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					CollectAnimAssets(InnerStructProperty->Struct, Helper.GetRawPtr(Index), OutAssets);
				}
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			if (UObject* Object = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress)))
			{
				OutAssets.Add(Object);
			}
		}
	}
}

// 诊断：找出 AlsKwang 里「骨架不是 Kwang」或「躯干位移偏离 Kwang 参考姿势」的动画，
// 以及动画蓝图里仍然指向曼尼昆骨架动画的节点。PIE 里实测到的 16.56cm 脖子、
// 10.84/19.23/13.38 的脊柱长度正好是曼尼昆（SK_Als）的参考值，说明这些数据还在被播放。
// 用法：在编辑器控制台执行  PP.CheckKwangAnimData
static void CheckKwangAnimData()
{
	USkeleton* TargetSkeleton = LoadObject<USkeleton>(nullptr, *TargetSkeletonObjectPath);
	if (!TargetSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimData] 找不到目标骨架：%s"), *TargetSkeletonObjectPath);
		return;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	const TArray<FName> Bones =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	const FReferenceSkeleton& ReferenceSkeleton = TargetSkeleton->GetReferenceSkeleton();

	int32 SequenceCount = 0;
	int32 BadSkeletonCount = 0;
	int32 BadTranslationCount = 0;
	int32 BadBlueprintRefCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		// 1) 动画资产：骨架对不对 + 躯干位移是否偏离 Kwang 参考姿势
		if (UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(Asset))
		{
			++SequenceCount;

			const USkeleton* AssetSkeleton = AnimAsset->GetSkeleton();
			if (AssetSkeleton != TargetSkeleton)
			{
				++BadSkeletonCount;
				UE_LOG(LogTemp, Warning, TEXT("[AnimData] %s 骨架=%s（不是 Kwang）"),
					*AnimAsset->GetName(), *GetNameSafe(AssetSkeleton));
				continue;
			}

			if (UAnimSequence* Sequence = Cast<UAnimSequence>(AnimAsset))
			{
				const IAnimationDataModel* Model = Sequence->GetDataModel();
				if (!Model)
				{
					continue;
				}

				float WorstDiff = 0.f;
				FName WorstBone = NAME_None;

				for (const FName& BoneName : Bones)
				{
					const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
					if (BoneIndex == INDEX_NONE)
					{
						continue;
					}

					const FVector RefTranslation = ReferenceSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();
					FVector AnimTranslation = RefTranslation;
					bool bHasKey = false;

					Model->IterateBoneKeys(BoneName, [&AnimTranslation, &bHasKey](const FVector3f& Position, const FQuat4f&, const FVector3f, const FFrameNumber&)
					{
						AnimTranslation = FVector(Position);
						bHasKey = true;
						return false;
					});

					if (!bHasKey)
					{
						continue;
					}

					const float Diff = static_cast<float>(FVector::Dist(AnimTranslation, RefTranslation));
					if (Diff > WorstDiff)
					{
						WorstDiff = Diff;
						WorstBone = BoneName;
					}
				}

				if (WorstDiff > 1.0f)
				{
					++BadTranslationCount;
					UE_LOG(LogTemp, Warning, TEXT("[AnimData] %s 加性=%d 位移偏离参考 %.1fcm（最差骨骼 %s）"),
						*Sequence->GetName(), Sequence->IsValidAdditive() ? 1 : 0, WorstDiff, *WorstBone.ToString());
				}
			}
		}
		// 2) 动画蓝图：里面是否还残留曼尼昆骨架的动画引用
		else if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
		{
			TArray<UObject*> ObjectsToProcess;
			ObjectsToProcess.Add(Blueprint);
			ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
			{
				if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
				{
					ObjectsToProcess.Add(GraphNode);
				}
				return true;
			}, true);

			TArray<UObject*> ReferencedAssets;
			for (UObject* Object : ObjectsToProcess)
			{
				CollectAnimAssets(Object->GetClass(), Object, ReferencedAssets);
			}

			for (UObject* ReferencedAsset : ReferencedAssets)
			{
				UAnimationAsset* ReferencedAnim = Cast<UAnimationAsset>(ReferencedAsset);
				if (!ReferencedAnim || ReferencedAnim->GetSkeleton() == TargetSkeleton)
				{
					continue;
				}

				++BadBlueprintRefCount;
				UE_LOG(LogTemp, Warning, TEXT("[AnimData] %s 仍引用 %s（骨架 %s）"),
					*Blueprint->GetName(), *ReferencedAnim->GetPathName(), *GetNameSafe(ReferencedAnim->GetSkeleton()));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AnimData] 共 %d 个动画：骨架不对 %d 个，位移偏离参考 %d 个，蓝图残留曼尼昆引用 %d 处"),
		SequenceCount, BadSkeletonCount, BadTranslationCount, BadBlueprintRefCount);
}

static FAutoConsoleCommand CheckKwangAnimDataCommand(
	TEXT("PP.CheckKwangAnimData"),
	TEXT("扫描 AlsKwang 里骨架不对 / 躯干位移偏离参考的动画，以及蓝图里残留的曼尼昆引用"),
	FConsoleCommandDelegate::CreateStatic(&CheckKwangAnimData)
);

// 修复：9 个 BS_*Kwang 混合空间仍然是曼尼昆骨架（Skeleton 属性是 SK_Als，采样动画也还指向
// /ALS/ 的原始资产）。混合空间没有 TargetSkeleton 属性，必须先换 Skeleton 再换采样动画，
// 顺序反了引擎的采样校验会因为骨架不匹配把采样判非。
// 用法：在编辑器控制台执行  PP.FixKwangBlendSpaces
static void FixKwangBlendSpaces()
{
	USkeleton* TargetSkeleton = LoadObject<USkeleton>(nullptr, *TargetSkeletonObjectPath);
	if (!TargetSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[BlendSpace] 找不到目标骨架：%s"), *TargetSkeletonObjectPath);
		return;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	MissingAnimations.Reset();

	int32 FixedCount = 0;
	int32 ReplacedTotal = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset || (!Asset->IsA(UBlendSpace::StaticClass()) && !Asset->IsA(UBlendSpace1D::StaticClass())))
		{
			continue;
		}

		UAnimationAsset* BlendSpace = Cast<UAnimationAsset>(Asset);
		if (!BlendSpace)
		{
			continue;
		}

		BlendSpace->Modify();

		// 1) 先把混合空间本身的骨架换成 Kwang
		if (FObjectProperty* SkeletonProperty = FindFProperty<FObjectProperty>(BlendSpace->GetClass(), TEXT("Skeleton")))
		{
			SkeletonProperty->SetObjectPropertyValue_InContainer(BlendSpace, TargetSkeleton);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[BlendSpace] %s 上找不到 Skeleton 属性"), *BlendSpace->GetName());
		}

		// 2) 再把采样动画换成同名重定向资产
		const int32 Replaced = ReplaceAssetsInStruct(BlendSpace->GetClass(), BlendSpace);

		BlendSpace->MarkPackageDirty();
		++FixedCount;
		ReplacedTotal += Replaced;

		UE_LOG(LogTemp, Warning, TEXT("[BlendSpace] %s：替换采样动画 %d 处，当前骨架=%s"),
			*BlendSpace->GetName(), Replaced, *GetNameSafe(BlendSpace->GetSkeleton()));
	}

	if (MissingAnimations.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BlendSpace] 以下 %d 个采样动画在 %s 里没有同名重定向资产：%s"),
			MissingAnimations.Num(), *DestFolder, *FString::Join(MissingAnimations.Array(), TEXT(", ")));
	}

	UE_LOG(LogTemp, Warning, TEXT("[BlendSpace] 完成：处理 %d 个混合空间，共替换 %d 处。请执行 File > Save All 落盘。"),
		FixedCount, ReplacedTotal);
}

static FAutoConsoleCommand FixKwangBlendSpacesCommand(
	TEXT("PP.FixKwangBlendSpaces"),
	TEXT("把 9 个混合空间的骨架换成 Kwang，并把采样动画换成同名重定向资产"),
	FConsoleCommandDelegate::CreateStatic(&FixKwangBlendSpaces)
);

// 递归遍历结构体，把「指向曼尼昆动画蓝图生成类」的引用换成对应的 Kwang 版本。
// 之前所有替换函数只处理 UAnimationAsset，TSubclassOf<UAnimInstance> 这种「类引用」全被漏掉了：
// 壳 AB_Als_Kwang 里的联动节点如果还指向 AB_Als_Standing，站立层就会播放 /ALS/ 的
// A_Als_Stand_Pose（曼尼昆数据），再二次重定向到 Kwang 骨架——位移按 Animation 模式原样保留，
// 躯干长度就还是曼尼昆的（实测 10.84/19.24/13.38/16.56/9.29）。
static int32 ReplaceAnimInstanceClasses(const UStruct* Struct, void* StructAddress, const FString& ObjectLabel)
{
	int32 ReplacedCount = 0;

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			ReplacedCount += ReplaceAnimInstanceClasses(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), ObjectLabel);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));

			if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					UClass* ValueClass = Cast<UClass>(InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
					if (!ValueClass || !ValueClass->GetName().StartsWith(TEXT("AB_Als_")) || ValueClass->GetName().Contains(TEXT("Kwang")))
					{
						continue;
					}

					const FString BaseName = ValueClass->GetName().LeftChop(2); // 去掉末尾的 _C
					const FString NewAssetName = BaseName + RetargetSuffix;
					UObject* NewClass = LoadObject<UObject>(nullptr,
						*FString::Printf(TEXT("%s/%s.%s_C"), *DestFolder, *NewAssetName, *NewAssetName));
					if (NewClass)
					{
						InnerObjectProperty->SetObjectPropertyValue(Helper.GetRawPtr(Index), NewClass);
						UE_LOG(LogTemp, Warning, TEXT("[Layers] %s.%s[%d]：%s → %s"),
							*ObjectLabel, *Property->GetName(), Index, *ValueClass->GetName(), *NewClass->GetName());
						++ReplacedCount;
					}
					else
					{
						MissingAnimations.Add(NewAssetName);
					}
				}
			}
			else if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					ReplacedCount += ReplaceAnimInstanceClasses(InnerStructProperty->Struct, Helper.GetRawPtr(Index), ObjectLabel);
				}
			}
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			void* ValueAddress = ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress);
			UClass* ValueClass = Cast<UClass>(ObjectProperty->GetObjectPropertyValue(ValueAddress));
			if (!ValueClass || !ValueClass->GetName().StartsWith(TEXT("AB_Als_")) || ValueClass->GetName().Contains(TEXT("Kwang")))
			{
				continue;
			}

			const FString BaseName = ValueClass->GetName().LeftChop(2); // 去掉末尾的 _C
			const FString NewAssetName = BaseName + RetargetSuffix;
			UObject* NewClass = LoadObject<UObject>(nullptr,
				*FString::Printf(TEXT("%s/%s.%s_C"), *DestFolder, *NewAssetName, *NewAssetName));
			if (NewClass)
			{
				ObjectProperty->SetObjectPropertyValue(ValueAddress, NewClass);
				UE_LOG(LogTemp, Warning, TEXT("[Layers] %s.%s：%s → %s"),
					*ObjectLabel, *Property->GetName(), *ValueClass->GetName(), *NewClass->GetName());
				++ReplacedCount;
			}
			else
			{
				MissingAnimations.Add(NewAssetName);
			}
		}
	}

	return ReplacedCount;
}

// 修复 AlsKwang 目录里**全部** AB_Als_* 蓝图的联动节点「实例类」。
// 只修壳不够：壳指向 AB_Als_LocomotionKwang，但 LocomotionKwang 内部再往下的
// Grounded / Standing 联动节点还是曼尼昆类，运行期就是它们把 A_Als_Stand_Pose 播出来的
// （DumpKwangPose 里同时出现的 AB_Als_Standing_C / AB_Als_Grounded_C 就是证据）。
// 用法：在编辑器控制台执行  PP.FixAlsKwangLayers，然后 File > Save All
static void FixAlsKwangLayers()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	MissingAnimations.Reset();

	int32 ScannedBlueprintCount = 0;
	int32 FixedBlueprintCount = 0;
	int32 ReplacedTotal = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset || !Asset->IsA(UBlueprint::StaticClass()))
		{
			continue;
		}

		const FString BlueprintName = Asset->GetName();
		if (!BlueprintName.StartsWith(TEXT("AB_Als_")))
		{
			continue;
		}

		++ScannedBlueprintCount;
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);

		TArray<UObject*> ObjectsToProcess;
		ObjectsToProcess.Add(Blueprint);
		ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
		{
			if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
			{
				ObjectsToProcess.Add(GraphNode);
			}
			return true;
		}, true);

		Blueprint->Modify();
		int32 ReplacedCount = 0;
		for (UObject* Object : ObjectsToProcess)
		{
			ReplacedCount += ReplaceAnimInstanceClasses(Object->GetClass(), Object, Object->GetName());
		}

		// 图节点引脚上的默认值不在 UPROPERTY 遍历范围内（UEdGraphNode::Pins 没有 UPROPERTY 标记，
		// 见 EdGraphNode.h），所以 LinkAnimClassLayers 之类把联动类存成引脚默认对象的节点，
		// 上面的属性遍历完全看不到。这里单独扫一遍引脚。
		for (UObject* Object : ObjectsToProcess)
		{
			UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object);
			if (!GraphNode)
			{
				continue;
			}

			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (!Pin)
				{
					continue;
				}

				UClass* PinClass = Cast<UClass>(Pin->DefaultObject);
				if (!PinClass || !PinClass->GetName().StartsWith(TEXT("AB_Als_")) || PinClass->GetName().Contains(TEXT("Kwang")))
				{
					continue;
				}

				const FString BaseName = PinClass->GetName().LeftChop(2); // 去掉末尾的 _C
				const FString NewAssetName = BaseName + RetargetSuffix;
				UObject* NewClass = LoadObject<UObject>(nullptr,
					*FString::Printf(TEXT("%s/%s.%s_C"), *DestFolder, *NewAssetName, *NewAssetName));
				if (NewClass)
				{
					Pin->DefaultObject = NewClass;
					++ReplacedCount;
					UE_LOG(LogTemp, Warning, TEXT("[Layers] 引脚 %s.%s：%s → %s"),
						*GraphNode->GetName(), *Pin->PinName.ToString(), *PinClass->GetName(), *NewClass->GetName());
				}
				else
				{
					MissingAnimations.Add(NewAssetName);
				}
			}
		}

		if (ReplacedCount > 0)
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			Blueprint->MarkPackageDirty();
			Blueprint->GetOutermost()->MarkPackageDirty();
			++FixedBlueprintCount;
			ReplacedTotal += ReplacedCount;

			UE_LOG(LogTemp, Warning, TEXT("[Layers] %s 替换 %d 处，已重新编译"), *BlueprintName, ReplacedCount);
		}
	}

	if (MissingAnimations.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Layers] 以下 %d 个子蓝图在 %s 里找不到：%s"),
			MissingAnimations.Num(), *DestFolder, *FString::Join(MissingAnimations.Array(), TEXT(", ")));
	}

	UE_LOG(LogTemp, Warning, TEXT("[Layers] 扫描 %d 个 AB_Als_* 蓝图，%d 个有改动，累计替换 %d 处。"),
		ScannedBlueprintCount, FixedBlueprintCount, ReplacedTotal);
}

static FAutoConsoleCommand FixAlsKwangLayersCommand(
	TEXT("PP.FixAlsKwangLayers"),
	TEXT("把 AlsKwang 目录里全部 AB_Als_* 蓝图联动节点的实例类从 AB_Als_* 改成 AB_Als_*Kwang"),
	FConsoleCommandDelegate::CreateStatic(&FixAlsKwangLayers)
);

// 运行时定位：当前姿势的躯干长度到底对应哪个动画资产。
// 遍历内存里所有已加载的动画，用首帧骨骼位移长度和实测长度比对，打印最接近的几个。
// 用法：PIE 里执行  PP.FindActivePoseSource
static void FindActivePoseSource()
{
	const TArray<FName> BoneNames =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	USkeletalMeshComponent* Mesh = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		Mesh = Character ? Character->GetMesh() : nullptr;
		if (Mesh)
		{
			break;
		}
	}

	if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Mesh->GetSkeletalMeshAsset()->GetSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("[PoseSource] 没找到带骨架的角色网格"));
		return;
	}

	const FReferenceSkeleton& MeshReferenceSkeleton = Mesh->GetSkeletalMeshAsset()->GetSkeleton()->GetReferenceSkeleton();

	TArray<float> MeasuredLengths;
	MeasuredLengths.Reserve(BoneNames.Num());

	for (const FName& BoneName : BoneNames)
	{
		const int32 BoneIndex = MeshReferenceSkeleton.FindBoneIndex(BoneName);
		const int32 ParentIndex = BoneIndex != INDEX_NONE ? MeshReferenceSkeleton.GetParentIndex(BoneIndex) : INDEX_NONE;
		if (ParentIndex == INDEX_NONE)
		{
			MeasuredLengths.Add(-1.f);
			continue;
		}

		const FVector BoneLocation = Mesh->GetBoneLocation(BoneName, EBoneSpaces::WorldSpace);
		const FVector ParentLocation = Mesh->GetBoneLocation(MeshReferenceSkeleton.GetBoneName(ParentIndex), EBoneSpaces::WorldSpace);
		MeasuredLengths.Add(static_cast<float>(FVector::Dist(BoneLocation, ParentLocation)));
	}

	UE_LOG(LogTemp, Warning, TEXT("[PoseSource] 实测：pelvis=%.2f spine_01=%.2f spine_02=%.2f spine_03=%.2f neck_01=%.2f head=%.2f"),
		MeasuredLengths[0], MeasuredLengths[1], MeasuredLengths[2], MeasuredLengths[3], MeasuredLengths[4], MeasuredLengths[5]);

	int32 Printed = 0;

	for (TObjectIterator<UAnimSequence> It; It; ++It)
	{
		UAnimSequence* Sequence = *It;
		const USkeleton* Skeleton = Sequence->GetSkeleton();
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Skeleton || !Model)
		{
			continue;
		}

		const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();

		float TotalDiff = 0.f;
		bool bComparable = true;

		for (int32 BoneIndex = 0; BoneIndex < BoneNames.Num(); ++BoneIndex)
		{
			const int32 SkeletonBoneIndex = ReferenceSkeleton.FindBoneIndex(BoneNames[BoneIndex]);
			if (SkeletonBoneIndex == INDEX_NONE)
			{
				bComparable = false;
				break;
			}

			FVector Translation = ReferenceSkeleton.GetRefBonePose()[SkeletonBoneIndex].GetTranslation();
			Model->IterateBoneKeys(BoneNames[BoneIndex], [&Translation](const FVector3f& Position, const FQuat4f&, const FVector3f, const FFrameNumber&)
			{
				Translation = FVector(Position);
				return false;
			});

			TotalDiff += FMath::Abs(static_cast<float>(Translation.Size()) - MeasuredLengths[BoneIndex]);
		}

		if (!bComparable)
		{
			continue;
		}

		const float AverageDiff = TotalDiff / BoneNames.Num();
		if (AverageDiff < 2.5f && Printed < 15)
		{
			++Printed;
			UE_LOG(LogTemp, Warning, TEXT("[PoseSource] 接近 %.2fcm：%s  骨架=%s  加性=%d"),
				AverageDiff, *Sequence->GetPathName(), *GetNameSafe(Skeleton), Sequence->IsValidAdditive() ? 1 : 0);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[PoseSource] 匹配结束，共列出 %d 个候选"), Printed);
}

static FAutoConsoleCommand FindActivePoseSourceCommand(
	TEXT("PP.FindActivePoseSource"),
	TEXT("PIE 中把当前躯干长度和内存里所有动画的首帧位移长度比对，找出姿势来源"),
	FConsoleCommandDelegate::CreateStatic(&FindActivePoseSource)
);

// 从动画的源数据模型里取某根骨骼首帧的局部位移（磁盘上的数据，不含压缩）
static FVector GetSequenceFrame0Translation(UAnimSequence* Sequence, FName BoneName)
{
	FVector Result = FVector::ZeroVector;
	if (!Sequence)
	{
		return Result;
	}

	const IAnimationDataModel* Model = Sequence->GetDataModel();
	if (!Model)
	{
		return Result;
	}

	return Model->GetBoneTrackTransform(BoneName, FFrameNumber(0)).GetTranslation();
}

// 收口：站立层播放的是 /Game/ParagonKwang/.../Animations/Idle.Idle —— 整条链里唯一一个
// 没经过重定向的资产。它的骨骼平移量级和重定向产物不是一套（实测首帧
// spine_01=10.84 spine_02=19.24 neck_01=16.56，与曼尼昆 A_Als_Stand_Pose 完全相同；
// 重定向产物是 6.66/10.52/25.10）。状态机在两者之间过渡时会形成一条统一 Alpha 的混合，
// 表现就是角色被拉长一段时间（PP.WatchTorso 抓到 6.66→10.84 的等系数渐变）。
// 这里把那一处换回重定向目录里的 A_Als_IdleKwang，让站立层回到同一套比例。
// 顺带把骨架参考位姿也打出来，用来判断重定向产物的比例本身对不对。
// 用法：在编辑器控制台执行  PP.UseRetargetedIdle（不需要 PIE）
static void LogTorsoLengths(const TCHAR* Label, UAnimSequence* Sequence)
{
	if (!Sequence)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] %s 加载失败"), Label);
		return;
	}

	const TArray<FName> Bones = { FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01") };
	TArray<FString> Parts;
	for (const FName BoneName : Bones)
	{
		Parts.Add(FString::Printf(TEXT("%s=%.2f"), *BoneName.ToString(), GetSequenceFrame0Translation(Sequence, BoneName).Size()));
	}

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] %s：%s  骨架=%s"),
		Label, *FString::Join(Parts, TEXT(" ")), *GetNameSafe(Sequence->GetSkeleton()));
}

static void UseRetargetedIdle()
{
	const FString FromObjectPath = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Idle.Idle");
	const FString ToObjectPath = DestFolder + TEXT("/A_Als_IdleKwang.A_Als_IdleKwang");

	UAnimSequence* FromAsset = LoadObject<UAnimSequence>(nullptr, *FromObjectPath);
	UAnimSequence* ToAsset = LoadObject<UAnimSequence>(nullptr, *ToObjectPath);
	if (!FromAsset || !ToAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 加载失败：%s / %s"), *FromObjectPath, *ToObjectPath);
		return;
	}

	// 先量三种来源的首帧骨骼长度，日志里能直接看出谁和谁不是一套比例
	if (USkeleton* Skeleton = FromAsset->GetSkeleton())
	{
		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		TArray<FString> Parts;
		for (const FName BoneName : { FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01") })
		{
			const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
			if (BoneIndex != INDEX_NONE)
			{
				Parts.Add(FString::Printf(TEXT("%s=%.2f"), *BoneName.ToString(),
					RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslation().Size()));
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 骨架参考位姿（%s）：%s"),
			*Skeleton->GetName(), *FString::Join(Parts, TEXT(" ")));
	}

	LogTorsoLengths(TEXT("Idle.Idle"), FromAsset);
	LogTorsoLengths(TEXT("A_Als_IdleKwang"), ToAsset);

	const FString StandingObjectPath = DestFolder + TEXT("/AB_Als_StandingKwang.AB_Als_StandingKwang");
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *StandingObjectPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到站立层：%s"), *StandingObjectPath);
		return;
	}

	TArray<UObject*> ObjectsToProcess;
	ObjectsToProcess.Add(Blueprint);
	ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
	{
		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
		{
			ObjectsToProcess.Add(GraphNode);
		}
		return true;
	}, true);

	Blueprint->Modify();
	int32 ReplacedCount = 0;
	for (UObject* Object : ObjectsToProcess)
	{
		ReplacedCount += ReplaceSpecificAssetInStruct(Object->GetClass(), Object, FromAsset, ToAsset);
	}

	if (ReplacedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 站立层里没有找到 Idle.Idle 的引用，未做修改"));
		return;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	Blueprint->GetOutermost()->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 站立层 Idle 引用替换 %d 处（只动 AB_Als_StandingKwang）。请执行 File > Save All 落盘。"),
		ReplacedCount);
}

static FAutoConsoleCommand UseRetargetedIdleCommand(
	TEXT("PP.UseRetargetedIdle"),
	TEXT("把站立层里 Paragon 自带的 Idle.Idle 换回重定向目录的 A_Als_IdleKwang，统一骨骼比例"),
	FConsoleCommandDelegate::CreateStatic(&UseRetargetedIdle)
);

// 回退：A_Als_IdleKwang 的平移轨道是空的，SequenceEvaluator 一播整个骨架就塌到原点
// （角色缩成地面上的一个小点）。这儿改回原有的 A_Als_Stand_PoseKwang——它平移正常，
// 而且和其余重定向资产同一套比例；同时整条链里最后一条非重定向资产（Idle.Idle）也一并去掉。
// 用法：在编辑器控制台执行  PP.RevertStandingIdle
static void RevertStandingIdle()
{
	const FString FromObjectPath = DestFolder + TEXT("/A_Als_IdleKwang.A_Als_IdleKwang");
	const FString ToObjectPath = DestFolder + TEXT("/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang");

	UAnimSequence* FromAsset = LoadObject<UAnimSequence>(nullptr, *FromObjectPath);
	UAnimSequence* ToAsset = LoadObject<UAnimSequence>(nullptr, *ToObjectPath);
	if (!FromAsset || !ToAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 加载失败：%s / %s"), *FromObjectPath, *ToObjectPath);
		return;
	}

	LogTorsoLengths(TEXT("A_Als_IdleKwang（当前）"), FromAsset);
	LogTorsoLengths(TEXT("A_Als_Stand_PoseKwang（回退目标）"), ToAsset);

	const FString StandingObjectPath = DestFolder + TEXT("/AB_Als_StandingKwang.AB_Als_StandingKwang");
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *StandingObjectPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[RetargetTool] 找不到站立层：%s"), *StandingObjectPath);
		return;
	}

	TArray<UObject*> ObjectsToProcess;
	ObjectsToProcess.Add(Blueprint);
	ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
	{
		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
		{
			ObjectsToProcess.Add(GraphNode);
		}
		return true;
	}, true);

	Blueprint->Modify();
	int32 ReplacedCount = 0;
	for (UObject* Object : ObjectsToProcess)
	{
		ReplacedCount += ReplaceSpecificAssetInStruct(Object->GetClass(), Object, FromAsset, ToAsset);
	}

	if (ReplacedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 站立层里没有找到 A_Als_IdleKwang 的引用，未做修改"));
		return;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	Blueprint->GetOutermost()->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("[RetargetTool] 回退完成：站立层替换 %d 处。请执行 File > Save All 落盘。"), ReplacedCount);
}

static FAutoConsoleCommand RevertStandingIdleCommand(
	TEXT("PP.RevertStandingIdle"),
	TEXT("回退站立层的 Idle：A_Als_IdleKwang（平移为空）换回 A_Als_Stand_PoseKwang"),
	FConsoleCommandDelegate::CreateStatic(&RevertStandingIdle)
);

// 按「资源路径」而不是指针比对来替换。上一版用指针比对，在站立层里静默匹配不到：
// LoadObject 出来的对象和蓝图里存的引用未必是同一个实例，指针相等判断就失效了。
// 顺带把每个「不在重定向目录里」的动画资产连同所在对象的名字一起打出来，
// 这样即使没匹配上，也能看到那处 Idle.Idle 到底挂在哪个节点上。
static int32 ReplaceAssetByPathInStruct(const UStruct* Struct, void* StructAddress,
	const FString& FromPath, UAnimationAsset* ToAsset, const FString& OwnerLabel, int32 Depth)
{
	if (!Struct || Depth > 8)
	{
		return 0;
	}

	int32 ReplacedCount = 0;

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			ReplacedCount += ReplaceAssetByPathInStruct(StructProperty->Struct,
				StructProperty->ContainerPtrToValuePtr<void>(StructAddress), FromPath, ToAsset, OwnerLabel, Depth + 1);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					ReplacedCount += ReplaceAssetByPathInStruct(InnerStructProperty->Struct,
						Helper.GetRawPtr(Index), FromPath, ToAsset, OwnerLabel, Depth + 1);
				}
			}
		}
		else if (CastField<FSoftObjectProperty>(Property))
		{
			continue;
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			void* ValueAddress = ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress);
			UObject* Value = ObjectProperty->GetObjectPropertyValue(ValueAddress);
			if (!Value || !Value->IsA(UAnimationAsset::StaticClass()))
			{
				continue;
			}

			const FString ValuePath = Value->GetPathName();
			if (ValuePath == FromPath)
			{
				if (ToAsset)
				{
					ObjectProperty->SetObjectPropertyValue(ValueAddress, ToAsset);
					UE_LOG(LogTemp, Warning, TEXT("[Restore] 替换 %s（%s.%s）：%s → %s"),
						*OwnerLabel, *Struct->GetName(), *Property->GetName(), *ValuePath, *ToAsset->GetPathName());
					++ReplacedCount;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Restore] 命中（未在此处替换） %s（%s.%s）：%s"),
						*OwnerLabel, *Struct->GetName(), *Property->GetName(), *ValuePath);
				}
			}
			else if (!ValuePath.StartsWith(DestFolder))
			{
				// 不是重定向目录里的资产：这条就是比例不一致的来源候选。
				// 顺带量首帧 spine_01 的长度：曼尼昆量级是 10.84，重定向产物是 6.66，
				// 一眼就能看出这条引用是不是把曼尼昆的字面平移搬过来了。
				FString Extra;
				if (UAnimSequence* Sequence = Cast<UAnimSequence>(Value))
				{
					Extra = FString::Printf(TEXT("  首帧spine_01=%.2f 骨架=%s"),
						GetSequenceFrame0Translation(Sequence, FName("spine_01")).Size(),
						*GetNameSafe(Sequence->GetSkeleton()));
				}

				UE_LOG(LogTemp, Warning, TEXT("[Restore] 非重定向资产 %s（%s.%s）：%s%s"),
					*OwnerLabel, *Struct->GetName(), *Property->GetName(), *ValuePath, *Extra);
			}
		}
	}

	return ReplacedCount;
}

// 全目录扫一遍：找出还有谁引用非重定向目录的动画资产。站立层命中就顺手换掉，
// 其它资产只打印（连同所在资产名），用来定位 Idle.Idle 到底挂在哪里。
// 除了蓝图包内的图节点，还要看编译后的 UAnimBlueprintGeneratedClass 的 CDO——
// 运行期实例的节点默认值来自那里，图节点里没有的引用有可能只存在于编译产物上。
// 用法：在编辑器控制台执行  PP.RestoreStandingPose
static void RestoreStandingPose()
{
	const FString FromPath = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Idle.Idle");
	const FString ToObjectPath = DestFolder + TEXT("/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang");
	const FString StandingObjectPath = DestFolder + TEXT("/AB_Als_StandingKwang.AB_Als_StandingKwang");

	UAnimSequence* ToAsset = LoadObject<UAnimSequence>(nullptr, *ToObjectPath);
	if (!ToAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[Restore] 加载失败：%s"), *ToObjectPath);
		return;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	UBlueprint* StandingBlueprint = nullptr;
	int32 ScannedAssetCount = 0;
	int32 ReplacedCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		++ScannedAssetCount;

		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		const bool bIsStanding = Asset->GetPathName() == StandingObjectPath;
		UAnimationAsset* Replacement = bIsStanding ? ToAsset : nullptr;

		if (bIsStanding)
		{
			StandingBlueprint = Blueprint;
			if (Blueprint)
			{
				Blueprint->Modify();
			}
		}

		// 1) 资产本体 + 蓝图包内的全部图节点（状态机子图也是包内对象）
		TArray<UObject*> ObjectsToProcess;
		ObjectsToProcess.Add(Asset);

		if (Blueprint)
		{
			ForEachObjectWithPackage(Blueprint->GetOutermost(), [&ObjectsToProcess](UObject* Object)
			{
				if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
				{
					ObjectsToProcess.Add(GraphNode);
				}
				return true;
			}, true);
		}

		for (UObject* Object : ObjectsToProcess)
		{
			ReplacedCount += ReplaceAssetByPathInStruct(Object->GetClass(), Object, FromPath,
				Replacement, FString::Printf(TEXT("%s.%s"), *Asset->GetName(), *Object->GetName()), 0);
		}

		// 2) 编译产物：每个动画节点的结构体默认值在生成类 CDO 里另存了一份。
		// 这里只做诊断、不写回（图节点才是权威来源），命中就说明图与编译产物不同步，
		// 末尾对站立层重新编译即可重建 CDO。
		if (UAnimBlueprintGeneratedClass* AnimClass = Blueprint ? Cast<UAnimBlueprintGeneratedClass>(Blueprint->GeneratedClass) : nullptr)
		{
			UObject* CDO = AnimClass->GetDefaultObject();
			if (CDO)
			{
				for (FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
				{
					ReplaceAssetByPathInStruct(NodeProperty->Struct,
						NodeProperty->ContainerPtrToValuePtr<void>(CDO), FromPath, nullptr,
						FString::Printf(TEXT("%s.CDO.%s"), *Asset->GetName(), *NodeProperty->GetName()), 0);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Restore] 扫描 %s 下 %d 个资产，站立层内替换 %d 处。"),
		*DestFolder, ScannedAssetCount, ReplacedCount);

	// 站立层无条件重编译：即使图里没有命中，也能把编译产物里可能残留的旧默认值重建掉
	if (StandingBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(StandingBlueprint);
		StandingBlueprint->MarkPackageDirty();
		StandingBlueprint->GetOutermost()->MarkPackageDirty();
		UE_LOG(LogTemp, Warning, TEXT("[Restore] 已重新编译 %s。请执行 File > Save All 落盘。"), *StandingBlueprint->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Restore] 没找到站立层蓝图：%s"), *StandingObjectPath);
	}
}

static FAutoConsoleCommand RestoreStandingPoseCommand(
	TEXT("PP.RestoreStandingPose"),
	TEXT("把站立层里的 Paragon Idle.Idle 换回 A_Als_Stand_PoseKwang（按资源路径比对）"),
	FConsoleCommandDelegate::CreateStatic(&RestoreStandingPose)
);

// 决定性验证：让角色网格绕开所有动画蓝图接线，直接单节点播放指定的一个资产。
// 判读（躯干长度，按 pelvis→root 之外的四段）：
//   6.66 / 10.52 / 6.85 / 25.10  → 资产和它的压缩数据都是重定向后的比例，问题在动画节点接线
//   10.84 / 19.24 / 13.38 / 16.56 → 运行期读到的仍是曼尼昆的压缩数据（CompressedData 过期）
// 用法：PIE 里执行  PP.TestPlayStandPose，或  PP.TestPlayAsset A_Als_Look_UpKwang
// 注意：会把网格切到单节点模式，测完请重启 PIE
static USkeletalMeshComponent* FindPieCharacterMesh()
{
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		if (USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr)
		{
			return Mesh;
		}
	}

	return nullptr;
}

static void PlaySingleNodeAsset(UAnimationAsset* Asset)
{
	USkeletalMeshComponent* Mesh = FindPieCharacterMesh();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Error, TEXT("[SingleNode] 没找到角色网格"));
		return;
	}

	if (!Asset)
	{
		UE_LOG(LogTemp, Error, TEXT("[SingleNode] 资产加载失败"));
		return;
	}

	Mesh->PlayAnimation(Asset, true);
	Mesh->TickPose(0.f, false);
	Mesh->RefreshBoneTransforms();

	const TArray<FName> BoneNames =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	const FReferenceSkeleton& ReferenceSkeleton = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	FString Result;
	for (const FName& BoneName : BoneNames)
	{
		const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		const int32 ParentIndex = BoneIndex != INDEX_NONE ? ReferenceSkeleton.GetParentIndex(BoneIndex) : INDEX_NONE;
		if (ParentIndex == INDEX_NONE)
		{
			continue;
		}

		const FName ParentName = ReferenceSkeleton.GetBoneName(ParentIndex);
		const float WorldDistance = static_cast<float>(FVector::Dist(
			Mesh->GetBoneTransform(BoneName, RTS_World).GetTranslation(),
			Mesh->GetBoneTransform(ParentName, RTS_World).GetTranslation()));

		Result += FString::Printf(TEXT("%s=%.2f "), *BoneName.ToString(), WorldDistance);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SingleNode] 单节点播放 %s：%s"), *Asset->GetName(), *Result);
	UE_LOG(LogTemp, Warning, TEXT("[SingleNode] 期望 Kwang：6.66 / 10.52 / 6.85 / 25.10 / 6.22；曼尼昆：10.84 / 19.24 / 13.38 / 16.56 / 9.29"));
}

static void TestPlayStandPose()
{
	PlaySingleNodeAsset(LoadObject<UAnimationAsset>(nullptr,
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang")));
}

static void TestPlayAsset(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SingleNode] 用法：PP.TestPlayAsset <AlsKwang 里的资产名>，例如 PP.TestPlayAsset A_Als_Look_UpKwang"));
		return;
	}

	const FString AssetName = Args[0];
	const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestFolder, *AssetName, *AssetName);

	UAnimationAsset* Asset = LoadObject<UAnimationAsset>(nullptr, *ObjectPath);
	if (!Asset)
	{
		UE_LOG(LogTemp, Error, TEXT("[SingleNode] 找不到资产：%s"), *ObjectPath);
		return;
	}

	// 顺带确认这个资产的骨架和加性设置：骨架不是 Kwang、或者加性但 BasePose 是别人的，都会让比例跑偏
	FString Info = FString::Printf(TEXT("[SingleNode] %s 骨架=%s"), *AssetName, *GetNameSafe(Asset->GetSkeleton()));
	if (const UAnimSequence* Sequence = Cast<UAnimSequence>(Asset))
	{
		Info += FString::Printf(TEXT(" 加性类型=%d BasePose=%s"),
			static_cast<int32>(Sequence->GetAdditiveAnimType()), *GetNameSafe(Sequence->GetAdditiveBasePose()));
	}
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Info);

	PlaySingleNodeAsset(Asset);
}

static FAutoConsoleCommand TestPlayStandPoseCommand(
	TEXT("PP.TestPlayStandPose"),
	TEXT("PIE 中让角色网格直接单节点播放 A_Als_Stand_PoseKwang，绕开动画蓝图验证资产压缩数据"),
	FConsoleCommandDelegate::CreateStatic(&TestPlayStandPose)
);

static FAutoConsoleCommand TestPlayAssetCommand(
	TEXT("PP.TestPlayAsset"),
	TEXT("PIE 中让角色网格单节点播放 AlsKwang 里的指定资产并量躯干长度：PP.TestPlayAsset A_Als_Look_UpKwang"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&TestPlayAsset)
);

// UAnimSequence::ClearAllCompressionData() 是 protected，外部调不到。
// 这里用一个不带任何成员的同名派生类只做「访问权限转发」：非虚函数，只访问基类成员，
// 不构造实例，纯属为了让编译器的访问检查通过。
class FKwangAnimSequenceAccess : public UAnimSequence
{
public:
	static void ForceClearCompressionData(UAnimSequence* Sequence)
	{
		static_cast<FKwangAnimSequenceAccess*>(Sequence)->ClearAllCompressionData();
	}
};

// FAnimNode_LinkedAnimGraph::ReinitializeLinkedAnimInstance() 是 protected，外部调不到。
// 与 FKwangAnimSequenceAccess 同一套做法：派生一个不带成员的类，只为转发访问权限。
// 该函数非虚，只访问基类成员，指针强转是安全的。
class FKwangLinkedAnimGraphAccess : public FAnimNode_LinkedAnimGraph
{
public:
	static void Reinitialize(FAnimNode_LinkedAnimGraph* Node, const UAnimInstance* OwningInstance)
	{
		static_cast<FKwangLinkedAnimGraphAccess*>(Node)->ReinitializeLinkedAnimInstance(OwningInstance);
	}
};

// 运行期逐层排查：把指定联动层的 InstanceClass 清空并重新初始化，等于把这一层从混合链里摘掉，
// 这样能直接看出「拉长」是哪个层算出来的，而不用去猜资产。
// 可用层名（取自 AB_Als_Kwang 的联动节点）：Locomotion / Grounded / Standing / Crouching /
// Layering / Head / Ragdolling / Default（Overlay）
// 用法：PIE 里  PP.MuteLayer Head   /   PP.MuteLayer Layering   /   PP.MuteLayer Default
static void MuteLayer(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mute] 用法：PP.MuteLayer Head（或 Layering / Default / Standing / Grounded / Locomotion）"));
		return;
	}

	USkeletalMeshComponent* Mesh = FindPieCharacterMesh();
	UAnimInstance* Instance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	UAnimBlueprintGeneratedClass* AnimClass = Instance ? Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass()) : nullptr;
	if (!AnimClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Mute] PIE 里没有找到正在运行的动画实例"));
		return;
	}

	const FString Want = Args[0].ToLower();
	int32 MutedCount = 0;

	for (const FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
	{
		if (!NodeProperty || !NodeProperty->Struct || !NodeProperty->Struct->IsChildOf(FAnimNode_LinkedAnimGraph::StaticStruct()))
		{
			continue;
		}

		FObjectProperty* InstanceClassProperty = FindFProperty<FObjectProperty>(NodeProperty->Struct, TEXT("InstanceClass"));
		if (!InstanceClassProperty)
		{
			continue;
		}

		void* NodeMemory = NodeProperty->ContainerPtrToValuePtr<void>(Instance);
		const UObject* CurrentClass = InstanceClassProperty->GetObjectPropertyValue_InContainer(NodeMemory);
		if (!CurrentClass || !CurrentClass->GetName().ToLower().Contains(Want))
		{
			continue;
		}

		FAnimNode_LinkedAnimGraph* LinkedNode = static_cast<FAnimNode_LinkedAnimGraph*>(NodeMemory);
		UE_LOG(LogTemp, Warning, TEXT("[Mute] 摘掉联动层 %s（%s）"),
			*NodeProperty->Struct->GetName(), *CurrentClass->GetName());

		LinkedNode->InstanceClass = nullptr;
		FKwangLinkedAnimGraphAccess::Reinitialize(LinkedNode, Instance);
		++MutedCount;
	}

	if (MutedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mute] 没有匹配「%s」的联动层，只做测量"), *Args[0]);
	}

	Mesh->TickPose(0.f, false);
	Mesh->RefreshBoneTransforms();

	const float Spine01Local = static_cast<float>(Mesh->GetBoneTransform(FName("spine_01"), RTS_ParentBoneSpace).GetTranslation().Size());
	UE_LOG(LogTemp, Warning, TEXT("[Mute] 摘掉 %d 个层后 spine_01 局部位移=%.2f（Kwang 应为 6.66）"), MutedCount, Spine01Local);
}

static FAutoConsoleCommand MuteLayerCommand(
	TEXT("PP.MuteLayer"),
	TEXT("PIE 中摘掉指定的 ALS 联动层，用来定位「躯干被拉长」是哪一层算出来的：PP.MuteLayer Head"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MuteLayer)
);

// 按编译后的执行顺序打印某个动画蓝图的节点链（GetAnimNodeProperties 的顺序就是求值顺序）。
// 逐节点置空资产无效、但整层摘掉有效时，只有看这个顺序才能确认「哪一步的混合算式放大了骨骼」。
// 用法：PP.DumpNodeOrder StandingKwang   /   PP.DumpNodeOrder LayeringKwang
static void DumpNodeOrder(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Order] 用法：PP.DumpNodeOrder StandingKwang"));
		return;
	}

	const FString Want = Args[0].ToLower();

	for (TObjectIterator<UAnimBlueprintGeneratedClass> It; It; ++It)
	{
		UAnimBlueprintGeneratedClass* AnimClass = *It;
		if (!AnimClass || AnimClass->GetName().ToLower().Contains(Want) == false)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Order] === %s 节点链（按求值顺序）==="), *AnimClass->GetName());

		UAnimInstance* DefaultInstance = AnimClass->GetDefaultObject<UAnimInstance>();

		int32 NodeIndex = 0;
		for (const FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
		{
			if (!NodeProperty || !NodeProperty->Struct)
			{
				continue;
			}

			UE_LOG(LogTemp, Warning, TEXT("[Order]   %d %s"), NodeIndex++, *NodeProperty->Struct->GetName());

			if (!DefaultInstance)
			{
				continue;
			}

			void* NodeMemory = NodeProperty->ContainerPtrToValuePtr<void>(DefaultInstance);
			for (const FName PropertyName : {FName("Sequence"), FName("BlendSpace"), FName("InstanceClass")})
			{
				FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(NodeProperty->Struct, PropertyName);
				if (!Property)
				{
					continue;
				}

				if (const UObject* Value = Property->GetObjectPropertyValue_InContainer(NodeMemory))
				{
					UE_LOG(LogTemp, Warning, TEXT("[Order]       %s = %s"), *PropertyName.ToString(), *Value->GetName());
				}
			}
		}
	}
}

static FAutoConsoleCommand DumpNodeOrderCommand(
	TEXT("PP.DumpNodeOrder"),
	TEXT("按求值顺序打印指定动画蓝图的节点链：PP.DumpNodeOrder StandingKwang"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DumpNodeOrder)
);

// 清掉 AlsKwang 里每个序列继承来的「重定向源」（RetargetSource / RetargetSourceAssetReferencePose）。
// 这些资产是「复制 ALS 源资产 + 换骨架」得到的，会带着曼尼昆的参考姿势；
// 而姿势数据本身已经是 Kwang 比例，引擎再按曼尼昆参考姿势做一次平移重定向，
// 就会给每根骨骼加上「曼尼昆长度 − Kwang长度」（实测 spine_01 正好多 4.18）。
// 用法：编辑器控制台执行  PP.ClearRetargetSource ，然后 File > Save All，重启 PIE
static void ClearRetargetSource()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	int32 ClearedCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UAnimSequence* Sequence = Cast<UAnimSequence>(AssetData.GetAsset());
		if (!Sequence)
		{
			continue;
		}

		// 只在处理前打印一次有问题的资产，确认「重定向源参考姿势」确实是曼尼昆量级
		if (ClearedCount < 8)
		{
			const int32 SpineIndex = Sequence->GetSkeleton()
				? Sequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(FName("spine_01")) : INDEX_NONE;
			const TArray<FTransform>& RetargetTransforms = Sequence->GetRetargetTransforms();
			const float RetargetSpine01 = (SpineIndex != INDEX_NONE && RetargetTransforms.IsValidIndex(SpineIndex))
				? static_cast<float>(RetargetTransforms[SpineIndex].GetTranslation().Size()) : -1.0f;

			UE_LOG(LogTemp, Warning, TEXT("[RetargetSource] %s 源名=%s 参考姿势=%d 根 spine_01=%.2f 自身 spine_01=%.2f"),
				*Sequence->GetName(), *Sequence->GetRetargetTransformsSourceName().ToString(),
				RetargetTransforms.Num(), RetargetSpine01,
				static_cast<float>(GetSequenceFrame0Translation(Sequence, FName("spine_01")).Size()));
		}

		Sequence->Modify();
		Sequence->RetargetSource = NAME_None;
		Sequence->RetargetSourceAssetReferencePose.Empty();

		// 重定向源参与压缩，必须让压缩数据重烘一遍
		FKwangAnimSequenceAccess::ForceClearCompressionData(Sequence);
		Sequence->CacheDerivedDataForCurrentPlatform();
		Sequence->MarkPackageDirty();

		++ClearedCount;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RetargetSource] 已清掉 %d 个序列的重定向源并重建压缩数据。请 File > Save All 后重启 PIE 复测。"), ClearedCount);
}

static FAutoConsoleCommand ClearRetargetSourceCommand(
	TEXT("PP.ClearRetargetSource"),
	TEXT("清除 AlsKwang 序列继承自曼尼昆的重定向源参考姿势，避免运行期再追加一次平移重定向"),
	FConsoleCommandDelegate::CreateStatic(&ClearRetargetSource)
);

// 运行期诊断：直接遍历「绑定在该网格上的每个动画实例」的动画节点结构（FAnimNode_*），
// 列出里面仍然指向 /ALS/（曼尼昆）的所有引用。
// 骨骼到父骨骼的距离 = 该骨骼的局部平移长度；只要某个节点还带着 /ALS/ 的序列，
// 平移按 Animation 模式原样保留，曼尼昆的字面位移就会被搬到 Kwang 骨骼上
// （实测 10.84 / 19.24 / 13.38 / 16.56 / 9.29）。
// 用法：PIE 里执行  PP.DumpLiveAnimRefs
static void CollectAlsPluginRefs(const UStruct* Struct, void* StructAddress, const FString& InstanceLabel, TSet<FString>& OutRefs, int32 Depth)
{
	if (!Struct || Depth > 8)
	{
		return;
	}

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectAlsPluginRefs(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), InstanceLabel, OutRefs, Depth + 1);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					CollectAlsPluginRefs(InnerStructProperty->Struct, Helper.GetRawPtr(Index), InstanceLabel, OutRefs, Depth + 1);
				}
			}
		}
		// 软引用不能按 UObject* 直接读，跳过；动画节点里的序列和类都是硬引用，不影响诊断
		else if (CastField<FSoftObjectProperty>(Property))
		{
			continue;
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			UObject* Value = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (!Value)
			{
				continue;
			}

			const FString ValuePath = Value->GetPathName();
			UClass* ValueClass = Cast<UClass>(Value);

			// 联动节点的实体类（含 *Kwang）一律打印，才能看出壳到底把 Layering 挂到了哪个类；
			// 非类的资产引用只打印 /ALS/ 曼尼昆的，避免把 Kwang 序列刷满
			if ((ValueClass && ValueClass->GetName().StartsWith(TEXT("AB_Als_")))
				|| ValuePath.StartsWith(TEXT("/ALS/")))
			{
				OutRefs.Add(FString::Printf(TEXT("%s | %s.%s = %s"),
					*InstanceLabel, *Struct->GetName(), *Property->GetName(), *ValuePath));
			}
		}
	}
}

static void DumpLiveAnimRefs()
{
	USkeletalMeshComponent* Mesh = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		Mesh = Character ? Character->GetMesh() : nullptr;
		if (Mesh)
		{
			break;
		}
	}

	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[AlsRefs] 没找到角色网格"));
		return;
	}

	TSet<FString> Refs;
	int32 ScanInstanceCount = 0;

	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Instance = *It;
		if (!Instance || Instance->GetSkelMeshComponent() != Mesh)
		{
			continue;
		}

		UAnimBlueprintGeneratedClass* AnimClass = Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass());
		if (!AnimClass)
		{
			continue;
		}

		++ScanInstanceCount;
		for (FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
		{
			if (!NodeProperty)
			{
				continue;
			}

			CollectAlsPluginRefs(NodeProperty->Struct,
				NodeProperty->ContainerPtrToValuePtr<void>(Instance),
				Instance->GetClass()->GetName(), Refs, 0);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AlsRefs] 扫描 %d 个动画实例，共 %d 条联动类/曼尼昆资产引用"), ScanInstanceCount, Refs.Num());
	for (const FString& Ref : Refs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AlsRefs]   %s"), *Ref);
	}
}

static FAutoConsoleCommand DumpLiveAnimRefsCommand(
	TEXT("PP.DumpLiveAnimRefs"),
	TEXT("PIE 中遍历已绑定的动画实例的动画节点，列出仍指向 /ALS/ 曼尼昆资源的引用"),
	FConsoleCommandDelegate::CreateStatic(&DumpLiveAnimRefs)
);

// 不做任何路径过滤地列出「动画源」引用：序列/蒙太奇/混合空间/姿态资产/各类 Class（含 ControlRig）。
// 上面那份 DumpLiveAnimRefs 只看 /ALS/ 和 AB_Als_*，会漏掉两类东西：
//   1) AlsKwang 目录里复制出来的姿态资产（PoseAsset 不会跟着重定向，平移还是曼尼昆的）；
//   2) 复制出来的 ControlRig（路径在 /Game 下，但绑的仍是曼尼昆骨骼朝向/位移）。
// 只统计 IsInitialized() 为真的实例，避免被几十个残链刷屏。
static void CollectPoseSourceRefs(const UStruct* Struct, void* StructAddress, const FString& InstanceLabel, TArray<FString>& OutRefs, int32 Depth)
{
	if (!Struct || Depth > 8)
	{
		return;
	}

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectPoseSourceRefs(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(StructAddress), InstanceLabel, OutRefs, Depth + 1);
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					CollectPoseSourceRefs(InnerStructProperty->Struct, Helper.GetRawPtr(Index), InstanceLabel, OutRefs, Depth + 1);
				}
			}
		}
		else if (CastField<FSoftObjectProperty>(Property))
		{
			continue;
		}
		else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			UObject* Value = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(StructAddress));
			if (!Value)
			{
				continue;
			}

			if (Value->IsA(UAnimationAsset::StaticClass()) || Value->IsA(UClass::StaticClass()))
			{
				// 顺带把「这个节点当前的播放时间 / 混合权重」反射出来：
				// 只看资产清单分不清哪个节点是当前生效的那个，有时间就能对上号。
				FString Extra;
				for (const FName FloatName : { FName("ExplicitTime"), FName("InternalTimeAccumulator"), FName("BlendWeight") })
				{
					if (const FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Struct, FloatName))
					{
						Extra += FString::Printf(TEXT(" %s=%.2f"), *FloatName.ToString(),
							FloatProperty->GetPropertyValue_InContainer(StructAddress));
					}
				}

				// 只给「曼尼昆量级」的序列打标记（重定向产物首帧 spine_01=6.66，
				// 曼尼昆是 10.84）：拉长时看一眼就知道哪个节点的资产在拖比例。
				if (UAnimSequence* Sequence = Cast<UAnimSequence>(Value))
				{
					const float Spine01Length = static_cast<float>(GetSequenceFrame0Translation(Sequence, FName("spine_01")).Size());
					if (Spine01Length > 8.0f)
					{
						Extra += FString::Printf(TEXT(" [曼尼昆量级 spine_01=%.2f]"), Spine01Length);
					}
				}

				OutRefs.AddUnique(FString::Printf(TEXT("%s | %s.%s = %s%s"),
					*InstanceLabel, *Struct->GetName(), *Property->GetName(), *Value->GetPathName(), *Extra));
			}
		}
	}
}

// 只看「藏在资产内部、不经过动画节点」的引用：混合空间的采样动画、加性动画的 Base Pose。
// PP.WatchTorso 的节点清单里看不到这两类引用（它们不是节点属性），
// 但它们照样会在运行期把另一个骨架的字面平移带进结果——是最后一处未核对的地方。
// 用法：编辑器控制台执行  PP.AuditInternalRefs（不需要 PIE）
static void AuditInternalRefs()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	TArray<FString> Refs;
	int32 ScannedCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		// 混合空间：采样动画存在 SampleData 里
		const bool bBlendSpace = Asset->IsA(UBlendSpace::StaticClass()) || Asset->IsA(UBlendSpace1D::StaticClass());
		// 序列：只有加性动画才有 Base Pose（RefPoseSeq），非加性序列这里没有内部引用
		UAnimSequence* Sequence = Cast<UAnimSequence>(Asset);
		const bool bAdditive = Sequence && Sequence->GetAdditiveAnimType() != EAdditiveAnimationType::AAT_None;

		if (!bBlendSpace && !bAdditive)
		{
			continue;
		}

		++ScannedCount;
		const FString Label = bAdditive
			? FString::Printf(TEXT("%s[加性 BasePose]"), *Asset->GetName())
			: FString::Printf(TEXT("%s[混合空间采样]"), *Asset->GetName());

		CollectPoseSourceRefs(Asset->GetClass(), Asset, Label, Refs, 0);
	}

	UE_LOG(LogTemp, Warning, TEXT("[InnerRef] 检查 %d 个混合空间/加性序列，共 %d 条内部引用"), ScannedCount, Refs.Num());
	for (const FString& Ref : Refs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InnerRef]   %s"), *Ref);
	}
}

static FAutoConsoleCommand AuditInternalRefsCommand(
	TEXT("PP.AuditInternalRefs"),
	TEXT("列出混合空间的采样动画与加性动画的 Base Pose，并标出曼尼昆量级的资产"),
	FConsoleCommandDelegate::CreateStatic(&AuditInternalRefs)
);

static void DumpLivePoseSources(USkeletalMeshComponent* Mesh)
{
	TArray<FString> Refs;
	int32 LiveInstanceCount = 0;

	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Instance = *It;
		if (!Instance || Instance->GetSkelMeshComponent() != Mesh || !Instance->IsInitialized())
		{
			continue;
		}

		UAnimBlueprintGeneratedClass* AnimClass = Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass());
		if (!AnimClass)
		{
			continue;
		}

		++LiveInstanceCount;
		for (FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
		{
			if (NodeProperty)
			{
				CollectPoseSourceRefs(NodeProperty->Struct, NodeProperty->ContainerPtrToValuePtr<void>(Instance),
					Instance->GetClass()->GetName(), Refs, 0);
			}
		}
	}

	// 骨架参考位姿一起打出来：如果拉长后的数值正好等于参考位姿，
	// 就说明那几根骨骼在那一帧根本没被动画驱动（回落到绑定姿势），而不是换成了别的资产。
	if (const USkeletalMesh* SkeletalMesh = Mesh->GetSkeletalMeshAsset())
	{
		const FReferenceSkeleton& ReferenceSkeleton = SkeletalMesh->GetRefSkeleton();
		FString Parts;
		for (const FName BoneName : { FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01") })
		{
			const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			Parts += FString::Printf(TEXT("%s=%.2f "), *BoneName.ToString(),
				BoneIndex != INDEX_NONE ? ReferenceSkeleton.GetRefBonePose()[BoneIndex].GetTranslation().Size() : 0.0);
		}
		UE_LOG(LogTemp, Warning, TEXT("[PoseSrc] 骨架参考位姿（%s）：%s"), *SkeletalMesh->GetName(), *Parts);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PoseSrc] 存活实例 %d 个，共 %d 条动画源引用"), LiveInstanceCount, Refs.Num());
	for (const FString& Ref : Refs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PoseSrc]   %s"), *Ref);
	}
}

// 逐帧监测：躯干一旦被拉长就立刻打印当时的完整状态。
// 拉长是瞬态的（一按 Enter 视角回正就恢复），手动跑 DumpKwangPose 抓不到那一帧，
// 所以这里每帧量 spine_01 到父骨骼 pelvis 的距离，超过 8（Kwang 应为 6.66）就落一条日志。
// 用法：PIE 里执行  PP.WatchTorso ，然后正常移动视角复现，最多记录 10 次或 30 秒。
static TWeakObjectPtr<USkeletalMeshComponent> WatchedMesh;
static FTSTicker::FDelegateHandle WatchTorsoHandle;
static double WatchTorsoStartTime = 0.0;
static int32 WatchTorsoLogCount = 0;
// 只在第一次命中时倒一次存活实例的动画源，避免 10 次命中刷 10 遍
static bool WatchTorsoDumpedSources = false;
// 第一次命中时在同一帧内对 Standing 层做三探测（拉长只持续约 1 秒，手动敲命令抓不到）
static bool WatchTorsoBisected = false;

// 层定位完成后再进一层：把该联动实例内部的每个动画节点的 Sequence / BlendSpace 逐个置空，
// 置空哪个节点后 spine_01 掉回 6.66，那个节点就是真正在输出曼尼昆比例姿势的节点。
static void BisectNodesInPlace(USkeletalMeshComponent* Mesh, UAnimInstance* TargetInstance)
{
	UAnimBlueprintGeneratedClass* AnimClass = Cast<UAnimBlueprintGeneratedClass>(TargetInstance->GetClass());
	if (!AnimClass)
	{
		return;
	}

	struct FNodeEntry
	{
		FObjectPropertyBase* Property = nullptr;
		void* ValueAddress = nullptr;
		UObject* Original = nullptr;
		FString Label;
	};

	TArray<FNodeEntry> Entries;

	for (const FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
	{
		if (!NodeProperty || !NodeProperty->Struct)
		{
			continue;
		}

		void* NodeMemory = NodeProperty->ContainerPtrToValuePtr<void>(TargetInstance);

		float Weight = -1.0f;
		if (const FFloatProperty* WeightProperty = FindFProperty<FFloatProperty>(NodeProperty->Struct, TEXT("BlendWeight")))
		{
			Weight = WeightProperty->GetPropertyValue_InContainer(NodeMemory);
		}

		for (const FName PropertyName : {FName("Sequence"), FName("BlendSpace")})
		{
			FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(NodeProperty->Struct, PropertyName);
			if (!Property)
			{
				continue;
			}

			void* ValueAddress = Property->ContainerPtrToValuePtr<void>(NodeMemory);
			UObject* Value = Property->GetObjectPropertyValue(ValueAddress);
			if (!Value)
			{
				continue;
			}

			Entries.Add({Property, ValueAddress, Value,
				FString::Printf(TEXT("%s.%s=%s 权重=%.2f"), *NodeProperty->Struct->GetName(),
					*PropertyName.ToString(), *Value->GetName(), Weight)});
		}
	}

	const auto MeasureSpine01 = [Mesh]()
	{
		Mesh->TickPose(0.f, false);
		Mesh->RefreshBoneTransforms();
		return static_cast<float>(Mesh->GetBoneTransform(FName("spine_01"), RTS_ParentBoneSpace).GetTranslation().Size());
	};

	UE_LOG(LogTemp, Warning, TEXT("[Bisect] 进入 %s 内部节点二分，共 %d 个节点，当前 spine_01=%.2f"),
		*TargetInstance->GetClass()->GetName(), Entries.Num(), MeasureSpine01());

	for (const FNodeEntry& Entry : Entries)
	{
		Entry.Property->SetObjectPropertyValue(Entry.ValueAddress, nullptr);
		const float MutedSpine01 = MeasureSpine01();
		Entry.Property->SetObjectPropertyValue(Entry.ValueAddress, Entry.Original);
		MeasureSpine01();

		UE_LOG(LogTemp, Warning, TEXT("[Bisect] 置空 %s → spine_01=%.2f"), *Entry.Label, MutedSpine01);
	}
}

// 在「已经拉长」的这一帧里，把每个联动层逐次摘掉、量一次、再装回去。
// 摘掉哪一层后 spine_01 掉回 6.66，那一层就是往姿态里注入曼尼昆比例平移的源头。
static void BisectLayersInPlace(USkeletalMeshComponent* Mesh, UAnimInstance* Instance)
{
	UAnimBlueprintGeneratedClass* AnimClass = Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass());
	if (!AnimClass)
	{
		return;
	}

	struct FLayerEntry
	{
		FAnimNode_LinkedAnimGraph* Node = nullptr;
		FString Name;
	};

	TArray<FLayerEntry> Layers;

	for (const FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
	{
		if (!NodeProperty || !NodeProperty->Struct || !NodeProperty->Struct->IsChildOf(FAnimNode_LinkedAnimGraph::StaticStruct()))
		{
			continue;
		}

		FAnimNode_LinkedAnimGraph* Node = static_cast<FAnimNode_LinkedAnimGraph*>(NodeProperty->ContainerPtrToValuePtr<void>(Instance));
		if (!Node->InstanceClass)
		{
			continue;
		}

		Layers.Add({Node, Node->InstanceClass->GetName()});
	}

	const auto MeasureSpine01 = [Mesh]()
	{
		Mesh->TickPose(0.f, false);
		Mesh->RefreshBoneTransforms();
		return static_cast<float>(Mesh->GetBoneTransform(FName("spine_01"), RTS_ParentBoneSpace).GetTranslation().Size());
	};

	UE_LOG(LogTemp, Warning, TEXT("[Bisect] 拉长帧内分层，当前 spine_01=%.2f（应 6.66），共 %d 层"),
		MeasureSpine01(), Layers.Num());

	UClass* SuspectClass = nullptr;

	for (const FLayerEntry& Layer : Layers)
	{
		const TSubclassOf<UAnimInstance> OriginalClass = Layer.Node->InstanceClass;

		Layer.Node->InstanceClass = nullptr;
		FKwangLinkedAnimGraphAccess::Reinitialize(Layer.Node, Instance);
		const float MutedSpine01 = MeasureSpine01();

		UE_LOG(LogTemp, Warning, TEXT("[Bisect] 摘掉 %s → spine_01=%.2f"), *Layer.Name, MutedSpine01);

		// 摘掉后回到正常比例的那一层就是源头
		if (!SuspectClass && MutedSpine01 < 7.0f)
		{
			SuspectClass = OriginalClass;
		}

		Layer.Node->InstanceClass = OriginalClass;
		FKwangLinkedAnimGraphAccess::Reinitialize(Layer.Node, Instance);
		MeasureSpine01();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Bisect] 分层结束，已恢复全部层，spine_01=%.2f"), MeasureSpine01());

	if (!SuspectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bisect] 没有任何单层被摘掉后恢复正常，说明是多层共同混合的结果"));
		return;
	}

	// 对该层实例再做一轮节点级二分
	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Candidate = *It;
		if (Candidate && Candidate->IsInitialized() && Candidate->GetClass() == SuspectClass
			&& Candidate->GetSkelMeshComponent() == Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Bisect] 定位到问题层 %s，进入其内部节点二分"), *SuspectClass->GetName());
			BisectNodesInPlace(Mesh, Candidate);
			break;
		}
	}
}

// 三探测（拉长帧内对 Standing 层实例）：
// ①打印每个状态机的当前状态名——确认拉长瞬间停在哪个状态 / 哪两个状态之间过渡
// ②逐节点转储结构体类型 + 全部标量属性——找出那条权重 0.39、不携带资产的姿势源藏在哪个节点
// ③原地重初始化 Standing 联动实例（不换 InstanceClass）再量一次——
//   掉回 6.66 说明是运行时缓存残留（惯性化之类），仍是 8.29 说明图结构按当前输入实时算出了曼尼昆平移
static void ProbeStandingInstance(USkeletalMeshComponent* Mesh, UAnimInstance* RootInstance)
{
	UAnimBlueprintGeneratedClass* RootClass = Cast<UAnimBlueprintGeneratedClass>(RootInstance->GetClass());
	if (!RootClass)
	{
		return;
	}

	// 根实例里找 Standing 联动节点（③要用）
	FAnimNode_LinkedAnimGraph* StandingNode = nullptr;
	for (const FStructProperty* NodeProperty : RootClass->GetAnimNodeProperties())
	{
		if (!NodeProperty || !NodeProperty->Struct || !NodeProperty->Struct->IsChildOf(FAnimNode_LinkedAnimGraph::StaticStruct()))
		{
			continue;
		}

		FAnimNode_LinkedAnimGraph* Node = static_cast<FAnimNode_LinkedAnimGraph*>(NodeProperty->ContainerPtrToValuePtr<void>(RootInstance));
		if (Node->InstanceClass && Node->InstanceClass->GetName().Contains(TEXT("Standing")))
		{
			StandingNode = Node;
			break;
		}
	}

	// 找正在运行的 Standing 联动实例
	UAnimInstance* Standing = nullptr;
	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Candidate = *It;
		if (Candidate && Candidate->IsInitialized() && Candidate->GetSkelMeshComponent() == Mesh
			&& Candidate->GetClass()->GetName().Contains(TEXT("Standing")))
		{
			Standing = Candidate;
			break;
		}
	}

	if (!Standing)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Probe] 没找到 Standing 联动实例"));
		return;
	}

	// ① 状态机当前状态名（5.7 没有 GetNumStateMachines，用 GetStateMachineInstance 递增探测到空为止）
	int32 MachineCount = 0;
	for (int32 Index = 0; Index < 16; ++Index)
	{
		if (!Standing->GetStateMachineInstance(Index))
		{
			break;
		}
		++MachineCount;
		UE_LOG(LogTemp, Warning, TEXT("[Probe] 状态机[%d] 当前状态=%s"), Index, *Standing->GetCurrentStateName(Index).ToString());
	}
	UE_LOG(LogTemp, Warning, TEXT("[Probe] Standing=%s 状态机数=%d"), *Standing->GetClass()->GetName(), MachineCount);

	// ⑥ 活动蒙太奇转储：Slot 注入的蒙太奇不体现在节点属性里，单独枚举（动态蒙太奇要挖出包装的源序列）
	int32 MontageCount = 0;
	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Instance = *It;
		if (!Instance || !Instance->IsInitialized() || Instance->GetSkelMeshComponent() != Mesh)
		{
			continue;
		}

		for (const FAnimMontageInstance* MontageInstance : Instance->MontageInstances)
		{
			if (!MontageInstance || !MontageInstance->Montage)
			{
				continue;
			}

			FString SourceAnims;
			for (const FSlotAnimationTrack& SlotTrack : MontageInstance->Montage->SlotAnimTracks)
			{
				for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
				{
					if (Segment.GetAnimReference())
					{
						SourceAnims += Segment.GetAnimReference()->GetPathName() + TEXT(" ; ");
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[Probe] 蒙太奇 实例=%s Montage=%s 骨架=%s 权重=%.3f 源动画=%s"),
				*Instance->GetClass()->GetName(),
				*MontageInstance->Montage->GetName(),
				MontageInstance->Montage->GetSkeleton() ? *MontageInstance->Montage->GetSkeleton()->GetName() : TEXT("None"),
				MontageInstance->GetWeight(),
				*SourceAnims);
			++MontageCount;
		}
	}
	if (MontageCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Probe] 蒙太奇：无活动实例"));
	}

	// 节点转储 lambda：变量名 + 结构体类型 + 标量属性（对象属性只列动画资产）
	const auto DumpInstanceNodes = [](UAnimInstance* Instance, const TCHAR* Tag)
	{
		UAnimBlueprintGeneratedClass* AnimClass = Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass());
		if (!AnimClass)
		{
			return;
		}

		for (const FStructProperty* NodeProperty : AnimClass->GetAnimNodeProperties())
		{
			if (!NodeProperty || !NodeProperty->Struct)
			{
				continue;
			}

			void* NodeMemory = NodeProperty->ContainerPtrToValuePtr<void>(Instance);
			FString Parts;

			for (TFieldIterator<FProperty> It(NodeProperty->Struct); It; ++It)
			{
				FProperty* Property = *It;
				const void* ValueAddress = Property->ContainerPtrToValuePtr<void>(NodeMemory);

				if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
				{
					UObject* Object = ObjectProperty->GetObjectPropertyValue(ValueAddress);
					if (Object && Object->IsA(UAnimationAsset::StaticClass()))
					{
						Parts += FString::Printf(TEXT(" %s=%s;"), *Property->GetName(), *Object->GetName());
					}
					continue;
				}

				if (CastField<FFloatProperty>(Property) || CastField<FDoubleProperty>(Property)
					|| CastField<FBoolProperty>(Property) || CastField<FIntProperty>(Property)
					|| CastField<FByteProperty>(Property) || CastField<FEnumProperty>(Property)
					|| CastField<FNameProperty>(Property))
				{
					FString ValueString;
					Property->ExportTextItem_Direct(ValueString, ValueAddress, nullptr, nullptr, PPF_None);
					Parts += FString::Printf(TEXT(" %s=%s;"), *Property->GetName(), *ValueString);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[Probe] %s 节点 %s (%s) |%s"),
				Tag, *NodeProperty->GetName(), *NodeProperty->Struct->GetName(), *Parts);
		}
	};

	// ⑤ 先倒根图节点——0.39 爬升的载体如果在根图混合节点上（比如某个 Blend 的 Alpha），这里能直接看到
	DumpInstanceNodes(RootInstance, TEXT("根图"));

	// ② Standing 全部节点
	DumpInstanceNodes(Standing, TEXT("Standing"));

	// ③ 原地重初始化 Standing（不换类），再量一次
	const auto MeasureSpine01 = [Mesh]()
	{
		Mesh->TickPose(0.f, false);
		Mesh->RefreshBoneTransforms();
		return static_cast<float>(Mesh->GetBoneTransform(FName("spine_01"), RTS_ParentBoneSpace).GetTranslation().Size());
	};

	// ④ 反向二分：只留 Standing，摘掉其余全部联动层，量 Standing 单独输出。
	// ≈10.84 = Standing 内部产出曼尼昆；≈6.66 = Standing 干净，污染源在根图混合/层间交叉
	if (StandingNode)
	{
		struct FSavedLayer
		{
			FAnimNode_LinkedAnimGraph* Node;
			TSubclassOf<UAnimInstance> Class;
		};
		TArray<FSavedLayer> SavedLayers;

		for (const FStructProperty* NodeProperty : RootClass->GetAnimNodeProperties())
		{
			if (!NodeProperty || !NodeProperty->Struct || !NodeProperty->Struct->IsChildOf(FAnimNode_LinkedAnimGraph::StaticStruct()))
			{
				continue;
			}

			FAnimNode_LinkedAnimGraph* Node = static_cast<FAnimNode_LinkedAnimGraph*>(NodeProperty->ContainerPtrToValuePtr<void>(RootInstance));
			if (!Node->InstanceClass)
			{
				continue;
			}

			SavedLayers.Add({Node, Node->InstanceClass});
			if (Node != StandingNode)
			{
				Node->InstanceClass = nullptr;
				FKwangLinkedAnimGraphAccess::Reinitialize(Node, RootInstance);
			}
		}

		const float SoloSpine01 = MeasureSpine01();
		UE_LOG(LogTemp, Warning, TEXT("[Probe] 只留 Standing（摘其余 %d 层）→ spine_01=%.2f（≈10.84=Standing 内部产出；≈6.66=Standing 干净，污染源在根图/层间）"),
			SavedLayers.Num() - 1, SoloSpine01);

		for (const FSavedLayer& Saved : SavedLayers)
		{
			if (Saved.Node != StandingNode)
			{
				Saved.Node->InstanceClass = Saved.Class;
				FKwangLinkedAnimGraphAccess::Reinitialize(Saved.Node, RootInstance);
			}
		}
		MeasureSpine01();
	}

	if (StandingNode)
	{
		const float BeforeReinit = MeasureSpine01();
		FKwangLinkedAnimGraphAccess::Reinitialize(StandingNode, RootInstance);
		const float AfterReinit = MeasureSpine01();
		UE_LOG(LogTemp, Warning, TEXT("[Probe] Standing 原地重初始化：前=%.2f 后=%.2f（后≈6.66=运行时缓存残留；后仍≈8.29=图结构实时算出）"),
			BeforeReinit, AfterReinit);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Probe] 根实例里没找到 Standing 联动节点，跳过重初始化探测"));
	}
}

static bool WatchTorsoTick(float DeltaTime)
{
	if (WatchTorsoLogCount >= 10 || FPlatformTime::Seconds() - WatchTorsoStartTime > 30.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] 监测结束，共捕获 %d 次"), WatchTorsoLogCount);
		WatchTorsoHandle.Reset();
		return false;
	}

	USkeletalMeshComponent* Mesh = WatchedMesh.Get();
	if (!Mesh)
	{
		WatchTorsoHandle.Reset();
		return false;
	}

	const FVector Spine01World = Mesh->GetBoneTransform(FName("spine_01"), RTS_World).GetTranslation();
	const FVector PelvisWorld = Mesh->GetBoneTransform(FName("pelvis"), RTS_World).GetTranslation();
	const float Spine01Distance = static_cast<float>(FVector::Dist(Spine01World, PelvisWorld));

	if (Spine01Distance < 8.0f)
	{
		return true;
	}

	++WatchTorsoLogCount;

	if (!WatchTorsoDumpedSources)
	{
		WatchTorsoDumpedSources = true;
		DumpLivePoseSources(Mesh);
	}

	if (!WatchTorsoBisected)
	{
		WatchTorsoBisected = true;
		if (UAnimInstance* RootInstance = Mesh->GetAnimInstance())
		{
			// 层二分已两次确认是 Standing；重初始化会清掉现场让二分失真，这次直接做三探测
			ProbeStandingInstance(Mesh, RootInstance);
		}
	}

	UAnimInstance* const PostProcessInstance = Mesh->GetPostProcessInstance();
	const UClass* const PostProcessAnimBP = Mesh->GetPostProcessAnimBPClassToBeUsed();

	const float Spine02Distance = static_cast<float>(FVector::Dist(
		Mesh->GetBoneTransform(FName("spine_02"), RTS_World).GetTranslation(), Spine01World));
	const float Neck01Distance = static_cast<float>(FVector::Dist(
		Mesh->GetBoneTransform(FName("neck_01"), RTS_World).GetTranslation(),
		Mesh->GetBoneTransform(FName("spine_03"), RTS_World).GetTranslation()));

	// 只统计存活/死亡数量：实例一多，拼进一条日志会被 1024 字符截断（上一版那行就是被切掉的），
	// 存活实例的完整名单由 DumpLivePoseSources 按行打印，不会再被截断。
	int32 LiveInstanceCount = 0;
	int32 DeadInstanceCount = 0;
	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Instance = *It;
		if (!Instance || Instance->GetSkelMeshComponent() != Mesh)
		{
			continue;
		}
		if (Instance->IsInitialized())
		{
			++LiveInstanceCount;
		}
		else
		{
			++DeadInstanceCount;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Watch] #%d spine_01=%.2f spine_02=%.2f neck_01=%.2f（Kwang 应 6.66/10.52/25.10）动画模式=%d 网格=%s 根实例=%s"),
		WatchTorsoLogCount, Spine01Distance, Spine02Distance, Neck01Distance,
		static_cast<int32>(Mesh->GetAnimationMode()),
		*GetNameSafe(Mesh->GetSkeletalMeshAsset()),
		*GetNameSafe(Mesh->GetAnimInstance()));

	// 上一条是世界空间距离（含组件缩放）；这里再打一条「相对父骨骼的局部平移」和组件缩放，
	// 用来区分「动画数据给的就是长骨骼」还是「组件被缩放」——资产侧已经确认全是 6.66，这一步是最后一刀。
	const float Spine01Local = static_cast<float>(Mesh->GetBoneTransform(FName("spine_01"), RTS_ParentBoneSpace).GetTranslation().Size());
	UE_LOG(LogTemp, Warning, TEXT("[Watch] #%d spine_01 局部位移=%.2f（Kwang 应为 6.66） 组件缩放=%s"),
		WatchTorsoLogCount, Spine01Local, *Mesh->GetComponentTransform().GetScale3D().ToString());

	// 拉长过程是「同一条权重 0.32 → 1.0 的线性混合」（三根骨骼的插值系数逐帧完全一致），
	// 说明某处有一条 Alpha 从 0 涨到 1 的混合节点在吃曼尼昆姿势。根实例和它的联动类都是 Kwang，
	// 因此嫌疑只剩「后处理动画实例」（它同样以该网格为 SkelMeshComponent，会出现在上面那份列表里）。
	UE_LOG(LogTemp, Warning, TEXT("[Watch] #%d 网格AnimClass=%s 后处理BP=%s 后处理实例=%s 主控网格=%s 实例(活=%d 死=%d)"),
		WatchTorsoLogCount, *GetNameSafe(Mesh->GetAnimClass()),
		PostProcessAnimBP ? *PostProcessAnimBP->GetName() : TEXT("无"),
		*GetNameSafe(PostProcessInstance),
		Mesh->LeaderPoseComponent.IsValid() ? *Mesh->LeaderPoseComponent->GetName() : TEXT("无"),
		LiveInstanceCount, DeadInstanceCount);

	return true;
}

static void WatchTorso()
{
	USkeletalMeshComponent* Mesh = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		Mesh = Character ? Character->GetMesh() : nullptr;
		if (Mesh)
		{
			break;
		}
	}

	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[Watch] 没找到角色网格"));
		return;
	}

	WatchedMesh = Mesh;
	WatchTorsoLogCount = 0;
	WatchTorsoDumpedSources = false;
	WatchTorsoBisected = false;
	WatchTorsoStartTime = FPlatformTime::Seconds();

	if (WatchTorsoHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WatchTorsoHandle);
	}
	WatchTorsoHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&WatchTorsoTick), 0.f);

	UE_LOG(LogTemp, Warning, TEXT("[Watch] 开始逐帧监测 %s 的躯干长度（30 秒 / 最多 10 次）"), *GetNameSafe(Mesh));
}

static FAutoConsoleCommand WatchTorsoCommand(
	TEXT("PP.WatchTorso"),
	TEXT("PIE 中逐帧监测躯干长度，角色被拉长的瞬间自动打印当时的实例与状态"),
	FConsoleCommandDelegate::CreateStatic(&WatchTorso)
);

// 内容审计：AlsKwang 目录里每个动画序列，逐帧检查躯干三根骨骼的局部平移长度。
// 骨骼到父骨骼的距离 = 该骨骼的局部平移长度，取“参考位姿”以外的东西就说明重定向没生效。
// 只要某一帧同时命中 spine_01≈10.81 / spine_02≈18.86 / neck_01≈16.56，
// 这个序列里就存着曼尼昆的字面位移（表现为躯干被拉长）。
// 编辑器里执行即可，不需要 PIE：  PP.AuditKwangTranslations
static void AuditKwangTranslations()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	const FName Spine01(TEXT("spine_01"));
	const FName Neck01(TEXT("neck_01"));

	// 正确重定向后 spine_01 的局部平移长度恒为 Kwang 参考位姿的 6.66（骨骼长度不随姿势变化），
	// 曼尼昆的同名骨骼是 10.81。用 7.2 做阈值，只要有一帧超过就说明存着曼尼昆字面位移。
	const float KwangSpine01Length = 6.66f;
	const float FlagThreshold = 7.2f;

	int32 SequenceCount = 0;
	int32 FlaggedCount = 0;
	int32 EmptyTranslationCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		UAnimSequence* Sequence = Cast<UAnimSequence>(Asset);
		if (!Sequence)
		{
			continue;
		}

		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Model)
		{
			continue;
		}

		++SequenceCount;

		const int32 NumFrames = Model->GetNumberOfFrames();
		float MaxSpine01Length = 0.f;
		float Frame0Neck01Length = 0.f;

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
		{
			const float Spine01Length = (float)Model->GetBoneTrackTransform(Spine01, FFrameNumber(FrameIndex)).GetTranslation().Size();
			MaxSpine01Length = FMath::Max(MaxSpine01Length, Spine01Length);

			if (FrameIndex == 0)
			{
				Frame0Neck01Length = (float)Model->GetBoneTrackTransform(Neck01, FFrameNumber(0)).GetTranslation().Size();
			}
		}

		if (MaxSpine01Length > FlagThreshold)
		{
			++FlaggedCount;
			UE_LOG(LogTemp, Warning, TEXT("[Audit] 未重定向：%s  spine_01长度最大=%.2f（Kwang 应为 %.2f） 首帧neck_01=%.2f  共%d帧"),
				*Sequence->GetName(), MaxSpine01Length, KwangSpine01Length, Frame0Neck01Length, NumFrames);
		}
		// 平移轨道为空/近零的序列同样不能用：被序列节点播出来会把整个骨架塌到原点，
		// 表现就是角色缩成地上一小团（A_Als_IdleKwang 就是这种）
		else if (MaxSpine01Length < 1.0f)
		{
			++EmptyTranslationCount;
			UE_LOG(LogTemp, Warning, TEXT("[Audit] 平移为空：%s  spine_01长度最大=%.2f  首帧neck_01=%.2f  共%d帧"),
				*Sequence->GetName(), MaxSpine01Length, Frame0Neck01Length, NumFrames);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Audit] 共检查 %d 个序列，%d 个含曼尼昆字面平移（躯干会被拉长），%d 个平移为空（骨架会塌缩）。"),
		SequenceCount, FlaggedCount, EmptyTranslationCount);
}

static FAutoConsoleCommand AuditKwangTranslationsCommand(
	TEXT("PP.AuditKwangTranslations"),
	TEXT("逐帧检查 AlsKwang 里每个序列的躯干平移，列出仍存着曼尼昆字面位移的资产"),
	FConsoleCommandDelegate::CreateStatic(&AuditKwangTranslations)
);

// 决定性诊断：把运行期实测的「局部位移向量」和三个参照摆在一起比。
// 只看长度分不清「运行期用的是曼尼昆数据」还是「压缩数据过期」——比对向量本身才能确定：
//   实测 == Kwang 首帧  -> 数据没问题，长的是角色本身比例，别再折腾动画
//   实测 == 曼尼昆首帧  -> 运行期确实在评估曼尼昆的字面位移，压缩数据过期（RebuildKwangCompression）
//   实测 != 两者        -> 是骨骼重定向/合并算出来的，要看骨架的重定向模式
// 用法：PIE 里执行  PP.DumpKwangPose
static void DumpKwangPose()
{
	const TArray<FName> BoneNames =
	{
		FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("neck_01"), FName("head")
	};

	USkeletalMeshComponent* Mesh = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		APlayerController* PlayerController = WorldContext.World() ? WorldContext.World()->GetFirstPlayerController() : nullptr;
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		Mesh = Character ? Character->GetMesh() : nullptr;
		if (Mesh)
		{
			break;
		}
	}

	if (!Mesh || !Mesh->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Error, TEXT("[Dump] 没找到角色网格"));
		return;
	}

	const USkeletalMesh* MeshAsset = Mesh->GetSkeletalMeshAsset();
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();

	UE_LOG(LogTemp, Warning, TEXT("[Dump] 网格=%s  骨架=%s  动画实例=%s"),
		*MeshAsset->GetName(),
		*GetNameSafe(MeshAsset->GetSkeleton()),
		*GetNameSafe(AnimInstance ? AnimInstance->GetClass() : nullptr));

	// 绑定同一个网格的全部动画实例：壳 + 每个联动层实例。
	// 如果这里出现 AB_Als_Standing_C 这种曼尼昆类，说明壳的联动实例类没换过，
	// 站立层播的就是 /ALS/ 的曼尼昆动画，躯干长度自然是曼尼昆的。
	int32 LinkedInstanceCount = 0;
	UE_LOG(LogTemp, Warning, TEXT("[Dump] 绑定该网格的动画实例："));
	for (TObjectIterator<UAnimInstance> It; It; ++It)
	{
		UAnimInstance* Instance = *It;
		if (!Instance || Instance->GetSkelMeshComponent() != Mesh)
		{
			continue;
		}

		++LinkedInstanceCount;
		UE_LOG(LogTemp, Warning, TEXT("[Dump]   - %s   Outer=%s"),
			*Instance->GetClass()->GetName(), *GetNameSafe(Instance->GetOuter()));
	}
	UE_LOG(LogTemp, Warning, TEXT("[Dump] 动画实例共 %d 个"), LinkedInstanceCount);

	UAnimSequence* KwangSequence = LoadObject<UAnimSequence>(nullptr,
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang"));
	UAnimSequence* SourceSequence = LoadObject<UAnimSequence>(nullptr,
		TEXT("/ALS/ALS/Animations/Base/A_Als_Stand_Pose.A_Als_Stand_Pose"));

	auto DumpSequenceState = [](const TCHAR* Label, UAnimSequence* Sequence)
	{
		if (!Sequence)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Dump] %s：加载失败"), Label);
			return;
		}

		const IAnimationDataModel* Model = Sequence->GetDataModel();

		UE_LOG(LogTemp, Warning, TEXT("[Dump] %s：骨架=%s  加性=%d  压缩有效=%d  压缩过期=%d  数据模型=%s  轨道数=%d"),
			Label,
			*GetNameSafe(Sequence->GetSkeleton()),
			Sequence->IsValidAdditive() ? 1 : 0,
			Sequence->IsCompressedDataValid() ? 1 : 0,
			Sequence->IsCompressedDataOutOfDate() ? 1 : 0,
			Model ? TEXT("有效") : TEXT("空"),
			Model ? Model->GetNumBoneTracks() : -1);
	};

	DumpSequenceState(TEXT("候选 Kwang"), KwangSequence);
	DumpSequenceState(TEXT("候选曼尼昆"), SourceSequence);

	const FReferenceSkeleton& ReferenceSkeleton = MeshAsset->GetRefSkeleton();

	for (const FName& BoneName : BoneNames)
	{
		const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		const int32 ParentIndex = BoneIndex != INDEX_NONE ? ReferenceSkeleton.GetParentIndex(BoneIndex) : INDEX_NONE;
		if (ParentIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Dump] %s 不存在或没有父骨骼"), *BoneName.ToString());
			continue;
		}

		const FName ParentName = ReferenceSkeleton.GetBoneName(ParentIndex);

		// 世界空间反解父骨骼，等价于「骨骼相对父骨骼的局部位移」，且不受组件缩放影响
		const FTransform BoneWorld = Mesh->GetBoneTransform(BoneName, RTS_World);
		const FTransform ParentWorld = Mesh->GetBoneTransform(ParentName, RTS_World);
		const FVector LiveLocal = (ParentWorld.Inverse() * BoneWorld).GetTranslation();
		const float LiveWorldDistance = static_cast<float>(FVector::Dist(BoneWorld.GetTranslation(), ParentWorld.GetTranslation()));

		const FVector RefLocal = ReferenceSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();
		const FVector KwangLocal = GetSequenceFrame0Translation(KwangSequence, BoneName);
		const FVector SourceLocal = GetSequenceFrame0Translation(SourceSequence, BoneName);

		UE_LOG(LogTemp, Warning,
			TEXT("[Dump] %s  实测=(%.2f,%.2f,%.2f) |v|=%.2f 世界距=%.2f  参考=(%.2f,%.2f,%.2f)  Kwang首帧=(%.2f,%.2f,%.2f) |v|=%.2f  曼尼昆首帧=(%.2f,%.2f,%.2f) |v|=%.2f"),
			*BoneName.ToString(),
			LiveLocal.X, LiveLocal.Y, LiveLocal.Z, LiveLocal.Size(), LiveWorldDistance,
			RefLocal.X, RefLocal.Y, RefLocal.Z,
			KwangLocal.X, KwangLocal.Y, KwangLocal.Z, KwangLocal.Size(),
			SourceLocal.X, SourceLocal.Y, SourceLocal.Z, SourceLocal.Size());
	}
}

static FAutoConsoleCommand DumpKwangPoseCommand(
	TEXT("PP.DumpKwangPose"),
	TEXT("PIE 中把实测躯干局部位移向量和 Kwang 首帧 / 曼尼昆首帧逐项比对，确定拉伸的真正来源"),
	FConsoleCommandDelegate::CreateStatic(&DumpKwangPose)
);

// 强制重建：AlsKwang 里所有动画的压缩数据。
// 关键前提：CacheDerivedDataForCurrentPlatform 在 DataByPlatformKeyHash 已含当前 key 时会直接 return
// （见 AnimSequence.cpp BeginCacheDerivedData 的 early-out），所以必须先清掉内存里的旧压缩数据，
// 否则这条命令完全是空转。
// 症状对照：源数据（Kwang首帧）是 6.66/10.52/6.85/25.10/6.22，运行期世界距却是
// 10.84/19.24/13.38/16.56/9.29（曼尼昆），且 IsCompressedDataOutOfDate() 报 0——典型的
// 「压缩数据是按旧骨架烘焙的、但 key 已经换成新骨架」的过期缓存。
// 用法：在编辑器控制台执行  PP.RebuildKwangCompression，然后 File > Save All
static void RebuildKwangCompression()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DestFolder));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	UAnimSequence* StandPose = LoadObject<UAnimSequence>(nullptr,
		TEXT("/Game/PersonalProject/Animations/AlsKwang/A_Als_Stand_PoseKwang.A_Als_Stand_PoseKwang"));

	int32 SequenceCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UAnimSequence* Sequence = Cast<UAnimSequence>(AssetData.GetAsset());
		if (!Sequence || !Sequence->GetSkeleton())
		{
			continue;
		}

		Sequence->Modify();
		FKwangAnimSequenceAccess::ForceClearCompressionData(Sequence);
		Sequence->CacheDerivedDataForCurrentPlatform();
		Sequence->MarkPackageDirty();

		// 现做一次抽样自检：重建完立刻读回数据模型，确认数据模型本身是 Kwang 的比例
		if (Sequence == StandPose)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Compression] 自检 A_Als_Stand_PoseKwang：spine_01=%.2f spine_02=%.2f neck_01=%.2f（应为 6.66 / 10.52 / 25.10）"),
				GetSequenceFrame0Translation(Sequence, FName("spine_01")).Size(),
				GetSequenceFrame0Translation(Sequence, FName("spine_02")).Size(),
				GetSequenceFrame0Translation(Sequence, FName("neck_01")).Size());
		}

		++SequenceCount;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Compression] 已强制清空并重建 %d 个动画的压缩数据。请执行 File > Save All 落盘，然后重启 PIE 复测。"), SequenceCount);
}

static FAutoConsoleCommand RebuildKwangCompressionCommand(
	TEXT("PP.RebuildKwangCompression"),
	TEXT("重建 AlsKwang 里所有动画的压缩数据，避免继续使用按曼尼昆骨架烘焙的旧缓存"),
	FConsoleCommandDelegate::CreateStatic(&RebuildKwangCompression)
);

#else

static void Run()
{
}

#endif
}
