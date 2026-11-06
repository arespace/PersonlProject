

#include "Characters/PP_BaseCharacter.h"

#include "AbilitySystem/PP_AttributeSet.h"
#include "GameplayTags/PPTags.h"
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
#include "PhysicsEngine/PhysicsAsset.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "Framework/PP_Log.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsConstants.h"

namespace PPActorTags
{
	const FName Player = FName("Player");
}

namespace
{
	/**
	 * ALS 的 StartRagdolling 要求物理资产里同时存在 pelvis / spine_03 两个刚体，
	 * 否则会触发 ALS_ENSURE。部分 Paragon 网格不满足该条件，
	 * 这里提前检查，避免死亡时每次都在调试器里断下。
	 */
	bool CanStartRagdolling(const USkeletalMeshComponent* Mesh)
	{
		return IsValid(Mesh)
			&& Mesh->GetBodyInstance(UAlsConstants::PelvisBoneName()) != nullptr
			&& Mesh->GetBodyInstance(UAlsConstants::Spine03BoneName()) != nullptr;
	}
}

bool APP_BaseCharacter::CanBeDamaged() const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(PPTags::Status::Invulnerable)
			|| ASC->HasMatchingGameplayTag(PPTags::Status::Dead)
			|| ASC->HasMatchingGameplayTag(PPTags::Status::BossPhaseTransition))
		{
			return false;
		}
	}
	return Super::CanBeDamaged();
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

	// ===== 关闭 ALS 的自动 Ragdoll 入口（不改插件源码）=====
	// ALS 默认在"高处落地"（Ragdolling.bStartRagdollingOnLand）和"翻滚中离地"
	// （Rolling.bInterruptRollingWhenInAir）时自动 StartRagdolling。
	// 它的前置检查 IsRagdollingAllowedToStart() 用的是 ALS_ENSURE_MESSAGE，
	// 本项目角色网格的物理资产普遍没有 pelvis / spine_03 刚体，一旦触发就会在
	// Development 构建里被调试器断下（实测冻结约 4.89 秒，表现为游戏一帧一帧）。
	// 这里在运行时关掉这两个自动入口；玩家死亡路径见 HandleDeath 的同等检查。
	if (Settings)
	{
		if (Settings->Ragdolling.bStartRagdollingOnLand || Settings->Rolling.bInterruptRollingWhenInAir)
		{
			UE_LOG(LogPP, Warning,
				TEXT("[ALS_FIX] 关闭自动 Ragdoll：落地=%d→0 翻滚离地=%d→0（物理资产缺少 pelvis/spine_03）"),
				Settings->Ragdolling.bStartRagdollingOnLand ? 1 : 0,
				Settings->Rolling.bInterruptRollingWhenInAir ? 1 : 0);
		}

		Settings->Ragdolling.bStartRagdollingOnLand = false;
		Settings->Rolling.bInterruptRollingWhenInAir = false;
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
	// 复活诊断窗口内：记录服务端复制造成的位移跳变。
	// 用来区分卡顿来自"本地预测回弹"还是"服务端把角色拉回"。
	if (RespawnMovementDebugEndTime > 0.f && IsLocallyControlled())
	{
		const FVector RepLocation = GetReplicatedMovement().Location;
		const float JumpDistance = FVector::Dist(GetActorLocation(), RepLocation);
		if (JumpDistance > 20.f)
		{
			UE_LOG(LogPP, Warning,
				TEXT("[PP_RespawnMove] 服务端校正: 位移=%.1f 当前=(%.1f, %.1f, %.1f) → 复制=(%.1f, %.1f, %.1f) 速度=%.1f"),
				JumpDistance,
				GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
				RepLocation.X, RepLocation.Y, RepLocation.Z,
				GetCharacterMovement() ? GetCharacterMovement()->Velocity.Size() : -1.f);
		}
	}

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

	// 复活后移动诊断：客户端本地角色 2 秒内每 0.1 秒采样一次
	if (RespawnMovementDebugEndTime > 0.f && IsLocallyControlled())
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now >= RespawnMovementDebugNextLogTime && Now <= RespawnMovementDebugEndTime)
		{
			RespawnMovementDebugNextLogTime = Now + 0.1f;

			const UCharacterMovementComponent* CM = GetCharacterMovement();
			const FVector InputVector = CM ? CM->GetLastInputVector() : FVector::ZeroVector;
			UE_LOG(LogPP, Warning,
				TEXT("[PP_RespawnMove] t=%.2f 位置=(%.1f, %.1f, %.1f) 速度=%.1f 加速度=(%.1f, %.1f, %.1f) 输入=(%.1f, %.1f, %.1f) 模式=%d 地面=%d 传送=%d 忽略移动=%d 忽略视角=%d 胶囊碰撞=%d"),
				Now,
				GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
				CM ? CM->Velocity.Size() : -1.f,
				CM ? CM->GetCurrentAcceleration().X : -1.f,
				CM ? CM->GetCurrentAcceleration().Y : -1.f,
				CM ? CM->GetCurrentAcceleration().Z : -1.f,
				InputVector.X, InputVector.Y, InputVector.Z,
				CM ? static_cast<int32>(CM->MovementMode) : -1,
				CM && CM->IsMovingOnGround() ? 1 : 0,
				CM && CM->bJustTeleported ? 1 : 0,
				GetController() && Cast<APlayerController>(GetController())->IsMoveInputIgnored() ? 1 : 0,
				GetController() && Cast<APlayerController>(GetController())->IsLookInputIgnored() ? 1 : 0,
				GetCapsuleComponent() ? static_cast<int32>(GetCapsuleComponent()->GetCollisionEnabled()) : -1);

			// 复活落地窗口内持续校正 Dead 残留：客户端本地清 GE、服务端复制、死亡能力的
			// ActivationOwnedTags 三者可能乱序，Dead 标签会在复活后又冒出来。
			// 这里在 3 秒窗口内每 0.1 秒兜底清一次（同时取消死亡能力，避免它继续持有标签）。
			if (bAlive)
			{
				if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
					ASC && ASC->HasMatchingGameplayTag(PPTags::Status::Dead))
				{
					// 死亡能力的 AbilityTags 是 PPTags.PPAbilities.Death，只在蓝图资产里配置，
					// C++ 侧没有对应常量，按名字取一次；取不到就只清 GE。
					const FGameplayTag DeathAbilityTag =
						FGameplayTag::RequestGameplayTag(TEXT("PPTags.PPAbilities.Death"), false);
					if (DeathAbilityTag.IsValid())
					{
						FGameplayTagContainer DeathAbilityTags;
						DeathAbilityTags.AddTag(DeathAbilityTag);
						ASC->CancelAbilities(&DeathAbilityTags);
					}

					ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));
					UE_LOG(LogPP, Warning,
						TEXT("[PP_RespawnFlow] 复活窗口内清理残留 Dead: %s 清理后 DeadTag=%d"),
						*GetNameSafe(this), ASC->HasMatchingGameplayTag(PPTags::Status::Dead) ? 1 : 0);
				}
			}
		}

		if (Now > RespawnMovementDebugEndTime)
		{
			RespawnMovementDebugEndTime = -1.f;
			UE_LOG(LogPP, Warning, TEXT("[PP_RespawnMove] 采样结束: %s"), *GetNameSafe(this));
		}
	}

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

	for (const auto& Ability : StartupAbilities)
	{
		if (!Ability)
		{
			continue;
		}

		// 去重：GameMode 的 CharacterAbilityMap 与角色的 StartupAbilities 可能配置同一个技能，
		// 重复授予后一次按键会同时激活多份同一技能的实例（日志里的"匹配能力数=2"），
		// 每份实例都会各扣一次体力，表现为"复活后只能用两次技能"。
		if (GetAbilitySystemComponent()->FindAbilitySpecFromClass(Ability))
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 跳过重复授予: %s 已存在"), *Ability->GetName());
			continue;
		}

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
	// 服务端一次死亡可能进入两次（属性委托重复绑定 / 同帧多来源伤害结算）。
	// 第二次进入时角色已经死亡：重复执行会再次启动复活计时、DisableMovement/DisableInput，
	// 这里直接忽略并保留一条日志便于确认。
	if (HasAuthority() && !bAlive)
	{
		UE_LOG(LogPP, Warning,
			TEXT("[PP_DeathFlow] HandleDeath 重复触发已忽略: %s 本机=%d LocalRole=%d"),
			*GetNameSafe(this), IsLocallyControlled() ? 1 : 0, static_cast<int32>(GetLocalRole()));
		return;
	}

	// 排错：死亡瞬间把会影响移动/输入的上下文全部打出来（服务端与客户端各打一条）
	UE_LOG(LogPP, Warning,
		TEXT("[PP_DeathFlow] HandleDeath 开始: %s 本机=%d LocalRole=%d 控制者=%s bAlive=%d 移动模式=%d LocomotionAction=%s 物理资产=%s Pelvis=%d Spine03=%d"),
		*GetNameSafe(this), IsLocallyControlled() ? 1 : 0, static_cast<int32>(GetLocalRole()),
		*GetNameSafe(GetController()), bAlive ? 1 : 0,
		GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
		*GetLocomotionAction().ToString(),
		*GetNameSafe(GetMesh() ? GetMesh()->GetPhysicsAsset() : nullptr),
		GetMesh() && GetMesh()->GetBodyInstance(UAlsConstants::PelvisBoneName()) ? 1 : 0,
		GetMesh() && GetMesh()->GetBodyInstance(UAlsConstants::Spine03BoneName()) ? 1 : 0);

	bAlive = false;

	// 停止受击反应计时，避免复活后残留 Injured 覆盖层
	GetWorldTimerManager().ClearTimer(HitReactionTimerHandle);
	SetOverlayMode(AlsOverlayModeTags::Default);

	// ALS 物理倒地：服务端调用后 ALS 内部会 Multicast 到所有客户端。
	// 物理资产缺少 pelvis / spine_03 时跳过，避免 ALS_ENSURE 中断调试。
	if (CanStartRagdolling(GetMesh()))
	{
		StartRagdolling();
	}
	else
	{
		UE_LOG(LogPP, Warning,
			TEXT("[PP_Death] %s 的物理资产缺少 pelvis/spine_03 刚体，跳过 Ragdoll（物理资产=%s）"),
			*GetNameSafe(this), *GetNameSafe(GetMesh() ? GetMesh()->GetPhysicsAsset() : nullptr));
	}

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

	// 排错：确认死亡后的最终状态（移动是否被禁用、输入是否被屏蔽、复活计时是否生效）
	const APlayerController* PC{Cast<APlayerController>(GetController())};
	UE_LOG(LogPP, Warning,
		TEXT("[PP_DeathFlow] HandleDeath 完成: %s 移动模式=%d 胶囊碰撞=%d 输入忽略(移动=%d 视角=%d) 复活计时器=%d"),
		*GetNameSafe(this),
		GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
		static_cast<int32>(GetCapsuleComponent()->GetCollisionEnabled()),
		PC && PC->IsMoveInputIgnored() ? 1 : 0,
		PC && PC->IsLookInputIgnored() ? 1 : 0,
		RespawnTimerHandle.IsValid() ? 1 : 0);
}

