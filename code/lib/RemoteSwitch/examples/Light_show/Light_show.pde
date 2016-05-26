#include <RemoteSwitch.h>

/*
* Demo for RF remote switch transmitter.
* For details, see RemoteSwitch.h!
*
* This sketch switches some devices on and off in a loop.
*/

//Intantiate a new ActionSwitch remote, use pin 11
ActionSwitch actionSwitch(11);

//Intantiate a new KaKuSwitch remote, also use pin 11 (same transmitter!)
KaKuSwitch kaKuSwitch(11);

//Intantiate a new Blokker remote, also use pin 11 (same transmitter!)
BlokkerSwitch blokkerSwitch(11);


void setup() {
}

void loop() {  
  //Switch off KaKu-device 10 on address M
  kaKuSwitch.sendSignal('M',10,false);
  
  //Switch on Action-device B on system code 1.
  actionSwitch.sendSignal(1,'B',true);
  
  //Switch on Blokker-device 7.
  blokkerSwitch.sendSignal(7,true);
  
  
  
  //wait 2 seconds
  delay(2000);
  
  
  
  //Switch on KaKu-device 2 of group 3 on address M (which is the same as device 10 on address M!)
  kaKuSwitch.sendSignal('M',3,2,true);
  
  //Switch off Action-device B on system code 1.
  actionSwitch.sendSignal(1,'B',false);
  
  //Switch off Blokker-device 7.
  blokkerSwitch.sendSignal(7,false);
  
  
  
  //wait 4 seconds
  delay(4000);
}
