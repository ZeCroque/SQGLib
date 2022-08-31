Scriptname SQGTargetActivatorScript extends ObjectReference 

import SQGLib

Event OnActivate(ObjectReference akActionRef)
	Debug.MessageBox(GetSelectedQuest().GetName())
	GetSelectedQuest().SetStage(30) 
endEvent