void APP_BaseCharacter::RespawnCharacter()
{
	ServerRespawn();
}

void APP_BaseCharacter::ServerRespawn()
{
	if (bAlive)
	{
		// 排错：说明复活已经由死亡 GA 的路径先完成，服务端计时器只是空转
		UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] ServerRespawn 跳过：%s 已经复活（GA 路径先行）"),
			*GetNameSafe(this));
		return;
	}

	// 传送回重生点（服务端权威）：玩家优先使用篝火记录的 PlayerState 坐标
	FTransform RespawnTransform;
	if (GetRespawnTransform(RespawnTransform))
	{
		// 把落点贴到地面：PlayerStart 或旧篝火点的高度可能让胶囊底部嵌进地板，
		// 移动时反复解穿透会造成顿挫。用向下射线找地面，再按胶囊半高抬高。
		FVector RespawnLocation = RespawnTransform.GetLocation();
		if (UWorld* World = GetWorld())
		{
			FVector TraceStart = RespawnLocation + FVector(0.f, 0.f, 150.f);
			FVector TraceEnd = RespawnLocation - FVector(0.f, 0.f, 700.f);
			FHitResult FloorHit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(RespawnGround), false, this);
			if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, Params))
			{
				const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
				RespawnLocation.Z = FloorHit.Location.Z + CapsuleHalfHeight + 2.f;
				RespawnTransform.SetLocation(RespawnLocation);
			}
		}

		// 记录落点，HandleRespawn 会把它同步给 owning 客户端，让客户端本地也做一次 Teleport
		PendingRespawnLocation = RespawnTransform.GetLocation();
		PendingRespawnYaw = RespawnTransform.Rotator().Yaw;
		bHasPendingRespawnLocation = true;

		// 必须用 TeleportPhysics：默认的 Teleport=None 会让客户端把这次大位移
		// 当成普通移动做网络平滑，复活后第一秒里表现为角色移动抖动。
		SetActorLocation(RespawnTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		SetActorRotation(FRotator(0.f, RespawnTransform.Rotator().Yaw, 0.f));

		UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] 复活瞬移: %s → 重生点 (%.0f, %.0f, %.0f)"),
			*GetNameSafe(this),
			RespawnTransform.GetLocation().X, RespawnTransform.GetLocation().Y, RespawnTransform.GetLocation().Z);
	}

	HandleRespawn(); // 内含 ResetAttributes
}

