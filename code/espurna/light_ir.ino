/*

LIGHT (EXPERIMENTAL) IR

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2017 by François Déchery 

------------------------------------------------------------------------------------------
Features :
 - ONLY RGB strips are supported (No RGBW or RGBWW)
 - IR remote supported (but not mandatory)
 - HSV (intuitive) WEB controls
 - MQTT & API "color_rgb" + "color_hsv" + "brightness" parameters
 - Uses the (amazing) Fastled library for fast and natural Color & Brightness
 - Several Animation Modes and Speed (controlled from Remote + Web + MQTT + API)
------------------------------------------------------------------------------------------
Not currently Implemented :
 - Saving/Restoring Settings
 - HomeAssistant
------------------------------------------------------------------------------------------

*/


#ifdef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <FastLED.h>

// #### Defined ##########################################################################
#define ANIM_SPEED_STEP 20
#define ANIM1_SPEED 350			// flash ON Variable
#define ANIM1_PAUSE 200			// flash OFF fixed
#define ANIM2_SPEED 550			// strobe OFF variable
#define ANIM2_PAUSE 150			// storbe ON fixed
#define ANIM3_SPEED 100			// fade speed
#define ANIM4_SPEED 700			// smooth speed
#define ANIM5_SPEED 200			// party speed

#define BUTTONS_COUNT 24
#define LED_DURATION 70			// Status led ON duration

// #### Variables ########################################################################
unsigned long r_but_codes[]={		// IR remote buttons codes
	IR_BUTTON_0  , //	Brightness +
	IR_BUTTON_1  , //	Brightness -
	IR_BUTTON_2  , //	OFF
	IR_BUTTON_3  , //	ON
	IR_BUTTON_4  , //	Red
	IR_BUTTON_5  , //	Green
	IR_BUTTON_6  , //	Blue
	IR_BUTTON_7  , //	White
	IR_BUTTON_8  , //	R1
	IR_BUTTON_9  , //	G1
	IR_BUTTON_10 , //	B1
	IR_BUTTON_11 , //	Flash
	IR_BUTTON_12 , //	R2
	IR_BUTTON_13 , //	G2
	IR_BUTTON_14 , //	B2
	IR_BUTTON_15 , //	Strobe
	IR_BUTTON_16 , //	R3
	IR_BUTTON_17 , //	G3
	IR_BUTTON_18 , //	B3
	IR_BUTTON_19 , //	Fade
	IR_BUTTON_20 , //	R4
	IR_BUTTON_21 , //	G4
	IR_BUTTON_22 , //	B4
	IR_BUTTON_23	//	Smooth
};

unsigned long r_but_colors[]={	// IR remote buttons colors
	0,			//	Brightness +
	0,			//	Brightness -
	0,			//	OFF
	0,			//	ON
	0xFF0000,	//	Red
	0x00FF00,	//	Green
	0x0000FF,	//	Blue
	0xFFFFFF,	//	White
	0xD13A01,	//	R1
	0x00E644,	//	G1
	0x0040A7,	//	B1
	0,			//	Flash
	0xE96F2A,	//	R2
	0x00BEBF,	//	G2
	0x56406F,	//	B2
	0,			//	Strobe
	0xEE9819,	//	R3
	0x00799A,	//	G3
	0x944E80,	//	B3
	0,			//	Fade
	0xFFFF00,	//	R4
	0x0060A1,	//	G4
	0xEF45AD,	//	B4
	0			//	Smooth
};

// variables declarations ###############################################################
CHSV 			_cur_color				= CHSV(0,255,255);
CHSV 			_cur_anim_color			= CHSV(0,0,0);
byte 			_cur_status  			= 0 ;
byte 			_cur_anim_mode  		= 0 ;
byte 			_cur_anim_step  		= 0;
boolean 		_cur_anim_dir	  		= true;
unsigned long	_cur_anim_speed 		= 1000;
unsigned long	_anim_last_update 		= millis();
unsigned long	_last_ir_button			= 0;
unsigned long	_last_status_led_time	= 0;

IRrecv _ir_recv(LIGHT_IR_PIN); 		//IRrecv _ir_recv(IR_PIN, IR_LED_PIN); dont work. Why ?
decode_results _ir_results;


// #######################################################################################
// #### PRIVATE ##########################################################################
// #######################################################################################

