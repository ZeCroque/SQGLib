Scriptname SQGQuestTargetScript extends ReferenceAlias  

Quest Property SQGSampleQuest Auto

Event OnDeath(Actor AkKiller)
	if SQGSampleQuest.GetStage() == 10
		SQGSampleQuest.SetStage(20)
    endif
EndEvent