bool APP_BaseCharacter::GetRespawnTransform(FTransform& OutTransform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode)
	{
		return false;
	}

	AActor* PlayerStart = GameMode->FindPlayerStart(GetController());
	if (!PlayerStart)
	{
		return false;
	}

	OutTransform = PlayerStart->GetActorTransform();
	return true;
}

void APP_BaseCharacter::ClientOnRespawn_Implementation(FVector RespawnLocation, float RespawnYaw)
{
	APlayerController* PC{Cast<APlayerController>(GetController())};
	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] 客户端收到 ClientOnRespawn: %s 本机=%d 控制者=%s 落点=(%.0f, %.0f, %.0f)"),
		*GetNameSafe(this), IsLocallyControlled() ? 1 : 0, *GetNameSafe(PC),
		RespawnLocation.X, RespawnLocation.Y, RespawnLocation.Z);

	// 客户端本地先零掉死亡残留速度，再做一次 Teleport，与服务端落点保持一致。
	// 否则客户端会按死亡点的旧位置做一帧移动预测，随后被服务端拉回，表现为复活后移动卡顿/回弹。
	const bool bDeadTagAtEntry = GetAbilitySystemComponent()
		&& GetAbilitySystemComponent()->HasMatchingGameplayTag(PPTags::Status::Dead);
	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] ClientOnRespawn 入口: bAlive=%d DeadTag=%d 位置=(%.1f, %.1f, %.1f) 速度=%.1f"),
		bAlive ? 1 : 0, bDeadTagAtEntry ? 1 : 0,
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
		GetCharacterMovement() ? GetCharacterMovement()->Velocity.Size() : -1.f);

	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->StopMovementImmediately();
		CM->Velocity = FVector::ZeroVector;
		CM->SetMovementMode(MOVE_Walking);
	}

	SetActorLocation(RespawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, RespawnYaw, 0.f));

	// 客户端在 HandleDeath 里关掉了胶囊碰撞，而碰撞状态不会从服务端复制，
	// 必须在这里恢复：否则客户端本地移动预测不参与碰撞，服务端却照常挡住，
	// 每帧都会被服务端校正（bJustTeleported 长期为 1），表现就是复活后持续卡顿。
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] 客户端恢复胶囊碰撞: %s 碰撞=%d"),
			*GetNameSafe(this), static_cast<int32>(Capsule->GetCollisionEnabled()));
	}

	// 客户端本地执行 HandleDeath 会写 bAlive=false，而服务端的 true 值不一定会再复制一次；
	// 这里以服务端这条 Client RPC 的语义为准本地记存活，否则 PlayerController::IsAlive()
	// 会一直为 false，技能按键会被直接拦截。
	bAlive = true;

	// 客户端本地清掉 Status.Dead 的 GE：死亡 GE 是在本地预测链路里加上的，
	// 只靠服务端移除有时不会及时同步过来，客户端 ASC 会残留 Dead 标签导致技能无法激活。
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));

		// 排错：确认客户端本地的 Dead 标签是否真的被清掉（残留会导致技能无法激活）
		FGameplayTagContainer RemainingTags;
		ASC->GetOwnedGameplayTags(RemainingTags);
		UE_LOG(LogPP, Warning,
			TEXT("[PP_RespawnFlow] 客户端清理 Dead 后: DeadTag=%d 拥有标签=[%s]"),
			ASC->HasMatchingGameplayTag(PPTags::Status::Dead) ? 1 : 0,
			*RemainingTags.ToStringSimple());

		// Dead 标签若仍存在，说明它不是由 GE 提供，而是由仍处于激活状态的死亡能力
		// （ActivationOwnedTags）持有：移除 GE 清不掉能力持有的标签，必须先取消能力。
		if (ASC->HasMatchingGameplayTag(PPTags::Status::Dead))
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.IsActive())
				{
					UE_LOG(LogPP, Warning,
						TEXT("[PP_RespawnFlow] 客户端残留激活能力: %s 实例=%s ActiveCount=%d"),
						*GetNameSafe(Spec.Ability), *GetNameSafe(Spec.GetPrimaryInstance()), Spec.ActiveCount);
				}
			}

			ASC->CancelAllAbilities();
			ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));

			UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] 客户端取消激活能力后: DeadTag=%d"),
				ASC->HasMatchingGameplayTag(PPTags::Status::Dead) ? 1 : 0);
		}
	}

	// 复活后 3 秒移动诊断采样，日志前缀 [PP_RespawnMove]
	RespawnMovementDebugEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 3.f : -1.f;
	RespawnMovementDebugNextLogTime = 0.f;

	if (PC)
	{
		// 视角 Yaw 对齐落点朝向，避免沿用死亡时锁定敌人的控制旋转
		FRotator ControlRotation{PC->GetControlRotation()};
		ControlRotation.Yaw = RespawnYaw;
		PC->SetControlRotation(ControlRotation);

		// 死亡链路可能多次 DisableInput（服务端/客户端/GA 各一次），忽略计数会残留 >0，
		// 单次 EnableInput 不一定清零；这里强制复位，否则复活后移动输入仍被部分屏蔽。
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		EnableInput(PC);
	}

	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] 客户端 ClientOnRespawn 完成: %s bAlive=%d 输入忽略(移动=%d 视角=%d) 速度=%.0f 移动模式=%d"),
		*GetNameSafe(this), bAlive ? 1 : 0,
		PC && PC->IsMoveInputIgnored() ? 1 : 0,
		PC && PC->IsLookInputIgnored() ? 1 : 0,
		GetCharacterMovement() ? GetCharacterMovement()->Velocity.Size() : -1.f,
		GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1);
}

