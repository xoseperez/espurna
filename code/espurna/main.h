/*

ESPurna

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

#include "espurna.h"

#if ALEXA_SUPPORT
#include "alexa.h"
#endif

#if API_SUPPORT
#include "api.h"
#endif

#if BUTTON_SUPPORT
#include "button.h"
#endif

#if DEBUG_SUPPORT
#include "crash.h"
#endif

#if KINGART_CURTAIN_SUPPORT
#include "curtain_kingart.h"
#endif

#if DEBUG_SUPPORT
#include "debug.h"
#endif

#if DOMOTICZ_SUPPORT
#include "domoticz.h"
#endif

#if ENCODER_SUPPORT
#include "encoder.h"
#endif

#if HOMEASSISTANT_SUPPORT
#include "homeassistant.h"
#endif

#if GARLAND_SUPPORT
#include "garland.h"
#endif

#if I2C_SUPPORT
#include "i2c.h"
#endif

#if INFLUXDB_SUPPORT
#include "influxdb.h"
#endif

#if FAN_SUPPORT
#include "fan.h"
#endif

#if IR_SUPPORT
#include "ir.h"
#endif

#if LED_SUPPORT
#include "led.h"
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
#include "light.h"
#endif

#ifdef FOXEL_LIGHTFOX_DUAL
#include "lightfox.h"
#endif

#if LLMNR_SUPPORT
#include "llmnr.h"
#endif

#if MDNS_SERVER_SUPPORT
#include "mdns.h"
#endif

#if MQTT_SUPPORT
#include "mqtt.h"
#endif

#if NETBIOS_SUPPORT
#include "netbios.h"
#endif

#if NOFUSS_SUPPORT
#include "nofuss.h"
#endif

#if NTP_SUPPORT
#include "ntp.h"
#endif

#if RELAY_SUPPORT
#include "relay.h"
#endif

#if RFB_SUPPORT
#include "rfbridge.h"
#endif

#if RFM69_SUPPORT
#include "rfm69.h"
#endif

#if RPN_RULES_SUPPORT
#include "rpnrules.h"
#endif

#if SCHEDULER_SUPPORT
#include "scheduler.h"
#endif

#if SENSOR_SUPPORT
#include "sensor.h"
#endif

#if SSDP_SUPPORT
#include "ssdp.h"
#endif

#if TELNET_SUPPORT
#include "telnet.h"
#endif

#if THERMOSTAT_SUPPORT
#include "thermostat.h"
#endif

#if THINGSPEAK_SUPPORT
#include "thingspeak.h"
#endif

#if TUYA_SUPPORT
#include "tuya.h"
#endif

#if UART_MQTT_SUPPORT
#include "uartmqtt.h"
#endif

#if WEB_SUPPORT
#include "web.h"
#include "ws.h"
#endif

#if MCP23S08_SUPPORT
#include "mcp23s08.h"
#endif

#if PROMETHEUS_SUPPORT
#include "prometheus.h"
#endif

#if PWM_SUPPORT
#include "pwm.h"
#endif