// ---------------------------------------------------------------------------------------
void _updateStatusLed(){
	if(millis() > _last_status_led_time + LED_DURATION  ){
		digitalWrite(LED1_PIN, LOW);
	}
}

// ---------------------------------------------------------------------------------------
void _flashStatusLed(){
	digitalWrite(LED1_PIN, HIGH);
	_last_status_led_time=millis();
}

// ---------------------------------------------------------------------------------------
void _loopProcessIR() {
	if (_ir_recv.decode(&_ir_results)) {
		//dumpIR(&_ir_results);
	    //DEBUG_MSG_P(PSTR(".\n"));
		unsigned long code = _ir_results.value;
		DEBUG_MSG_P(PSTR("[IR] received : 0x%X "), code );
		if( code == 0xFFFFFFFF){
			code = _last_ir_button;
			DEBUG_MSG_P(PSTR("(Repeat : %X) "), code );
		}
		DEBUG_MSG_P(PSTR("=> ") );
		_processIrButtons(code);
		_ir_recv.resume(); // Receive the next value
	}
}

// ---------------------------------------------------------------------------------------
void _processIrButtons(unsigned long code) {
	//DEBUG_MSG_P(PSTR("IR code : %X\n"), code );
	boolean done=false;
	for (int i = 0; i < BUTTONS_COUNT ; i = i + 1) {
		if( code == r_but_codes[i] ){
		    //DEBUG_MSG_P(PSTR(" : %X  -> "), r_but_colors[i] );

			_last_ir_button = 0; //no repat else if specified
				
			if(i == 0){
				_buttonBrightness(true);
				_last_ir_button = code;
				delay(150); //debounce
			}		 
			else if(i == 1){
				_buttonBrightness(false);
				_last_ir_button = code;
				delay(150); //debounce
			}		 
			else if(i == 2){
				_buttonPower(false);
			}		 
			else if(i == 3){
				_buttonPower(true);
			}		 
			else if(i == 11){
				_buttonAnimMode(1);
			}		 
			else if(i == 15){
				_buttonAnimMode(2);
			}
			else if(i == 19){
				_buttonAnimMode(3);
			}		 
			else if(i == 23){
				_buttonAnimMode(4);
			}
			else{
				_buttonColorRVB(r_but_colors[i],0);
			}
			done=true;
			lightUpdate(true, true);
		}
	}
	if(!done){
		_last_ir_button = 0; 
		//DEBUG_PRINTHEX(code);
		DEBUG_MSG_P(PSTR("ignored!\n"));
	}
}

// ---------------------------------------------------------------------------------------
void _buttonPower(boolean on){
	_flashStatusLed();
	DEBUG_MSG_P(PSTR("BUT Power: "));
	_cur_anim_mode=0;
	if(on){
		_cur_status=1;
		DEBUG_MSG_P(PSTR("ON : "));
		_setLedsHSV(_cur_color);
	}
	else{
		_cur_status=0;
		DEBUG_MSG_P(PSTR("OFF : "));
		_setLedsHSV(CHSV {0,0,0});
	}
}

// ---------------------------------------------------------------------------------------
void _buttonAnimMode(byte val){
	_flashStatusLed();
	DEBUG_MSG_P(PSTR("BUT Anim Mode: %d\n"),val);
	_processAnimation(val, true, true);
}

// ---------------------------------------------------------------------------------------
void _setAnimMode(byte val){
	DEBUG_MSG_P(PSTR("[LIGHT] Set AnimMode: %d\n"),val);
	_processAnimation(val, true, false);
}

// ---------------------------------------------------------------------------------------
void _buttonBrightness(boolean up){
	DEBUG_MSG_P(PSTR("BUT Brightness: "));
	if(up){
		if(_cur_anim_mode==0){
			DEBUG_MSG_P(PSTR("UP : "));
			_buttonColorHSV(_cur_color, 1 );
		}
		else{
			DEBUG_MSG_P(PSTR("FASTER\n"));
			_buttonChangeSpeed( - ANIM_SPEED_STEP );
		}
	}
	else{
		if(_cur_anim_mode==0){
			DEBUG_MSG_P(PSTR("DOWN  : "));
			_buttonColorHSV(_cur_color, -1 );
		}
		else{
			DEBUG_MSG_P(PSTR("SLOWER\n"));
			_buttonChangeSpeed( ANIM_SPEED_STEP );
		}
	}
}

