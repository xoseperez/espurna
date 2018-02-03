

#include <vector>
//#include "inputfilter.h"

typedef struct {
    InputFilter * input;
    unsigned long actions;
    unsigned int relayID;
} generic_input_t;

std::vector<generic_input_t> _inputs;


#if MQTT_SUPPORT
#ifdef MQTT_TOPIC_GENERICINPUT
void genericInputMQTT(unsigned char id, uint8_t event) {
    if (id >= _inputs.size()) return;
    char payload[2];
    snprintf_P(payload, sizeof(payload), PSTR("%d"), event);
    mqttSend(MQTT_TOPIC_GENERICINPUT, id, payload);
}
#endif
#endif


void genericInputEvent(unsigned char id, uint8_t event) {
    DEBUG_MSG_P(PSTR("[INPUT] Input #%d event %d\n"), id, event);
    if (event == 0) return;

    #if MQTT_SUPPORT
    #ifdef MQTT_TOPIC_GENERICINPUT
        switch (event) {
            case GIE_SWITCHED_OFF:
                genericInputMQTT(id, 0); break;
            case GIE_SWITCHED_ON:
                genericInputMQTT(id, 1); break;
        }

    #endif
    #endif

}

void genericInputSetup() {
	#ifdef INPUT1_PIN
    _inputs.push_back({new InputFilter(1,INPUT1_PIN, INPUT1_MODE, INPUT1_FILTER), 0, 0});
    #endif
    #ifdef INPUT2_PIN
    _inputs.push_back({new InputFilter(2,INPUT2_PIN, INPUT2_MODE, INPUT2_FILTER), 0, 0});
    #endif
    #ifdef INPUT3_PIN
    _inputs.push_back({new InputFilter(3,INPUT3_PIN, INPUT3_MODE, INPUT3_FILTER), 0, 0});
    #endif
    #ifdef INPUT4_PIN
    _inputs.push_back({new InputFilter(4,INPUT4_PIN, INPUT4_MODE, INPUT4_FILTER), 0, 0});
    #endif
    #ifdef INPUT5_PIN
    _inputs.push_back({new InputFilter(5,INPUT5_PIN, INPUT5_MODE, INPUT5_FILTER), 0, 0});
    #endif
    #ifdef INPUT6_PIN
    _inputs.push_back({new InputFilter(6,INPUT6_PIN, INPUT6_MODE, INPUT6_FILTER), 0, 0});
    #endif
    #ifdef INPUT7_PIN
    _inputs.push_back({new InputFilter(7,INPUT7_PIN, INPUT7_MODE, INPUT7_FILTER), 0, 0});
    #endif
    #ifdef INPUT8_PIN
    _inputs.push_back({new InputFilter(8,INPUT8_PIN, INPUT8_MODE, INPUT8_FILTER), 0, 0});
    #endif

    // Register loop
    espurnaRegisterLoop(genericInputLoop);

}

void genericInputLoop() {
	for (unsigned int i=0; i < _inputs.size(); i++) {
		if (unsigned char event = _inputs[i].input->loop()) {
            if (event != GIE_NONE)
                genericInputEvent(i, event);
		}
	}
}

uint8_t genericInputState(uint8_t input) {
    uint8_t retVal = GIS_UNUSED;

    for (unsigned int i=0; i < _inputs.size(); i++) {
        if (input == _inputs[i].input->getInputNumber()) {
            retVal = _inputs[i].input->getInputState();
            break;
        }
    }
    return retVal;
}
