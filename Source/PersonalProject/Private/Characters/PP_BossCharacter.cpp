// Chen Tianbao uses GAS to create online battle games

#include "Characters/PP_BossCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Framework/PP_Log.h"
#include "GameFramework/Character.h"
#include "GameplayTags/PPTags.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

APP_BossCharacter::APP_BossCharacter()
{
	// Boss 默认更大的胶囊（蓝图里可覆盖）
	GetCapsuleComponent()->InitCapsuleSize(60.f, 120.f);

	bShouldRespawn = false; // Boss 不复活
	GoldDropAmount = 500;   // 敌人基类里是 10，Boss 默认多给，实际再乘 BossGoldMultiplier
	BossPhaseThresholds = {0.75f, 0.50f, 0.25f};
}

void APP_BossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_BossCharacter, BossName);
	DOREPLIFETIME(APP_BossCharacter, CurrentPhase);
	DOREPLIFETIME(APP_BossCharacter, BossHealthPercent);
}

void APP_BossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 打上 Boss 标记，让 HUD 识别
	if (HasAuthority() && GetAbilitySystemComponent())
	{
		GetAbilitySystemComponent()->AddLooseGameplayTag(PPTags::Status::Boss);
	}

	if (!HasAuthority())
	{
		return;
	}

	// 绑定血量变化委托（EnemyCharacter.BeginPlay 里已经绑定了 Health 变化，
	// 但我们需要自己算 Percent + 阶段切换，所以再绑一次是安全的）
	if (UPP_AttributeSet* PP_Attr = Cast<UPP_AttributeSet>(GetAttributeSet()))
	{
		CachedMaxHealth = PP_Attr->GetMaxHealth();
		BossHealthPercent = CachedMaxHealth > 0.f ? 1.f : 0.f;
		CurrentPhase = 1;
		GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_Attr->GetHealthAttribute()).AddUObject(this, &ThisClass::OnBossHealthChanged);
	}
}

void APP_BossCharacter::OnBossHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority() || !GetWorld() || bIsBeingLaunched)
	{
		return;
	}

	if (Data.NewValue <= 0.f)
	{
		BossHealthPercent = 0.f;
		return;
	}

	const float Percent = FMath::Clamp(Data.NewValue / FMath::Max(1.f, CachedMaxHealth), 0.f, 1.f);
	BossHealthPercent = Percent;

	ApplyPhaseTransitionIfNeeded(Percent);
}

void APP_BossCharacter::ApplyPhaseTransitionIfNeeded(float NewHealthPercent)
{
	if (CurrentPhase > BossPhaseThresholds.Num() + 1)
	{
		return;
	}

	// PhaseThresholds 有 N 个阈值，对应 N+1 段。从高血量阶段依次往下切。
	// CurrentPhase == 1 表示满血 → 当血量降到 BossPhaseThresholds[0] 时 → Phase 2
	// 所以要检查下一个阈值：BossPhaseThresholds[CurrentPhase - 2]（当 CurrentPhase > 1 时）
	int32 TargetPhase = CurrentPhase;
	for (int32 i = 0; i < BossPhaseThresholds.Num(); ++i)
	{
		if (NewHealthPercent <= BossPhaseThresholds[i])
		{
			TargetPhase = FMath::Max(TargetPhase, i + 2); // 阈值 0 对应 Phase 2
		}
	}

	if (TargetPhase <= CurrentPhase)
	{
		return;
	}

	const int32 OldPhase = CurrentPhase;
	CurrentPhase = TargetPhase;

	// 短暂无敌，避免阶段切换瞬间被秒
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(PPTags::Status::BossPhaseTransition);
	}

	GetWorldTimerManager().ClearTimer(PhaseInvulnerableTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseInvulnerableTimerHandle, this, &ThisClass::OnPhaseTransitionInvulnerableEnd, PhaseTransitionInvulnerableTime, false);

	UE_LOG(LogPP, Warning, TEXT("[Boss] %s 阶段切换 %d → %d (血量 %.2f)"),
		*GetName(), OldPhase, CurrentPhase, NewHealthPercent);

	OnBossPhaseChanged(CurrentPhase, OldPhase);
}

void APP_BossCharacter::OnPhaseTransitionInvulnerableEnd()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(PPTags::Status::BossPhaseTransition);
	}
}

void APP_BossCharacter::HandleDeath()
{
	// 先停掉阶段计时器
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(PhaseInvulnerableTimerHandle);
	}

	// 移除 Boss 标记，让 HUD 自动清掉血条
	if (HasAuthority() && GetAbilitySystemComponent())
	{
		GetAbilitySystemComponent()->RemoveLooseGameplayTag(PPTags::Status::Boss);
		GetAbilitySystemComponent()->RemoveLooseGameplayTag(PPTags::Status::BossPhaseTransition);
		BossHealthPercent = 0.f;
	}

	// 把敌人基类的 GoldDropAmount 乘上 BossGoldMultiplier 传给父类
	// 父类 HandleDeath 里用的是 GoldDropAmount 字段（非 virtual），所以这里先改值再 Super
	GoldDropAmount = FMath::FloorToInt(static_cast<float>(GoldDropAmount) * BossGoldMultiplier);

	Super::HandleDeath();

	UE_LOG(LogPP, Warning, TEXT("[Boss] %s 死亡，击杀金币=%d 倍率=%.1f"),
		*GetName(), GoldDropAmount, BossGoldMultiplier);
}