// ---------------------------------------------------------------------------------------
void _buttonColorHSV(CHSV color, int offset){
	_flashStatusLed();
	DEBUG_MSG_P(PSTR("[LIGHT] Set Color to : "));
	//DEBUG_MSG_P(PSTR("(from HSV=%d,%d,%d) "), color.h, color.s, color.v);
	color=_dimHSV(color,offset);
	//DEBUG_MSG_P(PSTR("(to HSV=%d,%d,%d) "), color.h, color.s, color.v);
	_setLedsHSV(color);
	if(color.v ==0 ){
		_cur_status=0;
	}
	else{
		_cur_status=1;
	}
	_cur_color=color;
	_romSaveCurrentColor();
}

// ---------------------------------------------------------------------------------------
void _buttonColorRVB(CRGB color, int offset){
	_buttonColorHSV(_rgbToHsv(color), offset);
}

// ---------------------------------------------------------------------------------------
void _buttonChangeSpeed(int offset){
	_flashStatusLed();
	if(offset !=0){
		_cur_anim_speed=_cur_anim_speed + offset ;
	}
}

// ---------------------------------------------------------------------------------------
CHSV _dimHSV(CHSV color, int offset){
	offset=offset*10;
	int bright=color.v + offset;
	if(offset ==0){
		return color;	
	}
	else if(bright < 1){
		bright=1; // no off
	}
	else if(bright > 255){
		bright=255;
	}
	color.v=bright;
	return color;
}

// ---------------------------------------------------------------------------------------
void _setBrightnessMapped(byte val){
	DEBUG_MSG_P(PSTR("[LIGHT] Set from 0-100 Brightness : %u => "), val);
	val =map(val,0,100,0,255);
	_setBrightness(val);
}

// ---------------------------------------------------------------------------------------
void _setBrightness(byte val){
	DEBUG_MSG_P(PSTR("[LIGHT] Set Brightness to : %u\n"), val);
	_cur_color.v      = val;
	_cur_anim_color.v = val;
	if(val==0){
		_cur_status=0;
	}
	_buttonColorHSV(_cur_color,0);	
}

// ---------------------------------------------------------------------------------------
void _setAnimSpeed(unsigned long speed){
	if(speed !=0){
		_cur_anim_speed=speed ;
	}
}

// ---------------------------------------------------------------------------------------
void _setLedsRGB(CRGB rgb){
	analogWrite(LIGHT_CH1_PIN, rgb.r);
	analogWrite(LIGHT_CH2_PIN, rgb.g);
	analogWrite(LIGHT_CH3_PIN, rgb.b);	
	if(_cur_anim_mode == 0){
		DEBUG_MSG_P(PSTR("RGB=%3u,%3u,%3u\n"), rgb.r, rgb.g, rgb.b);
	}
}

// ---------------------------------------------------------------------------------------
void _setLedsHSV(CHSV hsv){
	if(_cur_anim_mode == 0){
		DEBUG_MSG_P(PSTR("HSV=%3u,%3u,%3u - "), hsv.h, hsv.s, hsv.v);
	}
	_setLedsRGB( CHSV(hsv) );
}

// ---------------------------------------------------------------------------------------
void _confirmFlash(){
	_setLedsRGB(CRGB::Black);
	delay(70);

	_setLedsRGB(CRGB::White);
	delay(70);

	_setLedsRGB(CRGB::Black);
	delay(70);

	_setLedsRGB(CRGB::White);
	delay(70);

	_setLedsRGB(CRGB::Black);
	delay(70);
}

// ---------------------------------------------------------------------------------------
void _processAnimation(byte mode, boolean init, boolean is_button){
	if(init){
		if(_cur_anim_mode == mode && is_button){
			_confirmFlash();
			DEBUG_MSG_P(PSTR("[ANIM_%d] Stopped !!!\n"), mode);
			_cur_anim_mode=0;
			_cur_status=0;
			return;
		}
		else if(mode == 0){
			DEBUG_MSG_P(PSTR("[ANIM_%d] Stopped !!!\n"), mode);
			_cur_anim_mode=0;
			_cur_status=0;
			return;
		}
		else{
			if(is_button){
				_confirmFlash();
			}
			DEBUG_MSG_P(PSTR("[ANIM_%d] Started !!!\n"), mode);
		}
	}

	if(mode==1){
		_anim1(init);
	}
	else if(mode==2){
		_anim2(init);
	}
	else if(mode==3){
		_anim3(init);
	}
	else if(mode==4){
		_anim4(init);
	}
	else if(mode==5){
		_anim5(init);
	}
	else{
		//invalid mode
	}
}

