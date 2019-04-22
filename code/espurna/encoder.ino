/*

ENCODER MODULE

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)

#include <Encoder.h>
#include <vector>

typedef struct {
    Encoder * encoder;
    unsigned char button_pin;
    unsigned char button_logic;
    unsigned char button_mode;
    unsigned char mode;
    unsigned char channel1;             // default
    unsigned char channel2;             // only if button defined and pressed
} encoder_t;

std::vector<encoder_t> _encoders;

void _encoderConfigure() {

    // Clean previous encoders
    for (unsigned char i=0; i<_encoders.size(); i++) {
        free(_encoders[i].encoder);
    }
    _encoders.clear();

    // Load encoders
    #if (ENCODER1_PIN1 != GPIO_NONE) && (ENCODER1_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER1_PIN1, ENCODER1_PIN2),
            ENCODER1_BUTTON_PIN, ENCODER1_BUTTON_LOGIC, ENCODER1_BUTTON_MODE, ENCODER1_MODE,
            ENCODER1_CHANNEL1, ENCODER1_CHANNEL2
        });
    }
    #endif
    #if (ENCODER2_PIN1 != GPIO_NONE) && (ENCODER2_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER2_PIN1, ENCODER2_PIN2),
            ENCODER2_BUTTON_PIN, ENCODER2_BUTTON_LOGIC, ENCODER2_BUTTON_MODE, ENCODER2_MODE,
            ENCODER2_CHANNEL1, ENCODER2_CHANNEL2
        });
    }
    #endif
    #if (ENCODER3_PIN1 != GPIO_NONE) && (ENCODER3_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER3_PIN1, ENCODER3_PIN2),
            ENCODER3_BUTTON_PIN, ENCODER3_BUTTON_LOGIC, ENCODER3_BUTTON_MODE, ENCODER3_MODE,
            ENCODER3_CHANNEL1, ENCODER3_CHANNEL2
        });
    }
    #endif
    #if (ENCODER4_PIN1 != GPIO_NONE) && (ENCODER4_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER4_PIN1, ENCODER4_PIN2),
            ENCODER4_BUTTON_PIN, ENCODER4_BUTTON_LOGIC, ENCODER4_BUTTON_MODE, ENCODER4_MODE,
            ENCODER4_CHANNEL1, ENCODER4_CHANNEL2
        });
    }
    #endif
    #if (ENCODER5_PIN1 != GPIO_NONE) && (ENCODER5_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER5_PIN1, ENCODER5_PIN2),
            ENCODER5_BUTTON_PIN, ENCODER5_BUTTON_LOGIC, ENCODER5_BUTTON_MODE, ENCODER5_MODE,
            ENCODER5_CHANNEL1, ENCODER5_CHANNEL2
        });
    }
    #endif

    // Setup encoders
    for (unsigned char i=0; i<_encoders.size(); i++) {
        if (GPIO_NONE != _encoders[i].button_pin) {
            pinMode(_encoders[i].button_pin, _encoders[i].button_mode);
        }
    }

}

void _encoderLoop() {

    // for each encoder
    for (unsigned char i=0; i<_encoders.size(); i++) {

        // get encoder
        encoder_t encoder = _encoders[i];

        // read encoder
        long delta = encoder.encoder->read();
        encoder.encoder->write(0);
        if (0 == delta) continue;

        DEBUG_MSG_P(PSTR("[ENCODER] Delta: %d\n"), delta);

        // action
        if (encoder.button_pin == GPIO_NONE) {

            // if there is no button, the encoder drives CHANNEL1
            lightChannelStep(encoder.channel1, delta);

        } else {

            // check if button is pressed
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

        lightUpdate(true, true);

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
