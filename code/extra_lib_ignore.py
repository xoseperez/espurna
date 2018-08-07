Import("env")

def check(symbol):
    def func(defines):
        return defines.get(symbol, False)
    return func


def check_my92xx(defines):
    if "LIGHT_PROVIDER" in defines:
        return defines["LIGHT_PROVIDER"] == "LIGHT_PROVIER_MY92XX"
    return False


# func(defines) -> lib:true, lib:false
deps = {
    check("I2C_USE_BRZO"): ["Brzo I2C", None],
    check("IR_SUPPORT"): ["IRremoteESP8266", None],
    check("MQTT_USE_ASYNC"): ["AsyncMqttClient", "PubSubClient"],
    check("ALEXA_SUPPORT"): ["FauxmoESP", None],
    check("HLW8012_SUPPORT"): ["HLW8012", None],
    check("MDNS_CLIENT_SUPPORT"): ["mDNSResolver", None],
    check("NOFUSS_SUPPORT"): ["NoFUSS", None],
    check("NTP_SUPPORT"): ["NtpClientLib", None],
    check("PZEM004T_SUPPORT"): ["PZEM004T", None],
    check("RF_SUPPORT"): ["rc-switch", None],
    check("RFM69_SUPPORT"): ["RFM69", None],
    check("DALLAS_SUPPORT"): ["OneWire", None],
    check_my92xx: ["my92xx", None]
}

lib_ignore = env.get("LIB_IGNORE", [])

for define, libs in deps.items():
    value = define(env.get("ESPURNA_DEFINES", {}))
    lib_true, lib_false = libs

    to_ignore = None
    if value:
        to_ignore = lib_false
    else:
        to_ignore = lib_true

    if to_ignore:
        lib_ignore.append(to_ignore)

env.Replace(LIB_IGNORE=lib_ignore)