// ---------------------------------------------------------------------------------------
// anim1 : flash
void _anim1(boolean init){
	if(init){
		_cur_anim_mode  = 1;
		_cur_anim_speed = ANIM1_SPEED;
		_cur_status     = 1;
		_cur_anim_step  = 1;
	}
	unsigned long now= millis();
	if(_cur_anim_step==1 && now > (_anim_last_update + _cur_anim_speed) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV(_cur_color);
		_cur_anim_step=0;
		_anim_last_update = now;
	}
	else if(_cur_anim_step==0 && now > (_anim_last_update + ANIM1_PAUSE) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV(CHSV {0,0,0});
		_cur_anim_step=1;
		_anim_last_update = now;
	}
}

// ---------------------------------------------------------------------------------------
// anim2 : strobe
void _anim2(boolean init){
	if(init){
		_cur_anim_mode  = 2;
		_cur_anim_speed = ANIM2_SPEED;
		_cur_status     = 1;
		_cur_anim_step  = 1;
	}
	unsigned long now= millis();
	if(_cur_anim_step==1 && now > (_anim_last_update + ANIM2_PAUSE) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV(_cur_color);
		_cur_anim_step=0;
		_anim_last_update = now;
	}
	else if(_cur_anim_step==0 && now > (_anim_last_update + _cur_anim_speed) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV(CHSV {0,0,0});
		_cur_anim_step=1;
		_anim_last_update = now;
	}
}

// ---------------------------------------------------------------------------------------
// anim3 : fade
void _anim3(boolean init){
	if(init){
		_cur_anim_mode  = 3;
		_cur_anim_speed = ANIM3_SPEED;
		_cur_status     = 1;
		_cur_anim_step  = _cur_color.v;
	}

	unsigned long now= millis();
	if( now > (_anim_last_update + _cur_anim_speed) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV( CHSV(_cur_color.h, _cur_color.s, dim8_lin(_cur_anim_step)) );
		if(_cur_anim_dir){
			if(_cur_anim_step == 255){
				_cur_anim_dir=false;
			}
			else{
				_cur_anim_step++;
			}
		}
		else{
			if(_cur_anim_step == 1){
				_cur_anim_dir=true;
			}
			else{
				_cur_anim_step--;
			}
		}
		_anim_last_update = now;
	}

}

// ---------------------------------------------------------------------------------------
// anim4 : smooth
void _anim4(boolean init){
	if(init){
		_cur_anim_mode    = 4;
		_cur_anim_speed   = ANIM4_SPEED;
		_cur_status       = 1;
		_cur_anim_step    = 0;
		_cur_anim_color   = _cur_color;
		_cur_anim_color.v = 255;
	}

	unsigned long now= millis();
	if( now > (_anim_last_update + _cur_anim_speed) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_cur_anim_color.h=_cur_anim_step;
		_cur_anim_color.s=255;
		_setLedsHSV( CHSV(_cur_anim_step,255,255) );
		_cur_anim_step++;
		_anim_last_update = now;
		if(_cur_anim_step > 255){
			_cur_anim_step=0;
		}
	}
}

// ---------------------------------------------------------------------------------------
// anim5 : party
void _anim5(boolean init){
	if(init){
		_cur_anim_mode   = 5;
		_cur_anim_speed  = ANIM5_SPEED;
		_cur_status      = 1;
		_cur_anim_step   = 0;
		_cur_anim_color  = _cur_color;
		_cur_anim_color.s= 255;
		_cur_anim_color.v= 255;
	}
	unsigned long now= millis();
	if(_cur_anim_step == 1 && now > (_anim_last_update + _cur_anim_speed) ){ 
		DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_cur_anim_color.h = random(0,255); 
		_setLedsHSV(_cur_anim_color);
		_cur_anim_step    = 0;
		_anim_last_update = now;
	}
	else if(_cur_anim_step==0 && now > (_anim_last_update + _cur_anim_speed + 15) ){
		//DEBUG_MSG_P(PSTR("[ANIM_%d] Update : "), _cur_anim_mode);
		_setLedsHSV(CHSV {0,0,0});
		_cur_anim_step    = 1;
		_anim_last_update = now;
	}
}

// ---------------------------------------------------------------------------------------
void _loopUpdateAnimation(){
	_processAnimation(_cur_anim_mode, false, false);
}

