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
 *
 * I haven't managed to get the terminalRegisterCommand commands in a method
 * because of the lambda function
 */
void serialSensorSetup() {
#if TERMINAL_SUPPORT
#if MQTT_SUPPORT
    terminalRegisterCommand(F("SENSOR0"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR0", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR0 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR1"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR1", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR1 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR2"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR2", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR2 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR3"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR3", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR3 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR4"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR4", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR4 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR5"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR5", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR5 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR6"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR6", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR6 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("SENSOR7"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("SENSOR7", value.c_str());
            ctx.output.printf_P(PSTR("SENSOR7 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("TEMP0"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("TEMP0", value.c_str());
            ctx.output.printf_P(PSTR("TEMP0 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("TEMP1"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            String value = String(ctx.argv[1]);
            mqttSend("TEMP1", value.c_str());
            ctx.output.printf_P(PSTR("TEMP1 has value %s\n"),
                value.c_str());
        }

        terminalOK(ctx);
    });
#endif
#endif
}

#endif // SERIAL_SENSOR_SUPPORT
