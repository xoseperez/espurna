/* 
 * RemoteSwitch library v2.0.0 made by Randy Simons http://randysimons.nl
 * See RemoteSwitchSender.h for details.
 *
 * License: "Free BSD license". See license.txt
 */

#include "RemoteSwitch.h"


/************
* RemoteSwitch
************/

RemoteSwitch::RemoteSwitch(unsigned short pin, unsigned int periodusec, unsigned short repeats) {
	_pin=pin;
	_periodusec=periodusec;
	_repeats=repeats;
	
	pinMode(_pin, OUTPUT);
}

unsigned long RemoteSwitch::encodeTelegram(unsigned short trits[]) {
	unsigned long data = 0;
		
	//Encode data
	for (unsigned short i=0;i<12;i++) {
		data*=3;
		data+=trits[i];
	}
	
	//Encode period duration
	data |= (unsigned long)_periodusec << 23;
	
	//Encode repeats
	data |= (unsigned long)_repeats << 20;
	
	return data;
}
		
void RemoteSwitch::sendTelegram(unsigned short trits[]) {
	sendTelegram(encodeTelegram(trits),_pin);	
}

/**
* Format data:
* pppppppp|prrrdddd|dddddddd|dddddddd (32 bit)
* p = perioud (9 bit unsigned int
* r = repeats as 2log. Thus, if r = 3, then signal is sent 2^3=8 times
* d = data
*/
void RemoteSwitch::sendTelegram(unsigned long data, unsigned short pin) {
	unsigned int periodusec = (unsigned long)data >> 23;
	unsigned short repeats = 1 << (((unsigned long)data >> 20) & B111);
	data = data & 0xfffff; //truncate to 20 bit
		
	//Convert the base3-code to base4, to avoid lengthy calculations when transmitting.. Messes op timings.
	unsigned long dataBase4 = 0;
	
	for (unsigned short i=0; i<12; i++) {
		dataBase4<<=2;
		dataBase4|=(data%3);
		data/=3;
	}
	
	for (unsigned short int j=0;j<repeats;j++) {		
		//Sent one telegram		
		
		//Use data-var as working var
		data=dataBase4;
		for (unsigned short i=0; i<12; i++) {
			switch (data & B11) {
				case 0:
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					break;
				case 1:
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					break;
				case 2: //AKA: X or float
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					break;
			}
			//Next trit
			data>>=2;
		}
		
		//Send termination/synchronisation-signal. Total length: 32 periods
		digitalWrite(pin, HIGH);
		delayMicroseconds(periodusec);
		digitalWrite(pin, LOW);
		delayMicroseconds(periodusec*31);
	}
}

boolean RemoteSwitch::isSameCode(unsigned long encodedTelegram, unsigned long receivedData) {
	return (receivedData==(encodedTelegram & 0xFFFFF)); //Compare the 20 LSB's
}


/************
* ActionSwitch
************/

ActionSwitch::ActionSwitch(unsigned short pin, unsigned int periodusec) : RemoteSwitch(pin,periodusec,3) {
	//Call contructor
}


void ActionSwitch::sendSignal(unsigned short systemCode, char device, boolean on) {		
	sendTelegram(getTelegram(systemCode,device,on), _pin);
}

unsigned long ActionSwitch::getTelegram(unsigned short systemCode, char device, boolean on) {
	unsigned short trits[12];
	
	device-=65;
	
	for (unsigned short i=0; i<5; i++) {
		//bits 0-4 contain address (2^5=32 addresses)
		trits[i]=(systemCode & 1)?1:2;          
		systemCode>>=1;
		
		//bits 5-9 contain device. Only one trit has value 0, others have 2 (float)!
		trits[i+5]=(i==device?0:2);
    }
	
	//switch on or off
	trits[10]=(!on?0:2);
	trits[11]=(on?0:2);
	
	return encodeTelegram(trits);
}

/************
* BlokkerSwitch
************/

BlokkerSwitch::BlokkerSwitch(unsigned short pin, unsigned int periodusec) : RemoteSwitch(pin,periodusec,3) {
	//Call contructor
}


void BlokkerSwitch::sendSignal(unsigned short device, boolean on) {
	sendTelegram(getTelegram(device,on), _pin);
}

unsigned long BlokkerSwitch::getTelegram(unsigned short device, boolean on) {
	unsigned short trits[12]={0};
	
	device--;
	
	for (unsigned short i=1; i<4; i++) {
		//Bits 1-3 contain device 
		trits[i]=(device & 1)?0:1;          
		device>>=1;
    }
	
	//switch on or off
	trits[8]=(on?1:0);
	
	return encodeTelegram(trits);
}

/************
* KaKuSwitch
************/

KaKuSwitch::KaKuSwitch(unsigned short pin, unsigned int periodusec) : RemoteSwitch(pin,periodusec,3) {
	//Call contructor
}

void KaKuSwitch::sendSignal(char address, unsigned short device, boolean on) {
	sendTelegram(getTelegram(address, device, on), _pin);
}

unsigned long KaKuSwitch::getTelegram(char address, unsigned short device, boolean on) {
	unsigned short trits[12];
	
	address-=65;
	device-=1;
	
	for (unsigned short i=0; i<4; i++) {
		//bits 0-3 contain address (2^4 = 16 addresses)
		trits[i]=(address & 1)?2:0;          
		address>>=1;
		
		//bits 4-8 contain device (2^4 = 16 addresses)
		trits[i+4]=(device & 1)?2:0;          
		device>>=1;
    }
	
	//bits 8-10 seem to be fixed
	trits[8]=0;
	trits[9]=2;
	trits[10]=2;
	
	//switch on or off
	trits[11]=(on?2:0);
	
	return encodeTelegram(trits);
}

void KaKuSwitch::sendSignal(char address, unsigned short group, unsigned short device, boolean on) {
	sendTelegram(getTelegram(address, group, on), _pin);
}

unsigned long KaKuSwitch::getTelegram(char address, unsigned short group, unsigned short device, boolean on) {
	unsigned short trits[12], i;
	
	address-=65;
	group-=1;
	device-=1;
	
	//address. M3E Pin A0-A3
	for (i=0; i<4; i++) {
		//bits 0-3 contain address (2^4 = 16 addresses)
		trits[i]=(address & 1)?2:0;          
		address>>=1;		
    }
		
	//device. M3E Pin A4-A5
	for (; i<6; i++) {
		trits[i]=(device & 1)?2:0;          
		device>>=1;
	}
	
	//group. M3E Pin A6-A7
	for (; i<8; i++) {
		trits[i]=(group & 1)?2:0;          
		group>>=1;
	}
	
	//bits 8-10 are be fixed. M3E Pin A8/D0-A10/D2
	trits[8]=0;
	trits[9]=2;
	trits[10]=2;
	
	//switch on or off, M3E Pin A11/D3
	trits[11]=(on?2:0);
	
	return encodeTelegram(trits);
}
