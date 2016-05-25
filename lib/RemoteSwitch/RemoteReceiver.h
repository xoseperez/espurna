/*
 * RemoteSwitch library v2.0.0 made by Randy Simons http://randysimons.nl
 *
 * License: "Free BSD license". See license.txt
 */

#ifndef RemoteReceiver_h
#define RemoteReceiver_h

//#include "WProgram.h"
#include "Arduino.h"

typedef void (*RemoteReceiverCallBack)(unsigned long, unsigned int);

/**
* See RemoteSwitch for introduction.
*
* RemoteReceiver decodes the signal received from a 433MHz-receiver, like the "KlikAanKlikUit"-system
* as well as the signal sent by the RemoteSwtich class. When a correct signal is received,
* a user-defined callback function is called.
*
* Note that in the callback function, the interrupts are still disabled. You can enabled them, if needed.
* A call to the callback must b finished before RemoteReceiver will call the callback function again, thus
* there is no re-entrant problem.
*
* When sending your own code using RemoteSwich, disable() the receiver first.
*
* This is a pure static class, for simplicity and to limit memory-use.
*/

class RemoteReceiver {
	public:
		/**
		* Initializes the decoder.
		*
		* @param interrupt The interrupt as is used by Arduino's attachInterrupt function. See attachInterrupt for details.
		* @param minRepeats The number of times the same code must be received in a row before the callback is calles
		* @param callback Pointer to a callback function, with signature void (*func)(unsigned long, unsigned int). First parameter is the decoded data, the second the period of the timing.
		*/
		static void init(unsigned short interrupt, unsigned short minRepeats, RemoteReceiverCallBack callback);

		/**
		* Enabled decoding. No need to call enable() after init().
		*/
		static void enable();

		/**
		* Disable decoding. You can enable decoding by calling enable();
		*/
		static void disable();

		/**
		* Tells wether a signal is being received. Since it makes no sense to transmit while another transmitter is active,
		* it's best to wait for isReceiving() to false.
		*
		* Note: isReceiving() depends on interrupts enabled. Thus, when disabled()'ed, or when interrupts are disabled (as is
		* the case in the callback), isReceiving() will not work properly.

		* @param waitMillis number of milliseconds to monitor for signal.
		* @return boolean If after waitMillis no signal was being processed, returns false. If before expiration a signal was being processed, returns true.
		*/
		static boolean isReceiving(int waitMillis = 50);

	private:
		static void interruptHandler();

		static unsigned short _interrupt;			//Radio input interrupt
		volatile static int _state;		//State of decoding process. There are 49 states, 1 for "waiting for signal" and 48 for decoding the 48 edges in a valid code.
		static unsigned short _minRepeats;
		static RemoteReceiverCallBack _callback;
		static boolean _inCallback;					//When true, the callback function is being executed; prevents re-entrance.

};

#endif