// ---------------------------------------------------------------------------------------
CHSV _rgbToHsv(CRGB rgb){
/*
	if(rgb.r <=1 && rgb.g <=1 && rgb.b <=1){
		DEBUG_MSG_P(PSTR(" {Rounding RGB to black} "));
		return CHSV {0,0,0};
	}
*/
     return rgb2hsv_approximate(rgb);
}

// ---------------------------------------------------------------------------------------
void _romSaveCurrentColor(){
	//save it to 1,2,3 positions
	//but dont stress eeprom if not needed
/*
	CHSV rom =_romLoadColor();
	if(_cur_color.h != rom.h){gw.saveState(1,_cur_color.h);}
	if(_cur_color.s != rom.s){gw.saveState(2,_cur_color.s);}
	if(_cur_color.v != rom.v){gw.saveState(3,_cur_color.v);}
*/
}

// ---------------------------------------------------------------------------------------
CHSV _romLoadColor(){
	//load from 1,2,3 positions
	CHSV color;
/*
	color.h=gw.loadState(1);
	color.s=gw.loadState(2);
	color.v=gw.loadState(3);
*/
	return color;
}

// ----------------------------------------------
CRGB _longToRgb(unsigned long rgb){
	CRGB out;
	out.r = rgb >> 16;
	out.g = rgb >> 8 & 0xFF;
	out.b = rgb & 0xFF;
	return out;
}

// ---------------------------------------------------------------------------------------
CHSV _longToHsv(unsigned long hsv){
	CHSV out;
	out.h = hsv >> 16;
	out.s = hsv >> 8 & 0xFF;
	out.v = hsv & 0xFF;
	return out;
}

// ---------------------------------------------------------------------------------------
unsigned long _rgbToLong(CRGB in){
	return (((long)in.r & 0xFF) << 16) + (((long)in.g & 0xFF) << 8) + ((long)in.b & 0xFF);
}

// ---------------------------------------------------------------------------------------
unsigned long _hsvToLong(CHSV in){
	return (((long)in.h & 0xFF) << 16) + (((long)in.s & 0xFF) << 8) + ((long)in.v & 0xFF);
}

// ---------------------------------------------------------------------------------------
CRGB _charToRgb(const char * rgb) {

    char * p = (char *) rgb;

    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {
           ++p;
 	}
    return _longToRgb( strtoul(p, NULL, 16) );
}

// ---------------------------------------------------------------------------------------
CHSV _charToHsv(const char * rgb) {

    char * p = (char *) rgb;

    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {
           ++p;
 	}
    return _longToHsv( strtoul(p, NULL, 16) );
}


