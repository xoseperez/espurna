#include <RemoteReceiver.h>

/*
* Demo for RF remote switch receiver.
* For details, see RemoteReceiver.h!
*
* This sketch shows the received signals on the serial port.
* Connect the receiver to digital pin 2.
*/


void setup() {
  Serial.begin(115200);
  
  //Initialize receiver on interrupt 0 (= digital pin 2), calls the callback "showCode"
  //after 3 identical codes have been received in a row. (thus, keep the button pressed
  //for a moment)
  //
  //See the interrupt-parameter of attachInterrupt for possible values (and pins)
  //to connect the receiver.
  RemoteReceiver::init(0, 3, showCode);
}

void loop() {
}

//Callback function is called only when a valid code is received.
void showCode(unsigned long receivedCode, unsigned int period) {
  //Note: interrupts are disabled. You can re-enable them if needed.
  
  //Print the received code.
  Serial.print("Code: ");
  Serial.print(receivedCode);
  Serial.print(", period duration: ");
  Serial.print(period);
  Serial.println("us.");
}


