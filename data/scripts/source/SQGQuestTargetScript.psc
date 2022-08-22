Scriptname SQGQuestTargetScript extends ReferenceAlias  

Event OnDeath(Actor AkKiller)
	GetOwningQuest().SetStage(40)
EndEvent