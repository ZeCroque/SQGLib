Scriptname SQGDebug Extends Quest Hidden

ReferenceAlias Property SQGTestAliasTarget Auto

Function Fragment_0()
Debug.MessageBox(SQGTestAliasTarget.GetActorReference().GetFormID())
SetObjectiveDisplayed(10)
EndFunction

Function Fragment_1()
SetObjectiveCompleted(10)
EndFunction
