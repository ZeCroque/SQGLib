Scriptname SQGTestActivator extends ObjectReference  
 
import SQGLib

Message Property SQGTestMessage Auto

Event OnActivate(ObjectReference akActionRef)
	Int result = SQGTestMessage.Show()
	If (result == 0) 
		Debug.MessageBox(GenerateQuest())
	ElseIf (result == 1) 
		Debug.MessageBox(SwapSelectedQuest())
	ElseIf (result == 2) 
		StartSelectedQuest()
	Else
		DraftDebugFunction()
	EndIf 
endEvent


