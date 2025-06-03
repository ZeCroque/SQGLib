Scriptname SQGDebugZZ Extends Quest Hidden

ReferenceAlias Property SQGTestAliasTarget Auto

Function Fragment_0()
    SetObjectiveDisplayed(10)
EndFunction

Function Fragment_1()
    SetObjectiveDisplayed(12)
EndFunction

Function Fragment_2()
    SetObjectiveDisplayed(15)
    SetObjectiveFailed(12)
EndFunction

Function Fragment_3()
    SetObjectiveCompleted(15)
EndFunction

Function Fragment_4()
    SetObjectiveFailed(12)
 EndFunction

Function Fragment_5()
    SetObjectiveCompleted(10)
    SetObjectiveFailed(12)
 EndFunction

Function Fragment_6()
    SetObjectiveFailed(10)
    SetObjectiveCompleted(12)
EndFunction