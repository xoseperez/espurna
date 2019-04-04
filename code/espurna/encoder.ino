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

void _encoderConfigure() {

    for (unsigned char index=0; _encoders.size(); ++id) {
        _encoders[index].mode = getSetting("encMode", index, ENCODER_MODE_RATIO).toInt();
        _encoders[index].channel1 = getSetting("encCh", index, 0).toInt();
        _encoders[index].channel2 = getSetting("encBtnCh", index, 1).toInt();
    }

}

void _encoderSetup() {

    // TODO: separate encoder max / max_components from v2?

    unsigned char index = 0;
    while (index < 8) {

        unsigned char pin1 = getSetting("encA", index, GPIO_NONE).toInt();
        unsigned char pin2 = getSetting("encB", index, GPIO_NONE).toInt();
        if ((GPIO_NONE == pin1) || (GPIO_NONE == pin2)) break;

        unsigned char button_pin = getSetting("encBtn", index, GPIO_NONE).toInt();
        unsigned char button_logic = getSetting("encBtnLogic", index, GPIO_LOGIC_INVERSE).toInt();
        unsigned char button_mode = getSetting("encBtnMode", index, INPUT_PULLUP).toInt();
        unsigned char mode = getSetting("encMode", index, ENCODER_MODE_RATIO).toInt();
        unsigned char channel1 = getSetting("encCh", index, 0).toInt();
        unsigned char channel2 = getSetting("encBtnCh", index, 1).toInt();

        _encoders.push_back({
            new Encoder(pin1, pin2),
            button_pin, button_logic, button_mode,
            mode,
            channel1, channel2
        });

        if (GPIO_NONE != button_pin) {
            pinMode(button_pin, button_mode);
        }

        ++index;

    }

    DEBUG_MSG_P(PSTR("[ENCODER] Number of encoders: %u\n"), _encoders.size());
}

// -----------------------------------------------------------------------------

void encoderSetup() {

    // Configure encoders
    _encoderSetup();
    _encoderConfigure();

    // Main callbacks
    espurnaRegisterLoop(_encoderLoop);
    espurnaRegisterReload(_encoderConfigure);

}

#endif // ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
