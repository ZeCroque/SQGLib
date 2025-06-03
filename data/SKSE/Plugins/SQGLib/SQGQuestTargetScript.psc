Scriptname SQGQuestTargetScriptZZ extends ReferenceAlias  

Event OnDeath(Actor AkKiller)
	GetOwningQuest().SetObjectiveFailed(12)
	GetOwningQuest().SetObjectiveFailed(15)
	GetOwningQuest().SetStage(40)
EndEvent