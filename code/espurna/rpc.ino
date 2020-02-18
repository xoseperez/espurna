/*

Part of MQTT and API modules

*/

#if MQTT_SUPPORT || API_SUPPORT

#include <Schedule.h>

#include "system.h"
#include "utils.h"
#include "rpc.h"

bool rpcHandleAction(const String& action) {
    bool result = false;
    if (action.equals("reboot")) {
        result = true;
        schedule_function([]() {
            deferredReset(100, CUSTOM_RESET_RPC);
        });
    } else if (action.equals("heartbeat")) {
        result = true;
        schedule_function(heartbeat);
    }
    return result;
}

#endif // MQTT_SUPPORT || API_SUPPORT
