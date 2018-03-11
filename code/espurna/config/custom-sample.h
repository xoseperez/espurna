/*
	To modify the stock configuration without changing any of the *.h files in this folder:

	1) rename this file to "custom.h" (It is ignored by Git)

	2) define your own settings below 

    3) define USE_CUSTOM_H as a build flags
        * Using Arduino IDE: **********************************************************
        Define it in your boards.txt for the board of your choice.
        For instance, for the "Generic ESP8266 Module" with prefix "generic" just add:

               generic.build.extra_flags=-DESP8266 -DUSE_CUSTOM_H

        * Using PlatformIO: ************************************************************
        Add the setting to your environment or just define global PLATFORMIO_BUILD_FLAGS

               export PLATFORMIO_BUILD_FLAGS="'-DUSE_CUSTOM_H'"
*/


// make the compiler show a warning to confirm that this file is inlcuded
#warning "**** Using Settings from custom.h File **********"



/*
#######################################################################################################
Your Own Default Settings
#######################################################################################################

	You can basically ovveride ALL settings.
	Don't forget to first #undef each existing #define that you add below.
	Here are some examples:
*/



//#undef	ADMIN_PASS
//#define ADMIN_PASS              "MyVerySecretPassort"

//#undef	NTP_SERVER
//#define NTP_SERVER              "it.pool.ntp.org"

//#undef WIFI1_SSID
//#define WIFI1_SSID              "MySSID"

//#undef WIFI1_PASS
//#define WIFI1_PASS              "MyVerySecretWifiPassword"
