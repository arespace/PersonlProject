// Chen Tianbao uses GAS to create online battle games


#include "Characters/PP_BaseCharacter.h"

#include "AlsAnimationInstance.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "UObject/ConstructorHelpers.h"

namespace PersonalTags
{
	const FName Player = FName("Player");
}

APP_BaseCharacter::APP_BaseCharacter()
{
	// ALS 需要 Tick 驱动视图/移动刷新，不得禁用 PrimaryActorTick

	//Tick and refresh bone transforms whether rendered or not - for bone updates on a dedicated server
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 兜底加载 ALS 默认设置资产：改父类后蓝图配置会被清空，缺失会触发 ALS_ENSURE 且移动不刷新
	static ConstructorHelpers::FObjectFinder<UAlsCharacterSettings> DefaultCharacterSettings(
		TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
	if (DefaultCharacterSettings.Succeeded())
	{
		Settings = DefaultCharacterSettings.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAlsMovementSettings> DefaultMovementSettings(
		TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
	if (DefaultMovementSettings.Succeeded())
	{
		MovementSettings = DefaultMovementSettings.Object;
	}
}

void APP_BaseCharacter::PostInitializeComponents()
{
	// ===== ALS Settings/MovementSettings 最终兜底 =====
	// 蓝图属性在 C++ 构造函数之后应用——如果 BP_PP_PlayerCharacter 里 Settings 设成 null
	// 会覆盖掉构造函数的兜底。PostInitializeComponents 时机最晚，这里再补一次。
	if (!Settings)
	{
		static UAlsCharacterSettings* CachedDefaultCharSettings = LoadObject<UAlsCharacterSettings>(
			nullptr, TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
		Settings = CachedDefaultCharSettings;
		UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] Settings was NULL — applied fallback CS_Als_Default"));
	}

	if (!MovementSettings)
	{
		static UAlsMovementSettings* CachedDefaultMoveSettings = LoadObject<UAlsMovementSettings>(
			nullptr, TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
		MovementSettings = CachedDefaultMoveSettings;
		UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] MovementSettings was NULL — applied fallback MS_Als_Normal"));
	}

	// ===== 网格体与动画蓝图兜底 =====
	// 必须在 Super::PostInitializeComponents 之前设置好，因为 AAlsCharacter 会在那里
	// 执行 Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance()) 来设置 AnimationInstance。
	// 如果蓝图里配了非 ALS 的 Anim BP（如 Greystone_AnimBlueprint），AnimationInstance 就是 null，
	// ALS Tick 守卫 !AnimationInstance.IsValid() 会直接 return，RefreshView 永远不跑。

	// 启用 ALS 动画蓝图的角色（玩家）必须用曼尼昆骨架的 SKM_Als，因为 AB_Als 是基于
	// 曼尼昆骨架制作的。若蓝图里残留 Greystone 等其他骨架的网格，套用 AB_Als 会导致
	// 骨骼比例错位、躯干拉长、武器位置异常。因此对 bForceAlsAnimationBlueprint 的角色
	// 无条件强制 SKM_Als，不依赖 ApplyCharacterAppearance 是否被调用。
	if (bForceAlsAnimationBlueprint)
	{
		static USkeletalMesh* CachedAlsMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
		if (CachedAlsMesh && GetMesh()->GetSkeletalMeshAsset() != CachedAlsMesh)
		{
			// 玩家强制 ALS 网格属于预期行为，不是配置错误，降级为 Verbose 避免每次 spawn 刷屏
			UE_LOG(LogTemp, Verbose, TEXT("[ALS_FIX] Overriding Mesh %s → SKM_Als"),
				*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()));
			GetMesh()->SetSkeletalMesh(CachedAlsMesh);
		}
	}
	else if (!GetMesh()->GetSkeletalMeshAsset())
	{
		static USkeletalMesh* CachedDefaultMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
		if (CachedDefaultMesh)
		{
			GetMesh()->SetSkeletalMesh(CachedDefaultMesh);
		}
	}

	// 仅在 bForceAlsAnimationBlueprint 时套用 ALS 动画蓝图。
	// 不能用"无条件强制"或"仅判空兜底"：
	//  - 无条件强制：敌人（Minion 骨架）也会被套上曼尼昆骨架的 AB_Als，动作比例错位；
	//  - 仅判空兜底：蓝图里残留的非 ALS Anim BP（如 Greystone_AnimBlueprint）会让
	//    AAlsCharacter::PostInitializeComponents 里的 Cast<UAlsAnimationInstance> 失败，
	//    AnimationInstance 为 null，ALS Tick 直接 return，相机与视角全部失效。
	if (bForceAlsAnimationBlueprint)
	{
		static UClass* CachedDefaultAnimClass = LoadClass<UAnimInstance>(
			nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
		if (CachedDefaultAnimClass && GetMesh()->GetAnimClass() != CachedDefaultAnimClass)
		{
			// 同上：玩家强制 AB_Als 是预期行为，降级为 Verbose
			UE_LOG(LogTemp, Verbose, TEXT("[ALS_FIX] Overriding AnimClass %s → AB_Als"),
				*GetNameSafe(GetMesh()->GetAnimClass()));
			GetMesh()->SetAnimInstanceClass(CachedDefaultAnimClass);
		}
	}

	Super::PostInitializeComponents();

	// ===== 确保 ALS BeginPlay 的 ensure 通过 =====
	// AAlsCharacter::BeginPlay 会 ensure bUseControllerRotationPitch/Yaw/Roll 全部为 false。
	// 如果蓝图从旧父类（ACharacter）继承时保存了非默认值，或敌人转向逻辑误设了 Yaw=true，
	// 第一次 PIE 就会断在 ensure 上。这里在 Super::PostInit 之后统一清零。
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 未启用 ALS 动画的角色（敌人）需要手动驱动朝向。
	// ALS 构造函数里 bUseControllerRotationYaw=false，且朝向由 RefreshGroundedRotation 在 Tick 中驱动；
	// 但这些角色没有 UAlsAnimationInstance，AAlsCharacter::Tick 会提前 return，
	// 若不手动转向，敌人移动时不会自动转向，出现滑步/朝向错乱。
	// 注意：不能用 bUseControllerRotationYaw=true（会触发上述 ensure），
	// 改为在 Tick 里手动把 Actor Yaw 对齐到 Controller->ControlRotation.Yaw（AI MoveTo 会设它）。
	if (!bForceAlsAnimationBlueprint)
	{
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			CM->bUseControllerDesiredRotation = false;
			CM->bOrientRotationToMovement = false;
			CM->RotationRate = FRotator(0.f, 540.f, 0.f);
		}
	}
}

void APP_BaseCharacter::BeginPlay()
{
	// 对于不使用 ALS 动画蓝图的角色（敌人），GetMesh()->GetAnimInstance() 不是
	// UAlsAnimationInstance，AAlsCharacter::PostInitializeComponents 里 Cast 失败，
	// AnimationInstance 为 null。AAlsCharacter::BeginPlay 的 ensure(AnimationInstance.IsValid())
	// 会崩溃。这里临时赋一个 UAlsAnimationInstance 的 CDO 让 ensure 通过，
	// Super::BeginPlay 之后立即清空，使 ALS Tick 的 !AnimationInstance.IsValid() 守卫
	// 跳过 ALS 逻辑（敌人不需要 ALS 驱动）。
	if (!bForceAlsAnimationBlueprint && !AnimationInstance.IsValid())
	{
		static UClass* CachedAlsAnimClass = LoadClass<UAnimInstance>(
			nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
		if (CachedAlsAnimClass)
		{
			AnimationInstance = Cast<UAlsAnimationInstance>(CachedAlsAnimClass->GetDefaultObject());
		}
	}

	Super::BeginPlay();

	// 敌人清空 AnimationInstance，让 ALS Tick 守卫跳过
	if (!bForceAlsAnimationBlueprint)
	{
		AnimationInstance.Reset();
	}
}

void APP_BaseCharacter::PostNetReceiveLocationAndRotation()
{
	// 玩家：保持 ALS 行为（模拟代理忽略服务端旋转，ALS 自行驱动）
	if (bForceAlsAnimationBlueprint)
	{
		Super::PostNetReceiveLocationAndRotation();
		return;
	}

	// 敌人：跳过 ALS 的忽略逻辑。ACharacter 版本只做位置平滑，朝向在下面显式接受。
	ACharacter::PostNetReceiveLocationAndRotation();

	// 模拟代理的朝向必须手动应用：ALS 会丢弃服务端旋转，而 ACharacter 的平滑路径在
	// 站桩（速度为 0、位置无变化）时不会落实旋转，导致客户端敌人永远保持生成朝向。
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		const FRotator RepRotation = GetReplicatedMovement().Rotation;
		SetActorRotation(FRotator(0.f, RepRotation.Yaw, 0.f));
	}
}

void APP_BaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 没有启用 ALS 动画的角色（敌人）：ALS Tick 因没有 UAlsAnimationInstance 提前 return，
	// 不会驱动朝向；标准角色的 bOrientRotationToMovement 也被 ALS 强制关闭。
	// 这里手动驱动 Yaw：
	//  - 移动中：对齐到水平速度方向（AIController 的 MoveTo 不更新 ControlRotation，不能用它）；
	//  - 静止时（远程站桩攻击）：对齐到 AIController 的 Focus 目标（由 SearchForTarget 设置）。
	// 不能用 bUseControllerRotationYaw=true，会触发 ALS BeginPlay 的 ensure。
	// 朝向只在服务端驱动；客户端模拟代理直接采用服务端复制的旋转
	// （见 PostNetReceiveLocationAndRotation），避免两边各转各的。
	if (!bForceAlsAnimationBlueprint && GetLocalRole() == ROLE_Authority)
	{
		const UCharacterMovementComponent* CM = GetCharacterMovement();
		if (!CM) return;

		float TargetYaw;
		if (CM->Velocity.Size2D() > 10.f)
		{
			TargetYaw = CM->Velocity.Rotation().Yaw;
		}
		else if (const AAIController* AICon = Cast<AAIController>(GetController()))
		{
			AActor* Focus = AICon->GetFocusActor();
			if (!Focus) return;
			TargetYaw = (Focus->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		}
		else
		{
			return;
		}

		FRotator Rotation = GetActorRotation();
		Rotation.Yaw = FMath::FInterpTo(Rotation.Yaw, TargetYaw, DeltaTime, 10.f);
		SetActorRotation(Rotation);
	}
}

void APP_BaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bAlive);
}

