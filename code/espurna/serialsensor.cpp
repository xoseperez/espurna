/*

SERIAL_SENSOR MODULE

Copyright (C) 2017-2021 by Patrizio Bekerle <patrizio at bekerle dot com>

*/

#if SERIAL_SENSOR_SUPPORT

#include "serialsensor.h"
#include "mqtt.h"
#include "ws.h"

// -----------------------------------------------------------------------------
// COMMUNICATIONS
// -----------------------------------------------------------------------------

/**
 * Setup function
 */
void serialSensorSetup() {
#if TERMINAL_SUPPORT
#if MQTT_SUPPORT
    terminalRegisterCommand(F("MQTT.SEND"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 3) {
            if (mqttSend(ctx.argv[1].c_str(), ctx.argv[2].c_str(), false, false)) {
                terminalOK(ctx);
            } else {
                terminalError(ctx, F("Cannot queue the message")); //
            }
            return;
        }

        terminalError(ctx, F("MQTT.SEND <topic> <payload>"));
    });
#endif
#endif
}

#endif // SERIAL_SENSOR_SUPPORT
