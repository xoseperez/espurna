/* ---------------------------- Original copyright -----------------------------
 *
 * Encoder Library, for measuring quadrature encoded signals
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 * Copyright (c) 2011,2013 PJRC.COM, LLC - Paul Stoffregen <paul@pjrc.com>
 *
 * Version 1.2 - fix -2 bug in C-only code
 * Version 1.1 - expand to support boards with up to 60 interrupts
 * Version 1.0 - initial release
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * -----------------------------------------------------------------------------
 *
 * Encoder.h, updated for ESP8266 use in ESPurna. Other hardware is not supported.
 *
 * - Added ESP-specific attributes to ISR handlers to place them in IRAM.
 * - Reduced per-encoder structure sizes - only 5 Encoders can be used on ESP8266,
 *   and we can directly reference pin number instead of storing both register and bitmask
 *
 */

#pragma once

//                           _______         _______       
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2

// new new old old
// pin2 pin1 pin2 pin1 Result
// ---- ---- ---- ---- ------
// 0    0    0    0    no movement
// 0    0    0    1    +1
// 0    0    1    0    -1
// 0    0    1    1    +2  (assume pin1 edges only)
// 0    1    0    0    -1
// 0    1    0    1    no movement
// 0    1    1    0    -2  (assume pin1 edges only)
// 0    1    1    1    +1
// 1    0    0    0    +1
// 1    0    0    1    -2  (assume pin1 edges only)
// 1    0    1    0    no movement
// 1    0    1    1    -1
// 1    1    0    0    +2  (assume pin1 edges only)
// 1    1    0    1    -1
// 1    1    1    0    +1
// 1    1    1    1    no movement

namespace EncoderLibrary {

    typedef struct {
        uint8_t             pin1;
        uint8_t             pin2;
        uint8_t             state;
        int32_t             position;
    } encoder_values_t;

    constexpr const unsigned char ENCODERS_MAXIMUM {5u};

    encoder_values_t * EncoderValues[ENCODERS_MAXIMUM] = {nullptr};

    uint8_t _encoderFindStorage() {
        for (uint8_t i = 0; i < ENCODERS_MAXIMUM; i++) {
            if (EncoderValues[i] == nullptr) {
                return i;
            }
        }
        return ENCODERS_MAXIMUM;
    }

    void _encoderCleanStorage(uint8_t pin1, uint8_t pin2) {
        for (uint8_t i = 0; i < ENCODERS_MAXIMUM; i++) {
            if (EncoderValues[i] == nullptr) continue;
            if (((EncoderValues[i])->pin1 == pin1) && ((EncoderValues[i])->pin2 == pin2)) {
                EncoderValues[i] = nullptr;
                break;
            }
        }
    }

    // update() is not meant to be called from outside Encoder,
    // but it is public to allow static interrupt routines.
    void ICACHE_RAM_ATTR update(encoder_values_t *target) {
        uint8_t p1val = GPIP(target->pin1);
        uint8_t p2val = GPIP(target->pin2);
        uint8_t state = target->state & 3;
        if (p1val) state |= 4;
        if (p2val) state |= 8;
        target->state = (state >> 2);
        switch (state) {
            case 1: case 7: case 8: case 14:
                target->position++;
                return;
            case 2: case 4: case 11: case 13:
                target->position--;
                return;
            case 3: case 12:
                target->position += 2;
                return;
            case 6: case 9:
                target->position -= 2;
                return;
        }
    }

    // 2 pins per encoder, 1 isr per encoder
    void ICACHE_RAM_ATTR isr0() { update(EncoderValues[0]); }
    void ICACHE_RAM_ATTR isr1() { update(EncoderValues[1]); }
    void ICACHE_RAM_ATTR isr2() { update(EncoderValues[2]); }
    void ICACHE_RAM_ATTR isr3() { update(EncoderValues[3]); }
    void ICACHE_RAM_ATTR isr4() { update(EncoderValues[4]); }

    constexpr void (*_isr_funcs[5])() = {
        isr0, isr1, isr2, isr3, isr4
    };

    class Encoder {

    private:

        encoder_values_t values;

    public:

        Encoder(uint8_t pin1, uint8_t pin2) {

            values.pin1 = pin1;
            values.pin2 = pin2;

            pinMode(values.pin1, INPUT_PULLUP);
            pinMode(values.pin2, INPUT_PULLUP);

            values.position = 0;

            // allow time for a passive R-C filter to charge
            // through the pullup resistors, before reading
            // the initial state
            delayMicroseconds(2000);

            uint8_t current = 0;
            if (GPIP(values.pin1)) {
                current |= 1;
            }

            if (GPIP(values.pin2)) {
                current |= 2;
            }

            values.state = current;

            attach();

        }

        ~Encoder() {
            detach();
        }

        uint8_t pin1() {
            return values.pin1;
        }

        uint8_t pin2() {
            return values.pin2;
        }

        int32_t read() {
            noInterrupts();

            update(&values);
            int32_t ret = values.position;

            interrupts();
            return ret;
        }

        void write(int32_t position) {
            noInterrupts();
            values.position = position;
            interrupts();
        }

        bool attach() {
            uint8_t index = _encoderFindStorage();
            if (index >= ENCODERS_MAXIMUM) return false;

            EncoderValues[index] = &values;

            attachInterrupt(values.pin1, _isr_funcs[index], CHANGE);
            attachInterrupt(values.pin2, _isr_funcs[index], CHANGE);

            return true;
        }

        void detach() {
            noInterrupts();

            _encoderCleanStorage(values.pin1, values.pin2);

            detachInterrupt(values.pin1);
            detachInterrupt(values.pin2);

            interrupts();
        }


    };

}

using EncoderLibrary::Encoder;
