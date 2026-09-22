#include "GameplayTags/PPTags.h"

namespace PPTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None,"PPTags.None","None Tag")
	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile,"PPTags.SetByCaller.Projectile","Tag for Set by Caller Magnitude for Projectiles")

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
		}

		namespace UI
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ShowScoreboard,"PPTags.Events.UI.ShowScoreboard","Tag for the Show Scoreboard UI Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TogglePause,"PPTags.Events.UI.TogglePause","Tag for the Toggle Pause UI Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageNumber,"PPTags.Events.UI.DamageNumber","Tag for the Damage Number UI Event")
		}
	}

	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead,"PPTags.Status.Dead","Tag granted by GE_Death while dead")
	}
}
