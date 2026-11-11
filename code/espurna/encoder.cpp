/*

ENCODER MODULE

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)

#include "encoder.h"
#include "light.h"

#include "libs/Encoder.h"

#include <vector>

struct encoder_t {
    Encoder * encoder;
    unsigned char button_pin;
    unsigned char button_logic;
    unsigned char button_mode;
    unsigned char mode;
    unsigned char channel1;             // default
    unsigned char channel2;             // only if button defined and pressed
};

std::vector<encoder_t> _encoders;
unsigned long _encoder_min_delta = 1;

struct EncoderDefault {
    uint8_t pin1;
    uint8_t pin2;
    uint8_t button_pin;
    uint8_t button_logic;
    uint8_t button_mode;
    uint8_t mode;
    uint8_t channel1;
    uint8_t channel2;
};

#define ENCODER_DEFAULT(N) EncoderDefault{\
    ENCODER##N##_PIN1, ENCODER##N##_PIN2,\
    ENCODER##N##_BUTTON_PIN, ENCODER##N##_BUTTON_LOGIC, ENCODER##N##_BUTTON_MODE,\
    ENCODER##N##_MODE, ENCODER##N##_CHANNEL1, ENCODER##N##_CHANNEL2}

constexpr EncoderDefault EncoderDefaults[] {
    ENCODER_DEFAULT(1),
    ENCODER_DEFAULT(2),
    ENCODER_DEFAULT(3),
    ENCODER_DEFAULT(4),
    ENCODER_DEFAULT(5),
};

#undef ENCODER_DEFAULT

constexpr size_t EncodersMax { sizeof(EncoderDefaults) / sizeof(EncoderDefaults[0]) };

void _encoderConfigure() {

    _encoder_min_delta = getSetting("encMinDelta", ENCODER_MINIMUM_DELTA);
    if (!_encoder_min_delta) _encoder_min_delta = 1;

    // no need to reload objects right now
    if (_encoders.size()) return;

    // Clean previous encoders and re-add them
    for (auto& encoder : _encoders) {
        delete encoder.encoder;
    }
    _encoders.clear();

    // Pins and modes come from the settings, build values are the defaults (generic images: none, idle)
    // encAGPIO<N> / encBGPIO<N>   encoder pins
    // encBtnGPIO<N>               optional button, encBtnLogic<N> (level when NOT pressed), encBtnMode<N> (pinMode value)
    // encMode<N>                  0 = channel (button selects encChA / encChB), 1 = ratio (button changes the
    //                             encChA / encChB ratio, otherwise the brightness)
    // encChA<N> / encChB<N>       light channels, 0-based
    for (size_t index = 0; index < EncodersMax; ++index) {
        const auto& defaults = EncoderDefaults[index];
        const uint8_t pin1 = getSetting({"encAGPIO", index}, defaults.pin1);
        const uint8_t pin2 = getSetting({"encBGPIO", index}, defaults.pin2);
        if (!gpioValid(pin1) || !gpioValid(pin2)) {
            break;
        }

        const uint8_t button = getSetting({"encBtnGPIO", index}, defaults.button_pin);
        _encoders.push_back({
            new Encoder(pin1, pin2),
            gpioValid(button) ? button : static_cast<uint8_t>(GPIO_NONE),
            getSetting({"encBtnLogic", index}, defaults.button_logic),
            getSetting({"encBtnMode", index}, defaults.button_mode),
            getSetting({"encMode", index}, defaults.mode),
            getSetting({"encChA", index}, defaults.channel1),
            getSetting({"encChB", index}, defaults.channel2),
        });
    }

    // TODO: manage buttons through debounceevent?
    for (auto& encoder : _encoders) {
        if (GPIO_NONE != encoder.button_pin) {
            pinMode(encoder.button_pin, encoder.button_mode);
        }
    }

}

void _encoderLoop() {

    // for each encoder, read delta (read()) and map button action
    for (auto& encoder : _encoders) {

        const auto delta = encoder.encoder->read();
        encoder.encoder->write(0);
        if ((0 == delta) || (_encoder_min_delta > abs(delta))) continue;

        if (encoder.button_pin == GPIO_NONE) {

            // if there is no button, the encoder drives CHANNEL1
            lightChannelStep(encoder.channel1, delta);

        } else {

            // otherwise, use button based on encoder mode
            bool pressed = (digitalRead(encoder.button_pin) != encoder.button_logic);

            if (ENCODER_MODE_CHANNEL == encoder.mode) {

                // the button controls what channel we are changing
                lightChannelStep(pressed ? encoder.channel2 : encoder.channel1, delta);

            } if (ENCODER_MODE_RATIO == encoder.mode) {

                // the button controls if we are changing the channel ratio or the overall brightness
                if (pressed) {
                    lightChannelStep(encoder.channel1, delta);
                    lightChannelStep(encoder.channel2, -delta);
                } else {
                    lightBrightnessStep(delta);
                }

            }

        }

        lightUpdate();

    }

}

// -----------------------------------------------------------------------------

void encoderSetup() {

    // Configure encoders
    _encoderConfigure();

    // Main callbacks
    espurnaRegisterLoop(_encoderLoop);
    espurnaRegisterReload(_encoderConfigure);

    DEBUG_MSG_P(PSTR("[ENCODER] Number of encoders: %u\n"), _encoders.size());

}

#endif // ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)