UAbilitySystemComponent* APP_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}


void APP_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	for (const auto& Ability	: StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void APP_BaseCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect) , TEXT("InitializeAttributesEffect not set."));
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APP_BaseCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	if (AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
	else if (AttributeChangeData.NewValue < AttributeChangeData.OldValue)
	{
		// 血量下降且未死亡 → 触发受击反应
		PlayHitReaction();
	}
}

void APP_BaseCharacter::HandleDeath()
{
	bAlive = false;

	// 停止受击反应计时，避免复活后残留 Injured 覆盖层
	GetWorldTimerManager().ClearTimer(HitReactionTimerHandle);
	SetOverlayMode(AlsOverlayModeTags::Default);

	// ALS 物理倒地：服务端调用后 ALS 内部会 Multicast 到所有客户端
	StartRagdolling();

	// 死亡后不再接受移动/碰撞推动
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 玩家死亡：停止输入（视角/移动）。敌人由 AI 控制器自行处理。
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// 服务端启动复活计时（复活不再走死亡 GA 的蒙太奇回调，会被 Ragdoll 卡死）
	if (HasAuthority() && ShouldRespawn())
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ThisClass::ServerRespawn, RespawnDelay, false);
	}
}

void APP_BaseCharacter::ServerRespawn()
{
	if (bAlive) return; // 已被 GA 侧复活（双保险）

	// 传送回出生点（服务端权威）
	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			if (AActor* PlayerStart = GameMode->FindPlayerStart(GetController()))
			{
				SetActorLocation(PlayerStart->GetActorLocation(), false);
			}
		}
	}

	HandleRespawn(); // 内含 ResetAttributes

	// 通知 owning 客户端恢复输入（服务端上的 EnableInput 无效）
	ClientOnRespawn();
}

