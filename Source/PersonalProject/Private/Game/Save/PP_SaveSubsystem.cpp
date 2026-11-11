#include "Game/Save/PP_SaveSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Characters/PP_BossCharacter.h"
#include "Characters/PP_PlayerCharacter.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"
#include "Engine/World.h"
#include "Game/PP_CrashGameMode.h"
#include "Game/PP_EnemyRespawnManager.h"
#include "GameFramework/PlayerController.h"
#include "GameObjects/PP_BossArenaGate.h"
#include "GameObjects/PP_SoulPickupActor.h"
#include "Gather/PP_GatherableActor.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PP_PlayerState.h"
#include "Quest/PP_QuestComponent.h"
#include "Save/PP_SaveGame.h"

namespace
{
	/** 读取九项属性的 base 值（不含装备 GE 的 Modifier） */
	void CaptureAttributes(UAbilitySystemComponent* ASC, FPP_SaveAttributeData& Out)
	{
		if (!ASC)
		{
			return;
		}

		Out.Health = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetHealthAttribute());
		Out.MaxHealth = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxHealthAttribute());
		Out.Mana = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetManaAttribute());
		Out.MaxMana = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxManaAttribute());
		Out.Stamina = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetStaminaAttribute());
		Out.MaxStamina = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxStaminaAttribute());
		Out.Attack = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetAttackAttribute());
		Out.Defense = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetDefenseAttribute());
		Out.StaminaRegenRate = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetStaminaRegenRateAttribute());
	}

	/** 写回九项属性的 base 值：先写上限，再把当前值夹到上限内 */
	void ApplyAttributes(UAbilitySystemComponent* ASC, const FPP_SaveAttributeData& In)
	{
		if (!ASC)
		{
			return;
		}

		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxHealthAttribute(), In.MaxHealth);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxManaAttribute(), In.MaxMana);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxStaminaAttribute(), In.MaxStamina);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetAttackAttribute(), In.Attack);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetDefenseAttribute(), In.Defense);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetStaminaRegenRateAttribute(), In.StaminaRegenRate);

		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetHealthAttribute(), FMath::Clamp(In.Health, 0.f, In.MaxHealth));
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetManaAttribute(), FMath::Clamp(In.Mana, 0.f, In.MaxMana));
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetStaminaAttribute(), FMath::Clamp(In.Stamina, 0.f, In.MaxStamina));
	}

	/** 采集世界状态快照：只记录与默认状态不同的部分 */
	void CaptureWorldState(UWorld* World, APP_PlayerState* PS, FPP_SaveWorldData& Out)
	{
		if (!World || !PS)
		{
			return;
		}

		// 采集点：满次即默认状态，不落盘
		for (TActorIterator<APP_GatherableActor> It(World); It; ++It)
		{
			APP_GatherableActor* Gatherable = *It;
			if (!IsValid(Gatherable))
			{
				continue;
			}

			const int32 Remaining = Gatherable->GetRemainingGatherTimes();
			if (Remaining >= Gatherable->MaxGatherTimes)
			{
				continue;
			}

			FPP_SaveGatherableState State;
			State.ActorName = Gatherable->GetFName();
			State.GatherableID = Gatherable->GatherableID;
			State.RemainingGatherTimes = FMath::Max(0, Remaining);
			Out.Gatherables.Add(State);
		}

		// Boss：死亡后 Actor 会被销毁，只能从 GameMode 的名字集合里取
		if (APP_CrashGameMode* GameMode = World->GetAuthGameMode<APP_CrashGameMode>())
		{
			for (const FName& BossName : GameMode->GetDefeatedBossNames())
			{
				FPP_SaveBossState State;
				State.BossName = BossName;
				State.bDefeated = true;
				Out.Bosses.Add(State);
			}
		}

		// 魂：只存当前玩家的
		for (TActorIterator<APP_SoulPickupActor> It(World); It; ++It)
		{
			APP_SoulPickupActor* Soul = *It;
			if (!IsValid(Soul) || Soul->GetOwnerState() != PS || Soul->GetAmount() <= 0)
			{
				continue;
			}

			FPP_SaveSoulState State;
			State.Amount = Soul->GetAmount();
			State.Location = Soul->GetActorLocation();
			Out.Souls.Add(State);
		}
	}

	/** 按关卡 Actor 名恢复采集点剩余次数；找不到就跳过并告警 */
	void ApplyGatherableStates(UWorld* World, const TArray<FPP_SaveGatherableState>& States)
	{
		if (!World)
		{
			return;
		}

		for (const FPP_SaveGatherableState& State : States)
		{
			if (State.ActorName.IsNone())
			{
				continue;
			}

			APP_GatherableActor* Match = nullptr;
			for (TActorIterator<APP_GatherableActor> It(World); It; ++It)
			{
				if (IsValid(*It) && It->GetFName() == State.ActorName)
				{
					Match = *It;
					break;
				}
			}

			if (!Match)
			{
				UE_LOG(LogPP, Warning, TEXT("[PP_Save] 采集点 %s 在当前关卡中不存在，已跳过"),
					*State.ActorName.ToString());
				continue;
			}

			Match->ApplySavedState(State.RemainingGatherTimes);
		}
	}

	/** 恢复 Boss 击败状态：移除存活 Boss，并打开关联的场地门 */
	void ApplyBossStates(UWorld* World, const TArray<FPP_SaveBossState>& States)
	{
		if (!World)
		{
			return;
		}

		TSet<FName> DefeatedNames;

		for (const FPP_SaveBossState& State : States)
		{
			if (!State.bDefeated || State.BossName.IsNone())
			{
				continue;
			}

			DefeatedNames.Add(State.BossName);

			// 先收集再处理，避免在 TActorIterator 里直接销毁 Actor
			TArray<APP_BossCharacter*> Matches;
			for (TActorIterator<APP_BossCharacter> It(World); It; ++It)
			{
				if (IsValid(*It) && It->GetBossName() == State.BossName)
				{
					Matches.Add(*It);
				}
			}

			// 注销重生记录：否则篝火休息会把已击败的 Boss 重建出来
			if (APP_CrashGameMode* GameMode = World->GetAuthGameMode<APP_CrashGameMode>())
			{
				if (UPP_EnemyRespawnManager* RespawnManager = GameMode->GetEnemyRespawnManager())
				{
					for (APP_BossCharacter* Boss : Matches)
					{
						RespawnManager->UnregisterEnemy(Boss);
					}
				}
			}

			for (APP_BossCharacter* Boss : Matches)
			{
				Boss->ApplySavedDefeatedState();
			}

			bool bGateFound = false;
			for (TActorIterator<APP_BossArenaGate> It(World); It; ++It)
			{
				if (IsValid(*It) && It->GetLinkedBossName() == State.BossName)
				{
					It->SetOpen(true);
					bGateFound = true;
				}
			}

			if (Matches.Num() == 0 && !bGateFound)
			{
				UE_LOG(LogPP, Warning,
					TEXT("[PP_Save] Boss %s 在当前关卡中既没有存活 Actor 也没有匹配的场地门，已跳过"),
					*State.BossName.ToString());
			}
		}

		// 整体替换本局记录，保证之后再存档写的还是读档时的状态
		if (APP_CrashGameMode* GameMode = World->GetAuthGameMode<APP_CrashGameMode>())
		{
			GameMode->SetDefeatedBossNames(DefeatedNames);
		}
	}

	/** 恢复掉落的魂：先清掉本会话该玩家的旧魂，再按存档重新生成 */
	void ApplySoulStates(UWorld* World, APP_PlayerState* PS, APlayerController* PC,
		const TArray<FPP_SaveSoulState>& States)
	{
		if (!World || !PS)
		{
			return;
		}

		TArray<APP_SoulPickupActor*> Existing;
		for (TActorIterator<APP_SoulPickupActor> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetOwnerState() == PS)
			{
				Existing.Add(*It);
			}
		}
		for (APP_SoulPickupActor* Soul : Existing)
		{
			Soul->Destroy();
		}

		if (States.Num() == 0)
		{
			return;
		}

		APP_PlayerCharacter* PlayerCharacter = PC ? Cast<APP_PlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!PlayerCharacter)
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Save] 读档时没有 Pawn，%d 个掉落魂未恢复"), States.Num());
			return;
		}

		for (const FPP_SaveSoulState& State : States)
		{
			if (State.Amount <= 0 || !FMath::IsFinite(State.Location.X)
				|| !FMath::IsFinite(State.Location.Y) || !FMath::IsFinite(State.Location.Z))
			{
				UE_LOG(LogPP, Warning, TEXT("[PP_Save] 魂数据非法（金额=%d），已跳过"), State.Amount);
				continue;
			}

			PlayerCharacter->RestoreSoul(State.Amount, State.Location);
		}
	}
}

