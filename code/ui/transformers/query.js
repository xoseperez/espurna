const query = {
    directives: {debug: false}, params: {
        ENV: "development",
        VERSION: "2.0.1",
        HA: true,
        DCZ: true,
        THINGSPEAK: true,
        NOFUSS: true,
        IDB: true,
        LIGHTFOX: true,
        ALEXA: true,
        MQTT: true,
        RELAYS: true,
        SENSOR: true,
        LED: true,
        BUTTON: true,
        LIGHT: true,
        THERMOSTAT: true,
        RFBRIDGE: true,
        RFM69: true,
        PWA: false
    }, verbose: false
};

module.exports = query;