void APP_BaseCharacter::ClientOnRespawn_Implementation()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}
}

void APP_BaseCharacter::HandleRespawn()
{
	bAlive = true;

	// ALS 起身：内部会播放 GetUp 蒙太奇，服务端调用后 Multicast
	StopRagdolling();

	// 恢复移动与碰撞
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 玩家复活：恢复输入
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}

	ResetAttributes();
}

void APP_BaseCharacter::PlayHitReaction()
{
	// 死亡/Ragdoll 状态下不播放受击反应。
	// ALS 用 LocomotionAction == Ragdolling 标记当前是否处于倒地状态
	// （见 AAlsCharacter::IsRagdollingAllowedToStop）。
	if (!bAlive || GetLocomotionAction() == AlsLocomotionActionTags::Ragdolling) return;

	SetOverlayMode(AlsOverlayModeTags::Injured);

	GetWorldTimerManager().ClearTimer(HitReactionTimerHandle);
	GetWorldTimerManager().SetTimer(
		HitReactionTimerHandle, this, &ThisClass::ResetHitReactionOverlay, 0.35f, false);
}

void APP_BaseCharacter::ResetHitReactionOverlay()
{
	if (!bAlive || GetLocomotionAction() == AlsLocomotionActionTags::Ragdolling) return;
	SetOverlayMode(AlsOverlayModeTags::Default);
}

void APP_BaseCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffect) , TEXT("ResetAttributesEffect not set."));
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(ResetAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

