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

#include "alexa.h"
#include "api.h"
#include "button.h"
#include "crash.h"
#include "curtain_kingart.h"
#include "debug.h"
#include "domoticz.h"
#include "encoder.h"
#include "homeassistant.h"
#include "garland.h"
#include "i2c.h"
#include "influxdb.h"
#include "fan.h"
#include "ir.h"
#include "led.h"
#include "light.h"
#include "lightfox.h"
#include "llmnr.h"
#include "mdns.h"
#include "mqtt.h"
#include "netbios.h"
#include "nofuss.h"
#include "ntp.h"
#include "ota.h"
#include "relay.h"
#include "rfbridge.h"
#include "rfm69.h"
#include "rpc.h"
#include "rpnrules.h"
#include "rtcmem.h"
#include "scheduler.h"
#include "sensor.h"
#include "ssdp.h"
#include "telnet.h"
#include "thermostat.h"
#include "thingspeak.h"
#include "tuya.h"
#include "uartmqtt.h"
#include "web.h"
#include "ws.h"
#include "mcp23s08.h"
#include "prometheus.h"
