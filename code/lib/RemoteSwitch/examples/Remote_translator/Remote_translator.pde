#include <RemoteReceiver.h>
#include <RemoteSwitch.h>

/*
* Demo for RF remote switch receiver.
* For details, see RemoteReceiver.h!
*
* This sketch "translates" a Action-remote to a Blokker-remote.
* When the A-On-button of the Action-remote is pressed, the Blokker-devices
* 5, 6 and 7 are switched on. The A-Off-button switches the devices off again.
*
* Connect the transmitter to digital pin 11, and the receiver to digital pin 2.
*/

ActionSwitch actionSwitch(11);
BlokkerSwitch blokkerSwitch(11);

//Prepare the code for switch A (system code 1) on and off, for easy comparision later.
unsigned long actionAOn = actionSwitch.getTelegram(1,'A',true);
unsigned long actionAOff = actionSwitch.getTelegram(1,'A',false);

void setup() {
  //See example Show_received_code for info on this
  RemoteReceiver::init(0, 3, translateCode);
}

void loop() {
}

//Callback function is called only when a valid code is received.
void translateCode(unsigned long receivedCode, unsigned int period) {
  //Enabled interrupts, so RemoteReceiver::isReceiving() can be used.
  interrupts();
  
  //Compare the signals
  if (RemoteSwitch::isSameCode(actionAOn, receivedCode)) {
	//A-On-button pressed!
	
	//Wait for a free ether
    while(RemoteReceiver::isReceiving());
	
	//Switch devices on
    blokkerSwitch.sendSignal(5,true);
    blokkerSwitch.sendSignal(6,true);
    blokkerSwitch.sendSignal(7,true);
	
  } else if (RemoteSwitch::isSameCode(actionAOff, receivedCode)) {  
	//A-Off-button pressed!
	
	//Wait for a free ether
    while(RemoteReceiver::isReceiving());
	
	//Switch devices off
    blokkerSwitch.sendSignal(5,false);
    blokkerSwitch.sendSignal(6,false);
    blokkerSwitch.sendSignal(7,false);
  }  
}
