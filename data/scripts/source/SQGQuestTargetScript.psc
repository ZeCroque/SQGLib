Scriptname SQGQuestTargetScript extends ReferenceAlias  

Event OnDeath(Actor AkKiller)
	if GetOwningQuest().GetStage() == 10
		GetOwningQuest().SetStage(20)
    endif
EndEvent