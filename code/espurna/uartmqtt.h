/*

UART_MQTT MODULE

Copyright (C) 2018 by Albert Weterings
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if UART_MQTT_SUPPORT

#include <SoftwareSerial.h>

void uartmqttSetup();

#endif // UART_MQTT_SUPPORT == 1