void APP_BaseCharacter::HandleRespawn()
{
	// 复活只允许在服务端执行。死亡 GA 的回调在客户端也会触发 HandleRespawn，
	// 如果客户端也执行，会把已经复活并开始移动的 Pawn 再次 StopMovement（速度清零），
	// 表现为复活后跑一步就被卡住、之后才恢复。
	if (!HasAuthority())
	{
		UE_LOG(LogPP, Log, TEXT("[PP_RespawnFlow] 客户端忽略 HandleRespawn: %s"), *GetNameSafe(this));
		return;
	}

	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] HandleRespawn 开始: %s 本机=%d LocalRole=%d 控制者=%s 移动模式=%d LocomotionAction=%s"),
		*GetNameSafe(this), IsLocallyControlled() ? 1 : 0, static_cast<int32>(GetLocalRole()),
		*GetNameSafe(GetController()),
		GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
		*GetLocomotionAction().ToString());

	bAlive = true;

	// 兜底：死亡能力若因为蒙太奇回调丢失而一直保持激活，它的 ActivationOwnedTags
	// 会一直持有 Status.Dead（移除 GE 清不掉），服务端技能同样会被永久封锁。
	// 这里仅在标签确实残留时取消所有激活能力，然后重新清一次 Dead GE。
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(PPTags::Status::Dead))
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.IsActive())
				{
					UE_LOG(LogPP, Warning,
						TEXT("[PP_RespawnFlow] 服务端残留激活能力: %s 实例=%s ActiveCount=%d"),
						*GetNameSafe(Spec.Ability), *GetNameSafe(Spec.GetPrimaryInstance()), Spec.ActiveCount);
				}
			}

			ASC->CancelAllAbilities();
			ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));

			UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] 服务端取消激活能力后: DeadTag=%d"),
				ASC->HasMatchingGameplayTag(PPTags::Status::Dead) ? 1 : 0);
		}
	}

	// ALS 起身：内部会播放 GetUp 蒙太奇，服务端调用后 Multicast
	StopRagdolling();

	// 清掉死亡期间残留的速度与待发送的移动输入。
	// 否则复活后第一秒会把旧速度/网络校正插值出来，表现为角色移动抖动，
	// 且锁定状态下死亡时角色常处于冲刺速度，抖动更明显。
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_RespawnFlow] 清空残留速度: %s 速度=%.0f 移动模式=%d"),
			*GetNameSafe(this), CM->Velocity.Size(), static_cast<int32>(CM->MovementMode));

		CM->StopMovementImmediately();
		CM->Velocity = FVector::ZeroVector;
	}

	// 视角与角色朝向对齐：死亡前若锁定了敌人，ControlRotation 会停留在敌人方向，
	// 复活后角色会朝着旧方向转身/移动，看起来像抖一下。这里把视角 Yaw 拉回出生朝向。
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			FRotator ControlRotation{PC->GetControlRotation()};
			ControlRotation.Yaw = GetActorRotation().Yaw;
			PC->SetControlRotation(ControlRotation);
		}
	}

	// 恢复移动与碰撞
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 玩家复活：恢复输入（本机 PlayerController 同时复位忽略计数，listen server 也走这里）
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			PC->ResetIgnoreMoveInput();
			PC->ResetIgnoreLookInput();
		}
		EnableInput(PC);
	}

	// 通知 owning 客户端恢复输入，并把服务端落点同步给客户端做本地 Teleport。
	// 服务端实例上的 DisableInput/EnableInput 不影响远程客户端，
	// 且复活可能由死亡 GA 或 ServerRespawn 两条路径触发，必须在这里统一兜底。
	// 仅玩家控制器才发 Client RPC，避免敌人复活时对无 owning client 的 AI 发送。
	if (HasAuthority() && Cast<APlayerController>(GetController()))
	{
		const FVector ClientLocation = bHasPendingRespawnLocation ? PendingRespawnLocation : GetActorLocation();
		const float ClientYaw = bHasPendingRespawnLocation ? PendingRespawnYaw : GetActorRotation().Yaw;
		ClientOnRespawn(ClientLocation, ClientYaw);
		bHasPendingRespawnLocation = false;
	}

	ResetAttributes();

	// 排错：确认复活后的最终状态
	const APlayerController* PC{Cast<APlayerController>(GetController())};
	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] HandleRespawn 完成: %s 移动模式=%d 胶囊碰撞=%d 输入忽略(移动=%d 视角=%d) LocomotionAction=%s"),
		*GetNameSafe(this),
		GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
		static_cast<int32>(GetCapsuleComponent()->GetCollisionEnabled()),
		PC && PC->IsMoveInputIgnored() ? 1 : 0,
		PC && PC->IsLookInputIgnored() ? 1 : 0,
		*GetLocomotionAction().ToString());
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
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	UPP_AttributeSet* AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(ASC) || !IsValid(AttributeSet))
	{
		return;
	}

	// 旧实现是 Apply ResetAttributesEffect。该 GE 的 MaxStamina 修饰符是 Add，
	// 每次休息/复活都会让耐力上限 +100 且叠加，并且可能留下阻塞技能的效果实例。
	// 先清掉该 GE 的所有活跃实例，再把三项资源直接设满，保证幂等。
	if (IsValid(ResetAttributesEffect))
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(ResetAttributesEffect, nullptr);
	}

	UE_LOG(LogPP, Warning,
		TEXT("[PP_Attributes] ResetAttributes 前: %s 生命=%.0f/%.0f 魔力=%.0f/%.0f 耐力=%.0f/%.0f"),
		*GetNameSafe(this),
		AttributeSet->GetHealth(), AttributeSet->GetMaxHealth(),
		AttributeSet->GetMana(), AttributeSet->GetMaxMana(),
		AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());

	ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), AttributeSet->GetMaxHealth());
	ASC->SetNumericAttributeBase(AttributeSet->GetManaAttribute(), AttributeSet->GetMaxMana());
	ASC->SetNumericAttributeBase(AttributeSet->GetStaminaAttribute(), AttributeSet->GetMaxStamina());

	UE_LOG(LogPP, Warning,
		TEXT("[PP_Attributes] ResetAttributes 后: %s 生命=%.0f/%.0f 魔力=%.0f/%.0f 耐力=%.0f/%.0f"),
		*GetNameSafe(this),
		AttributeSet->GetHealth(), AttributeSet->GetMaxHealth(),
		AttributeSet->GetMana(), AttributeSet->GetMaxMana(),
		AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());
}

