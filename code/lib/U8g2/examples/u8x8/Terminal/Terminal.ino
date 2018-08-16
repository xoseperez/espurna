/*

  Terminal.ino
  
  Demo for U8log
  
  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include <Arduino.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

// Please UNCOMMENT one of the contructor lines below
// U8x8 Contructor List 
// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
// Please update the pin numbers according to your setup. Use U8X8_PIN_NONE if the reset pin is not connected
//U8X8_NULL u8x8;	// null device, a 8x8 pixel display which does nothing
//U8X8_SSD1306_128X64_NONAME_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1306_128X64_NONAME_4W_HW_SPI u8x8(/* cs=*/ 6, /* dc=*/ 4, /* reset=*/ 12);	// Arduboy (DevKit)
//U8X8_SSD1306_128X64_NONAME_4W_HW_SPI u8x8(/* cs=*/ 12, /* dc=*/ 4, /* reset=*/ 6);	// Arduboy 10 (Production, Kickstarter Edition)
//U8X8_SSD1306_128X64_NONAME_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1306_128X64_NONAME_3W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* reset=*/ 8);
//U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); 	      
//U8X8_SSD1306_128X64_ALT0_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); 	      // same as the NONAME variant, but may solve the "every 2nd line skipped" problem
//U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 2, /* data=*/ 0, /* reset=*/ U8X8_PIN_NONE); 	      // Digispark ATTiny85
//U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display
//U8X8_SSD1306_128X64_VCOMH0_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		// same as the NONAME variant, but maximizes setContrast() range
//U8X8_SSD1306_128X64_ALT0_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		// same as the NONAME variant, but may solve the "every 2nd line skipped" problem
//U8X8_SH1106_128X64_NONAME_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
//U8X8_SH1106_128X64_VCOMH0_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		// same as the NONAME variant, but maximizes setContrast() range
//U8X8_SH1106_128X64_WINSTAR_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		// same as the NONAME variant, but uses updated SH1106 init sequence
//U8X8_SH1106_72X40_WISE_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SH1107_64X128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SH1107_128X128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SH1107_128X128_HW_I2C u8x8(/* reset=*/ 8);
//U8X8_SH1107_SEEED_96X96_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SH1108_160X160_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED
//U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 21, /* data=*/ 20, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather M0 Basic Proto + FeatherWing OLED
//U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED
//U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C
//U8X8_SSD1306_64X48_ER_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   // EastRising 0.66" OLED breakout board, Uno: A4=SDA, A5=SCL, 5V powered
//U8X8_SSD1306_48X64_WINSTAR_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
//U8X8_SSD1306_64X32_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);  
//U8X8_SSD1306_64X32_1F_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);  
//U8X8_SSD1306_96X16_ER_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   // EastRising 0.69"
//U8X8_SSD1306_128X64_NONAME_6800 u8x8(13, 11, 2, 3, 4, 5, 6, A4, /*enable=*/ 7, /*cs=*/ 10, /*dc=*/ 9, /*reset=*/ 8);
//U8X8_SSD1306_128X64_NONAME_8080 u8x8(13, 11, 2, 3, 4, 5, 6, A4, /*enable=*/ 7, /*cs=*/ 10, /*dc=*/ 9, /*reset=*/ 8);
//U8X8_SSD1309_128X64_NONAME0_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_SSD1309_128X64_NONAME0_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_SSD1309_128X64_NONAME2_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_SSD1309_128X64_NONAME2_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_SSD1322_NHD_256X64_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1322_NHD_256X64_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1322_NHD_128X64_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1322_NHD_128X64_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1325_NHD_128X64_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1325_NHD_128X64_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	
//U8X8_SSD1326_ER_256X32_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);         // experimental driver for ER-OLED018-1
//U8X8_SSD1327_SEEED_96X96_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);	// Seeedstudio Grove OLED 96x96
//U8X8_SSD1327_SEEED_96X96_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);	// Seeedstudio Grove OLED 96x96
//U8X8_SSD1327_EA_W128128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1327_EA_W128128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1327_MIDAS_128X128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1327_MIDAS_128X128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1329_128X96_NONAME_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1329_128X96_NONAME_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	
//U8X8_SSD1305_128X32_NONAME_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1305_128X32_NONAME_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1305_128X64_ADAFRUIT_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_SSD1305_128X64_ADAFRUIT_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_KS0108_128X64 u8x8(8, 9, 10, 11, 4, 5, 6, 7, /*enable=*/ 18, /*dc=*/ 17, /*cs0=*/ 14, /*cs1=*/ 15, /*cs2=*/ U8X8_PIN_NONE, /* reset=*/  U8X8_PIN_NONE); 	// Set R/W to low!
//U8X8_KS0108_ERM19264 u8x8(8, 9, 10, 11, 4, 5, 6, 7, /*enable=*/ 18, /*dc=*/ 17, /*cs0=*/ 14, /*cs1=*/ 15, /*cs2=*/ 16, /* reset=*/  U8X8_PIN_NONE); 	// Set R/W to low!
//U8X8_UC1701_EA_DOGS102_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_UC1701_EA_DOGS102_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_PCD8544_84X48_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// Nokia 5110 Display
//U8X8_PCD8544_84X48_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);							// Nokia 5110 Display
//U8X8_PCF8812_96X65_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// Could be also PCF8814
//U8X8_PCF8812_96X65_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);			// Could be also PCF8814
//U8X8_HX1230_96X68_3W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* reset=*/ 8);
//U8X8_HX1230_96X68_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_EA_DOGM128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_EA_DOGM128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_64128N_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_64128N_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_EA_DOGM132_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ U8X8_PIN_NONE);	// DOGM132 Shield
//U8X8_ST7565_EA_DOGM132_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ U8X8_PIN_NONE);	// DOGM132 Shield
//U8X8_ST7565_ZOLEN_128X64_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_ZOLEN_128X64_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_LM6059_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// Adafruit ST7565 GLCD
//U8X8_ST7565_LM6059_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		// Adafruit ST7565 GLCD
//U8X8_ST7565_LX12864_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	
//U8X8_ST7565_LX12864_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);		
//U8X8_ST7565_ERC12864_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_ERC12864_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_NHD_C12832_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_NHD_C12832_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_NHD_C12864_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_NHD_C12864_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_JLX12864_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7565_JLX12864_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST7567_PI_132X64_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);  // Pax Instruments Shield, LCD_BL=6
//U8X8_ST7567_PI_132X64_4W_HW_SPI u8x8(/* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);  // Pax Instruments Shield, LCD_BL=6
//U8X8_ST7567_JLX12864_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_ST7567_JLX12864_4W_HW_SPI u8x8(/* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_ST7567_ENH_DG128064_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_ST7567_ENH_DG128064_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_ST7567_ENH_DG128064I_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_ST7567_ENH_DG128064I_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_ST75256_JLX172104_4W_SW_SPI u8x8( /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX172104_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX256128_4W_SW_SPI u8x8( /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX256128_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX256160_4W_SW_SPI u8x8( /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX256160_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX240160_4W_SW_SPI u8x8( /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX240160_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_ST75256_JLX25664_2ND_HW_I2C u8x8(/* reset=*/ 8);	// Due, 2nd I2C
//U8X8_NT7534_TG12864R_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_NT7534_TG12864R_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_ST7588_JLX12864_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ 5);  
//U8X8_ST7588_JLX12864_HW_I2C u8x8(/* reset=*/ 5);
//U8X8_IST3020_ERC19264_6800 u8x8(44, 43, 42, 41, 40, 39, 38, 37,  /*enable=*/ 28, /*cs=*/ 32, /*dc=*/ 30, /*reset=*/ 31); // Connect WR pin with GND
//U8X8_IST3020_ERC19264_8080 u8x8(44, 43, 42, 41, 40, 39, 38, 37,  /*enable=*/ 29, /*cs=*/ 32, /*dc=*/ 30, /*reset=*/ 31); // Connect RD pin with 3.3V
//U8X8_IST3020_ERC19264_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_UC1601_128X32_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_UC1601_128X32_3W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* reset=*/ 8);
//U8X8_UC1604_JLX19264_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_UC1604_JLX19264_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  
//U8X8_UC1608_ERC24064_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  // Due, SW SPI, ERC24064-1 Test Board 
//U8X8_UC1608_ERC240120_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8X8_UC1608_240X128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 
//U8X8_UC1610_EA_DOGXL160_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/  U8X8_PIN_NONE);
//U8X8_UC1610_EA_DOGXL160_4W_HW_SPI u8x8(/* cs=*/ 10, /* dc=*/ 9, /* reset=*/  U8X8_PIN_NONE);
//U8X8_UC1611_EA_DOGM240_2ND_HW_I2C u8x8(/* reset=*/ 8);	// Due, 2nd I2C, DOGM240 Test Board
//U8X8_UC1611_EA_DOGM240_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  // SW SPI, Due DOGXL240 Test Board
//U8X8_UC1611_EA_DOGXL240_2ND_HW_I2C u8x8(/* reset=*/ 8);	// Due, 2nd I2C, DOGXL240 Test Board
//U8X8_UC1611_EA_DOGXL240_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);  // SW SPI, Due DOGXL240 Test Board
//U8X8_UC1611_EW50850_8080 u8x8(8, 9, 10, 11, 4, 5, 6, 7,  /*enable=*/ 18, /*cs=*/ 3, /*dc=*/ 16, /*reset=*/ 16); // 240x160, Connect RD/WR1 pin with 3.3V, CS is aktive high
//U8X8_UC1638_160X128_4W_HW_SPI u8x8(/* cs=*/ 2, /* dc=*/ 3, /* reset=*/ 4); 	 // Not tested
//U8X8_SSD1606_172X72_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// eInk/ePaper Display
//U8X8_SSD1607_200X200_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// eInk/ePaper Display, original LUT from embedded artists
//U8X8_SSD1607_GS_200X200_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// Good Display
//U8X8_IL3820_296X128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// eInk/ePaper Display, original LUT
//U8X8_IL3820_V2_296X128_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);	// eInk/ePaper Display, optimized (lesser screen flicker)


// End of constructor list


// setup the terminal (U8X8LOG) and connect to u8g2 for automatic refresh of the display
// The size (width * height) depends on the display 

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];
U8X8LOG u8x8log;



void setup(void)
{
  /* U8g2 Project: SSD1306 Test Board */
  //pinMode(10, OUTPUT);
  //pinMode(9, OUTPUT);
  //digitalWrite(10, 0);
  //digitalWrite(9, 0);		
  
  /* U8g2 Project: KS0108 Test Board */
  //pinMode(16, OUTPUT);
  //digitalWrite(16, 0);	
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8x8log.setRedrawMode(1);		// 0: Update screen with newline, 1: Update screen for every char  
  
}

unsigned long t = 0;

// print the output of millis() to the terminal every second
void loop(void) {
  if ( t < millis() ) {
    t = millis() + 15000;			// every 15 seconds
    u8x8log.print("\f");			// \f = form feed: clear the screen
  }
  u8x8log.print("millis=");
  u8x8log.print(millis());
  u8x8log.print("\n");
  delay(500);
}