// ---------------------------------------------------------------------------------------
boolean _charColorIsValid(const char * rgb){
    char * p = (char *) rgb;
    if (strlen(p) == 6 || strlen(p) == 7 ){
    	return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------
void _CurrentColorToRGB(char * buffer) {
    snprintf_P(buffer, 8, PSTR("%06X"), _rgbToLong( CHSV(_cur_color)));
}

// ---------------------------------------------------------------------------------------
void _CurrentColorToHSV(char * buffer) {
    snprintf_P(buffer, 8, PSTR("%06X"), _hsvToLong( _cur_color));
}

// ---------------------------------------------------------------------------------------
void _lightColorRestore() {
/*
    for (unsigned int i=0; i < _channels.size(); i++) {
        _channels[i].value = getSetting("ch", i, 0).toInt();
    }
    _brightness = getSetting("brightness", LIGHT_MAX_BRIGHTNESS).toInt();
    lightUpdate(false, false);
*/
}


// ---------------------------------------------------------------------------------------
void _lightAPISetup() {

    #if WEB_SUPPORT

        // API entry points (protected with apikey)
        if (lightHasColor()) {

            apiRegister(MQTT_TOPIC_COLOR_RGB, MQTT_TOPIC_COLOR_RGB,
                [](char * buffer, size_t len) {
                    _CurrentColorToRGB(buffer);
                },
                [](const char * payload) {
                    _SetLightColorRGB(payload);
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_COLOR_HSV, MQTT_TOPIC_COLOR_HSV,
                [](char * buffer, size_t len) {
                    _CurrentColorToHSV(buffer);
                },
                [](const char * payload) {
                    _SetLightColorHSV(payload);
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_BRIGHTNESS, MQTT_TOPIC_BRIGHTNESS,
                [](char * buffer, size_t len) {
        			snprintf_P(buffer, len, PSTR("%d"), _cur_color.v);
                },
                [](const char * payload) {
                    _setBrightness(atoi(payload));
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_ANIM_MODE, MQTT_TOPIC_ANIM_MODE,
                [](char * buffer, size_t len) {
        			snprintf_P(buffer, len, PSTR("%d"), _cur_anim_mode);
                },
                [](const char * payload) {
                    _setAnimMode(atoi(payload));
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_ANIM_SPEED, MQTT_TOPIC_ANIM_SPEED,
                [](char * buffer, size_t len) {
        			snprintf_P(buffer, len, PSTR("%d"), _cur_anim_speed);
                },
                [](const char * payload) {
                    _setAnimSpeed(atoi(payload));
                    lightUpdate(true, true);
                }
            );


        }
/*
        for (unsigned int id=0; id<lightChannels(); id++) {

            char url[15];
            snprintf_P(url, sizeof(url), PSTR("%s/%d"), MQTT_TOPIC_CHANNEL, id);

            char key[10];
            snprintf_P(key, sizeof(key), PSTR("%s%d"), MQTT_TOPIC_CHANNEL, id);

            apiRegister(url, key,
                [id](char * buffer, size_t len) {
    				snprintf_P(buffer, len, PSTR("%d"), lightChannel(id));
                },
                [id](const char * payload) {
                    lightChannel(id, atoi(payload));
                    lightUpdate(true, true);
                }
            );

        }
*/
    #endif // WEB_SUPPORT

}


/*
// #######################################################################################

// ---------------------------------------------------------------------------------------
CRGB _hsvToRgb(CHSV hsv){
	if(hsv.v <=1){
		DEBUG_MSG_P(PSTR("	{Rounding HSV to black}\n"));
		hsv.v=0;
	}
	CRGB rgb=CHSV(hsv);
	//round to black
	if(rgb.r <=1 && rgb.g <=1 && rgb.b <=1){
		DEBUG_MSG_P(PSTR("	{Rounding RGB to black}\n"));
		rgb=CRGB::Black;
	}
	return rgb;
}


// ---------------------------------------------------------------------------------------
void _strToUpper(char * str){
    for(int i=0; i<6; i++){
    	//a-z
        if (97<=str[i]&&str[i]<=122){
            str[i]-=32;
        }
    }
}

*/



// #######################################################################################
// #### PUBLIC ###########################################################################
// #######################################################################################


// ################################################################
void lightSetup() {

    DEBUG_MSG_P(PSTR("[LIGHT] LIGHT_PROVIDER = %d (With IR)\n"), LIGHT_PROVIDER);

	pinMode(LIGHT_CH1_PIN, OUTPUT);		
	pinMode(LIGHT_CH2_PIN, OUTPUT);
	pinMode(LIGHT_CH3_PIN, OUTPUT);

	_ir_recv.enableIRIn(); // Start the receiver

	//confirmRgb();
	
	_cur_color = _romLoadColor();

    _lightColorRestore();
    _lightAPISetup();
    mqttRegister(_lightMQTTCallback);


}

// ---------------------------------------------------------------------------------------
void lightLoop() {
	_loopProcessIR();
	_loopUpdateAnimation();
	_updateStatusLed();
}

// ---------------------------------------------------------------------------------------
void lightUpdate(bool save, bool forward) {
	//DEBUG_MSG_P(PSTR("[LIGHT] Updating... \n"));

//    _lightProviderUpdate();

    // Report color & brightness to MQTT broker
    if (forward) lightMQTT();

    // Report color to WS clients (using current brightness setting)
    #if WEB_SUPPORT
    {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["colorVisible"] = 1;
        root["useColor"] = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
        root["useWhite"] = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;
        root["useGamma"] = getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1;

	    root["anim_mode"] = lightAnimMode();
	    root["anim_speed"] = lightAnimSpeed();
	    JsonObject& color_hsv = root.createNestedObject("color_hsv");
        color_hsv["h"] = lightColorH();
        color_hsv["s"] = lightColorS();
        color_hsv["v"] = lightColorV();

    	//root["color_hsv"]  = lightColor();
        //root["brightness"] = _cur_color.v;

		// RGB channels        
        //JsonArray& channels = root.createNestedArray("channels");
        //for (unsigned char id=0; id < lightChannels(); id++) {
        //    channels.add(lightChannel(id));
        //}
	    
	    // Relay
	    JsonArray& relay = root.createNestedArray("relayStatus");
		relay.add(_cur_status);

        String output;
        root.printTo(output);
        wsSend(output.c_str());
        //DEBUG_MSG_P(PSTR("JSON : %s"), output.c_str() );
    }
    #endif

    // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
    //if (save) colorTicker.once(LIGHT_SAVE_DELAY, _lightColorSave);
}


// ---------------------------------------------------------------------------------------
void lightMQTT() {

    char buffer[8];

    if (lightHasColor()) {

        // RGB Color
        _CurrentColorToRGB(buffer);
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        // HSV Color
        _CurrentColorToHSV(buffer);
        mqttSend(MQTT_TOPIC_COLOR_HSV, buffer);

        // Brightness
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _cur_color.v);
        mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

        // Anim Mode
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _cur_anim_mode);
        mqttSend(MQTT_TOPIC_ANIM_MODE, buffer);

        // Anim Speed
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _cur_anim_speed);
        mqttSend(MQTT_TOPIC_ANIM_SPEED, buffer);
    }

    // Channels
    //for (unsigned int i=0; i < _channels.size(); i++) {
    //    snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _channels[i].value);
    //    mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    //}
}

// ---------------------------------------------------------------------------------------
// Get Number of Channels
unsigned char lightChannels() {
	return 3;
}

// ---------------------------------------------------------------------------------------
// Get Channel's Value
unsigned int lightChannel(unsigned char id) {

	CRGB current_rvb = CHSV(_cur_color);
	if(id == 0 ){
		return current_rvb.r;
	}
	else if(id == 1 ){
		return current_rvb.g;
	}
	else if(id == 2 ){
		return current_rvb.b;
	}
	else{
		DEBUG_MSG_P(PSTR("    [ERROR] GET lightChannel : %s\n"), id);	
	    return 0;
	}
}

// ---------------------------------------------------------------------------------------
// Set Channel's Value
void lightChannel(unsigned char id, unsigned int value) {
	DEBUG_MSG_P(PSTR("[WEB|API] Set Color Channel "));	
	value= constrain(value, 0, 255);
	CRGB current_rvb = CHSV(_cur_color);

	if(id == 0 ){
		DEBUG_MSG_P(PSTR("RED  to : %d => "), value);	
		current_rvb.r=value;
		_buttonColorRVB(current_rvb,0);
	}
	else if(id == 1 ){
		DEBUG_MSG_P(PSTR("GREEN  to : %d => "), value);	
		current_rvb.g=value;
		_buttonColorRVB(current_rvb,0);
	}
	else if(id == 2 ){
		DEBUG_MSG_P(PSTR("BLUE  to : %d => "), value);	
		current_rvb.b=value;
		_buttonColorRVB(current_rvb,0);
	}
	else{
		DEBUG_MSG_P(PSTR("    [ERROR] SET lightChannel %s To %d \n"), id, value);	
	}
}

// ---------------------------------------------------------------------------------------
// Get Brightness
unsigned int lightBrightness() {
    return _cur_color.v;
}

// ---------------------------------------------------------------------------------------
// Set Brightness
void lightBrightness(unsigned int b) {
	b=constrain(b, 0, 255);
	DEBUG_MSG_P(PSTR("[WEB|API] Set Brightness to : %d\n"), b);	
	_cur_color.v=b;
	_setLedsHSV(_cur_color);
	
	//set status
	if(b > 0){
		_cur_status=1;
	}
	else{
		_cur_status=0;
	}
}

// ---------------------------------------------------------------------------------------
// Get Color
String lightColor() {
	char rgb[8];
    snprintf_P(rgb, 8, PSTR("#%06X"), _rgbToLong( CHSV(_cur_color)));
    return String(rgb);
	//return String("");
}

String lightColorH(){
	return String(_cur_color.h);
}
String lightColorS(){
	return String(_cur_color.s);
}
String lightColorV(){
	return String(_cur_color.v);
}


// ---------------------------------------------------------------------------------------
// Set Color
void lightColor(const char * color) {
	//used only from settings
	_SetLightColorRGB(color);
}

void _SetLightColorRGB(const char * color) {
//used only from settings
	DEBUG_MSG_P(PSTR("[WEB|API] Set (#RGB) Color to : "));	
	if( _charColorIsValid(color) ){
		DEBUG_MSG_P(PSTR("%s \n"), color);	
		_buttonColorRVB(_charToRgb(color), 0);
	}
	else{
		DEBUG_MSG_P(PSTR(" Canceled ('%s' is invalid) !\n"), color);	
	}
}

void _SetLightColorHSV(const char * color) {
	DEBUG_MSG_P(PSTR("[WEB|API] Set (#HSV) Color to : "));	
	if( _charColorIsValid(color) ){
		DEBUG_MSG_P(PSTR("%s \n"), color);	
		_buttonColorHSV(_charToHsv(color), 0);
	}
	else{
		DEBUG_MSG_P(PSTR(" Canceled ('%s' is invalid) !\n"), color);	
	}
}

void setLightColor (const char * h, const char * s, const char * v){
	DEBUG_MSG_P(PSTR("[WEB|API] Set Color from (%s,%s,%s) "), h, s, v);	
	CHSV color;
	color.h=strtoul(h, NULL, 10);
	color.s=strtoul(s, NULL, 10);
	color.v=strtoul(v, NULL, 10);
	DEBUG_MSG_P(PSTR("to (%d,%d,%d) "), color.h, color.s, color.v);	
	_buttonColorRVB(color, 0);
}


// ---------------------------------------------------------------------------------------
bool lightHasColor() {
//    bool useColor = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
//    return useColor && (_channels.size() > 2);
	//if(_cur_status || _cur_anim_mode){return 1;}
	return true;
}


// ---------------------------------------------------------------------------------------
// Get State
bool lightState() {
	//DEBUG_MSG_P(PSTR("[->LIGHT] _cur_status is : %d \n"),_cur_status);
	return _cur_status;
}

// ---------------------------------------------------------------------------------------
// Set State
void lightState(bool state){
	DEBUG_MSG_P(PSTR("[WEB|API] Set Relay to : %u => "), state);	
	//if(state != _cur_status){
		_buttonPower(state);
	//}
}


// ---------------------------------------------------------------------------------------
String lightAnimMode(){
	return String(_cur_anim_mode);
}
void lightAnimMode(const char * val){
	DEBUG_MSG_P(PSTR("[WEB|API] Set AnimMode to %s\n"), val);	
	_setAnimMode(strtoul(val, NULL, 10));
}
// ---------------------------------------------------------------------------------------
String lightAnimSpeed(){
	return String(_cur_anim_speed);
}
void lightAnimSpeed(const char * val){
	DEBUG_MSG_P(PSTR("[WEB|API] Set AnimSpeed to %s \n"), val);	
	_setAnimSpeed(strtoul(val, NULL, 10));
}


// ---------------------------------------------------------------------------------------
// MQTT
// ---------------------------------------------------------------------------------------

void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {


    if (type == MQTT_CONNECT_EVENT) {

        if (lightHasColor()) {
            mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
            mqttSubscribe(MQTT_TOPIC_ANIM_MODE);
            mqttSubscribe(MQTT_TOPIC_ANIM_SPEED);
        }

        //char buffer[strlen(MQTT_TOPIC_CHANNEL) + 3];
        //snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_CHANNEL);
        //mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);

        // Color RGB
        if (t.equals(MQTT_TOPIC_COLOR_RGB)) {
            _SetLightColorRGB(payload);
            lightUpdate(true, mqttForward());
        }

        // Color HSV
        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            _SetLightColorHSV(payload);
            lightUpdate(true, mqttForward());
        }

        // ANIM Mode
        if (t.equals(MQTT_TOPIC_ANIM_MODE)) {
            _setAnimMode(atoi(payload));
            lightUpdate(true, mqttForward());
        }

        // ANIM Speed
        if (t.equals(MQTT_TOPIC_ANIM_SPEED)) {
            _setAnimSpeed(atoi(payload));
            lightUpdate(true, mqttForward());
        }
        
        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _setBrightness (constrain(atoi(payload), 0, LIGHT_MAX_BRIGHTNESS));
            lightUpdate(true, mqttForward());
        }

/*
        // Channel
        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            unsigned int channelID = t.substring(strlen(MQTT_TOPIC_CHANNEL)+1).toInt();
            if (channelID >= _channels.size()) {
                DEBUG_MSG_P(PSTR("[LIGHT] Wrong channelID (%d)\n"), channelID);
                return;
            }
            lightChannel(channelID, atoi(payload));
            lightUpdate(true, mqttForward());
        }
*/
    }
}

#endif // LIGHT_IR_PIN
