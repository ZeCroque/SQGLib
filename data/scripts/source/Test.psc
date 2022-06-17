Scriptname HelloWorldScript extends ObjectReference  
 
import SQGLibRegistrationScript

Event OnActivate(ObjectReference akActionRef)
	Debug.MessageBox(HelloWorld())
endEvent