UPP_SaveSubsystem* UPP_SaveSubsystem::Get(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UPP_SaveSubsystem>() : nullptr;
}

bool UPP_SaveSubsystem::SavePlayer(APlayerController* PC, FString& OutError)
{
	OutError.Reset();

	if (!PC || !PC->HasAuthority())
	{
		OutError = TEXT("只能在服务端保存");
		return false;
	}

	// 存档只认房主/单人：客户端发来的保存请求在这里拒绝，避免用客户端数据覆盖单槽存档
	if (!PC->IsLocalController())
	{
		OutError = TEXT("只有房主可以保存");
		return false;
	}

	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		OutError = TEXT("未找到 PlayerState");
		return false;
	}

	// 同一帧内多个时机（篝火 + 返回大厅/退出）触发时只写一次
	if (LastSaveFrame == GFrameCounter)
	{
		UE_LOG(LogPP, Verbose, TEXT("[PP_Save] 同一帧内重复保存已忽略"));
		return true;
	}

	UPP_SaveGame* SaveData = Cast<UPP_SaveGame>(UGameplayStatics::CreateSaveGameObject(UPP_SaveGame::StaticClass()));
	if (!SaveData)
	{
		OutError = TEXT("创建存档对象失败");
		return false;
	}

	SaveData->MapName = FName(*UGameplayStatics::GetCurrentLevelName(PC->GetWorld(), true));
	SaveData->PlayerName = PS->GetPlayerName();
	SaveData->Character.CharacterId = PS->GetSelectedCharacterId();
	CaptureAttributes(PS->GetAbilitySystemComponent(), SaveData->Character.BaseAttributes);

	if (const UPP_InventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		SaveData->Inventory.Items = Inventory->GetItems();
		SaveData->Inventory.Gold = Inventory->GetGold();

		// 装备只落盘 ItemID + Slot；EffectHandle 是运行时句柄，读档时重新解析重建
		SaveData->Inventory.Equipped = Inventory->GetEquippedItems();
		for (FPP_EquippedItem& Equipped : SaveData->Inventory.Equipped)
		{
			Equipped.EffectHandle = FActiveGameplayEffectHandle();
		}
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 保存时未找到 InventoryComponent，背包数据未写入"));
	}

	if (const UPP_QuestComponent* QuestComponent = PS->GetQuestComponent())
	{
		SaveData->Quests.ActiveQuests = QuestComponent->GetActiveQuests();
		SaveData->Quests.CompletedQuestIDs = QuestComponent->GetCompletedQuests();
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 保存时未找到 QuestComponent，任务进度未写入"));
	}

	// 篝火检查点：只记录玩家状态，不依赖篝火 Actor 是否还在
	SaveData->Checkpoint.bHasCheckpoint = PS->HasCheckpoint();
	SaveData->Checkpoint.Transform = PS->GetCheckpointTransform();
	SaveData->Checkpoint.BonfireActorName = PS->GetCheckpointBonfireName();

	// 世界状态：采集点剩余次数 / 已击败 Boss / 掉落的魂
	CaptureWorldState(PC->GetWorld(), PS, SaveData->World);

	UE_LOG(LogPP, Log, TEXT("[PP_Save] 世界状态快照：采集点=%d Boss=%d 魂=%d 检查点=%d"),
		SaveData->World.Gatherables.Num(), SaveData->World.Bosses.Num(), SaveData->World.Souls.Num(),
		SaveData->Checkpoint.bHasCheckpoint ? 1 : 0);

	const bool bSaved = UPP_SaveGame::Save(SaveData, OutError);
	if (bSaved)
	{
		LastSaveFrame = GFrameCounter;
	}
	return bSaved;
}

