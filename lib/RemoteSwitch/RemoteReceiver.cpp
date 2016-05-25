/*
 * RemoteSwitch library v2.0.0 made by Randy Simons http://randysimons.nl
 * See RemoteSwitchSender.h for details.
 *
 * License: "Free BSD license". See license.txt
 */

#include "RemoteReceiver.h"


/************
* RemoteReceiver
************/

unsigned short RemoteReceiver::_interrupt;
volatile int RemoteReceiver::_state;
unsigned short RemoteReceiver::_minRepeats;
RemoteReceiverCallBack RemoteReceiver::_callback;
boolean RemoteReceiver::_inCallback = false;

void RemoteReceiver::init(unsigned short interrupt, unsigned short minRepeats, RemoteReceiverCallBack callback) {
	_interrupt = interrupt;
	_minRepeats = minRepeats;
	_callback = callback;

	//enable();
}

void RemoteReceiver::enable() {
	_state = -1;
	attachInterrupt(_interrupt, interruptHandler, CHANGE);
}

void RemoteReceiver::disable() {
	detachInterrupt(_interrupt);
}


void RemoteReceiver::interruptHandler() {
	static unsigned int period;			//Calculated duration of 1 period
	static unsigned short receivedBit;	//Contains "bit" currently receiving
	static unsigned long receivedCode;	//Contains received code
	static unsigned long previousCode;	//Contains previous received code
	static unsigned short repeats = 0;	//The number of times the an identical code is received in a row.
	static unsigned long lastChange=0;	//Timestamp of previous edge
	static unsigned int min1Period, max1Period, min3Period, max3Period;

	unsigned long currentTime=micros();
	unsigned int duration=currentTime-lastChange; //Duration = Time between edges
	lastChange=currentTime;


    if (_state==-1) { //Waiting for sync-signal
		if (duration>3720) { //==31*120 minimal time between two edges before decoding starts.
			//Sync signal received.. Preparing for decoding
			period=duration/31;
			receivedCode=previousCode=repeats=0;

			//Allow for large error-margin. ElCheapo-hardware :(
			min1Period=period*4/10; //Avoid floating point math; saves memory.
			max1Period=period*16/10;
			min3Period=period*23/10;
			max3Period=period*37/10;
		}
		else {
			return;
		}
	} else if (_state<48) { //Decoding message
		//bit part durations can ONLY be 1 or 3 periods.
		if (duration>=min1Period && duration<=max1Period) {
			bitClear(receivedBit,_state%4); //Note: this sets the bits in reversed order! Correct order would be: 3-(_state%4), but that's overhead we don't want.
		}
		else if (duration>=min3Period && duration<=max3Period) {
			bitSet(receivedBit,_state%4); //Note: this sets the bits in reversed order!
		}
		else { //Otherwise the entire sequence is invalid
			_state=-1;
			return;
		}

		if ((_state%4)==3) { //Last bit part?
			//Shift
			receivedCode*=3;
			//Decode bit.
			if (receivedBit==B1010) {  //short long short long == B0101, but bits are set in reversed order, so compare to B1010
				//bit "0" received
				receivedCode+=0; //I hope the optimizer handles this ;)
			}
			else if (receivedBit==B0101) { //long short long short == B101, but bits are set in reversed order, so compare to B0101
				//bit "1" received
				receivedCode+=1;
			}
			else if (receivedBit==B0110) { //short long long short. Reversed too, but makes no difference.
				//bit "f" received
				receivedCode+=2;
			}
			else {
				//Bit was rubbish. Abort.
				_state=-1;
				return;
			}
            		}
	} else if (_state==48) { //Waiting for sync bit part 1
		//Must be 1 period.
		if (duration<min1Period || duration>max1Period) {
			_state=-1;
			return;
		}
	} else { //Waiting for sync bit part 2
		//Must be 31 periods.
		if (duration<period*25 || duration>period*36) {
		  _state=-1;
		  return;
		}

		//receivedCode is a valid code!

		if (receivedCode!=previousCode) {
			repeats=0;
			previousCode=receivedCode;
		}

		repeats++;

		if (repeats>=_minRepeats) {
			if (!_inCallback) {
				_inCallback = true;
				(_callback)(receivedCode, period);
				_inCallback = false;
			}
			//Reset after callback.
			_state=-1;
			return;
		}

		//Reset for next round
		receivedCode = 0;
		_state=0; //no need to wait for another sync-bit!
		return;
	}

	_state++;
	return;
}

boolean RemoteReceiver::isReceiving(int waitMillis) {
	unsigned long startTime=millis();

	int waited; //signed int!
	do {
		if (_state!=-1) {
			return true;
		}
		waited = (millis()-startTime);
	} while(waited>=0 && waited <= waitMillis); //Yes, clock wraps every 50 days. And then you'd have to wait for a looooong time.

	return false;
}
