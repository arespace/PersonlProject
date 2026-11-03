#include "GameplayTags/PPTags.h"

namespace PPTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None,"PPTags.None","None Tag")
	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile,"PPTags.SetByCaller.Projectile","Tag for Set by Caller Magnitude for Projectiles")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StaminaCost,"PPTags.SetByCaller.StaminaCost","Tag for Set by Caller Magnitude for Stamina Cost")

		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"PPTags.SetByCaller.Player.Secondary","Tag for Set by Caller Magnitude for the Secondary Ability")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary,"PPTags.SetByCaller.Player.Tertiary","Tag for Set by Caller Magnitude for the Tertiary Ability")
		}
	}
	namespace PPAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven,"PPTags.PPAbilities.ActivateOnGiven","Tag for Abilities that should activate immediately once given.")
		
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,"PPTags.PPAbilities.Primary","Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"PPTags.PPAbilities.Secondary","Tag for the Secondary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary,"PPTags.PPAbilities.Tertiary","Tag for the Tertiary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fourth,"PPTags.PPAbilities.Fourth","Tag for the Fourth Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fifth,"PPTags.PPAbilities.Fifth","Tag for the Fifth Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockHitReact,"PPTags.PPAbilities.BlockHitReact","Tag granted by GE_BlockHitReact while blocking/reacting")

		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack,"PPTags.PPAbilities.Enemy.Attack","Enemy Attack Tag")
		}
	}
	
	namespace Events
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(KillScored,"PPTags.Events.KillScored","Tag for the KillScored Event")

		namespace Inventory
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentChanged,"PPTags.Events.Inventory.EquipmentChanged","武器装备槽变更事件：装备/卸下时广播")
		}

		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,"PPTags.Events.Player.HitReact","Tag for the Player HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death,"PPTags.Events.Player.Death","Tag for the Player Death Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,"PPTags.Events.Player.Primary","Tag for the Player Primary Attack AnimNotify Event")

			
		}
		
		
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,"PPTags.Events.Enemy.HitReact","Tag for the HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack,"PPTags.Events.Enemy.EndAttack","Tag for the Ending an Attack ")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit,"PPTags.Events.Enemy.MeleeTraceHit","Tag for the Enemy Melee Trace Hit ")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reset,"PPTags.Events.Enemy.Reset","篝火休息后重置区域敌人事件")
		}

		namespace UI
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ShowScoreboard,"PPTags.Events.UI.ShowScoreboard","Tag for the Show Scoreboard UI Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TogglePause,"PPTags.Events.UI.TogglePause","Tag for the Toggle Pause UI Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageNumber,"PPTags.Events.UI.DamageNumber","Tag for the Damage Number UI Event")
		}

		namespace Boss
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead,"PPTags.Events.Boss.Dead","Boss 死亡事件")
		}

		namespace Settlement
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Show,"PPTags.Events.Settlement.Show","Boss 死亡后显示结算界面")
		}
	}

	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead,"PPTags.Status.Dead","Tag granted by GE_Death while dead")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sprinting,"PPTags.Status.Sprinting","Loose tag while the player is actually sprinting")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StaminaRegenBlocked,"PPTags.Status.StaminaRegenBlocked","Tag granted by GE_StaminaRegenDelay to suppress stamina regeneration")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Boss,"PPTags.Status.Boss","Boss marker: drives HUD bar and reward multiplier")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BossPhaseTransition,"PPTags.Status.BossPhaseTransition","Brief invulnerability during boss phase transition")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invulnerable,"PPTags.Status.Invulnerable","Roll i-frames: blocks all damage while active")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BossArenaArmed,"PPTags.Status.BossArenaArmed","BossArenaTrigger Overlap 后激活，Boss 据此授予 SearchForTarget GA")
	}
}