bool UPP_SaveSubsystem::SaveLocalPlayer(FString& OutError)
{
	OutError.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		OutError = TEXT("当前没有有效世界");
		return false;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController())
		{
			return SavePlayer(PC, OutError);
		}
	}

	OutError = TEXT("未找到本机玩家控制器");
	return false;
}

bool UPP_SaveSubsystem::LoadFromDisk(FString& OutError)
{
	UPP_SaveGame* Loaded = UPP_SaveGame::Load(OutError);
	if (!Loaded)
	{
		if (OutError.IsEmpty())
		{
			OutError = TEXT("没有存档");
		}
		return false;
	}

	PendingSave = Loaded;
	UE_LOG(LogPP, Log, TEXT("[PP_Save] 已读取存档到内存：地图=%s 角色=%s"),
		*Loaded->MapName.ToString(), *Loaded->Character.CharacterId.ToString());
	return true;
}

bool UPP_SaveSubsystem::ApplyPendingToPlayer(APlayerController* PC, FString& OutError)
{
	OutError.Reset();

	if (!PC || !PC->HasAuthority())
	{
		OutError = TEXT("只能在服务端读档");
		return false;
	}

	if (!PendingSave)
	{
		OutError = TEXT("没有待应用的存档");
		return false;
	}

	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		OutError = TEXT("未找到 PlayerState");
		return false;
	}

	// 上一次应用残留的落点（理论上不会出现，防御性清掉，避免下次 RestartPlayer 误传送）
	bHasPendingSpawnTransform = false;

	const FPP_SaveCharacterData& Character = PendingSave->Character;

	PS->SetSelectedCharacterId(Character.CharacterId);

	if (APP_PlayerCharacter* PlayerCharacter = Cast<APP_PlayerCharacter>(PC->GetPawn()))
	{
		PlayerCharacter->ApplyCharacterAppearance(Character.CharacterId);
	}

	// 先恢复背包/装备：装备 GE 的加成先挂上去，最后再写属性 base，避免加成被 base 覆盖或双算
	if (UPP_InventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		Inventory->RestoreFromSave(PendingSave->Inventory);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 读档时未找到 InventoryComponent，背包数据未应用"));
	}

	// 任务进度：结构以 DT_Task 校正，恢复后广播一次任务更新事件
	if (UPP_QuestComponent* QuestComponent = PS->GetQuestComponent())
	{
		QuestComponent->RestoreFromSave(PendingSave->Quests);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 读档时未找到 QuestComponent，任务进度未应用"));
	}

	ApplyAttributes(PS->GetAbilitySystemComponent(), Character.BaseAttributes);

	// ===== 篝火检查点 + 世界状态：只在存档地图与当前地图一致时应用 =====
	// 地图级数据（关卡 Actor 名、世界坐标）换图后没有意义，套过来会把上一张图的状态带进来
	UWorld* World = PC->GetWorld();
	const FString SavedMapName = PendingSave->MapName.ToString();
	const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(World, true);
	const bool bSameMap = !SavedMapName.IsEmpty() && SavedMapName.Equals(CurrentMapName, ESearchCase::IgnoreCase);

	if (bSameMap)
	{
		const FPP_SaveCheckpointData& Checkpoint = PendingSave->Checkpoint;
		if (Checkpoint.bHasCheckpoint)
		{
			PS->SetCheckpoint(Checkpoint.Transform, Checkpoint.BonfireActorName);
		}
		else
		{
			PS->ClearCheckpoint();
		}

		ApplyGatherableStates(World, PendingSave->World.Gatherables);
		ApplyBossStates(World, PendingSave->World.Bosses);
		ApplySoulStates(World, PS, PC, PendingSave->World.Souls);

		// 落点：已有存活 Pawn 时直接传送（控制台读档）；否则留给 RestartPlayer 一次性消费（主菜单读档）
		if (Checkpoint.bHasCheckpoint)
		{
			if (APP_PlayerCharacter* PlayerCharacter = Cast<APP_PlayerCharacter>(PC->GetPawn()))
			{
				if (PlayerCharacter->IsAlive())
				{
					PlayerCharacter->TeleportTo(Checkpoint.Transform.GetLocation(), Checkpoint.Transform.Rotator(), false, true);
					UE_LOG(LogPP, Log, TEXT("[PP_Save] 读档落点：传送到检查点 (%.0f, %.0f, %.0f)"),
						Checkpoint.Transform.GetLocation().X, Checkpoint.Transform.GetLocation().Y,
						Checkpoint.Transform.GetLocation().Z);
				}
			}
			else
			{
				PendingSpawnTransform = Checkpoint.Transform;
				bHasPendingSpawnTransform = true;
			}
		}
	}
	else
	{
		UE_LOG(LogPP, Warning,
			TEXT("[PP_Save] 检查点与世界状态未应用：存档地图=%s 当前地图=%s"),
			SavedMapName.IsEmpty() ? TEXT("<空>") : *SavedMapName, *CurrentMapName);
	}

	UE_LOG(LogPP, Log, TEXT("[PP_Save] 已应用存档到 %s：角色=%s 生命=%.1f/%.1f 法力=%.1f/%.1f 耐力=%.1f/%.1f"),
		*PS->GetPlayerName(), *Character.CharacterId.ToString(),
		Character.BaseAttributes.Health, Character.BaseAttributes.MaxHealth,
		Character.BaseAttributes.Mana, Character.BaseAttributes.MaxMana,
		Character.BaseAttributes.Stamina, Character.BaseAttributes.MaxStamina);

	// one-shot：应用成功即清除，重生/重连/再次 Possess 不得重复套用
	PendingSave = nullptr;
	return true;
}

bool UPP_SaveSubsystem::ConsumePendingSpawnTransform(FTransform& OutTransform)
{
	if (!bHasPendingSpawnTransform)
	{
		return false;
	}

	OutTransform = PendingSpawnTransform;
	bHasPendingSpawnTransform = false;
	PendingSpawnTransform = FTransform::Identity;
	return true;
}
