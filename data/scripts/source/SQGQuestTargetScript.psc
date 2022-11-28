Scriptname SQGQuestTargetScript extends ReferenceAlias  

Event OnDeath(Actor AkKiller)
	GetOwningQuest().SetStage(40)
	GetOwningQuest().SetObjectiveFailed(12)
	GetOwningQuest().SetObjectiveFailed(15)
EndEvent