/*

RFM69Wrap

RFM69 by Felix Ruso (http://LowPowerLab.com/contact) wrapper for ESP8266
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>

class RFM69Wrap: public RFM69_ATC {

    public:

        RFM69Wrap(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=0):
            RFM69_ATC(slaveSelectPin, interruptPin, isRFM69HW, interruptNum) {};

    protected:

        // overriding SPI_CLOCK for ESP8266
        void select() {

            noInterrupts();

            #if defined (SPCR) && defined (SPSR)
                // save current SPI settings
                _SPCR = SPCR;
                _SPSR = SPSR;
            #endif

            // set RFM69 SPI settings
            SPI.setDataMode(SPI_MODE0);
            SPI.setBitOrder(MSBFIRST);

            #if defined(__arm__)
            	SPI.setClockDivider(SPI_CLOCK_DIV16);
            #elif defined(ARDUINO_ARCH_ESP8266)
                SPI.setClockDivider(SPI_CLOCK_DIV2); // speeding it up for the ESP8266
            #else
                SPI.setClockDivider(SPI_CLOCK_DIV4);
            #endif

            digitalWrite(_slaveSelectPin, LOW);

        }

};
