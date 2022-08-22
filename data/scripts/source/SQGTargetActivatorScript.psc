Scriptname SQGTargetActivatorScript extends ObjectReference 

Quest Property SQGSampleQuest  Auto  

Event OnActivate(ObjectReference akActionRef)
	Debug.MessageBox("Activated")
	SQGSampleQuest.SetStage(30) 
endEvent


