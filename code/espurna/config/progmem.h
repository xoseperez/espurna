//--------------------------------------------------------------------------------
// PROGMEM definitions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Reset reasons
//--------------------------------------------------------------------------------

PROGMEM const char custom_reset_hardware[] = "Hardware button";
PROGMEM const char custom_reset_web[] = "Reboot from web interface";
PROGMEM const char custom_reset_terminal[] = "Reboot from terminal";
PROGMEM const char custom_reset_mqtt[] = "Reboot from MQTT";
PROGMEM const char custom_reset_rpc[] = "Reboot from RPC";
PROGMEM const char custom_reset_ota[] = "Reboot after successful OTA update";
PROGMEM const char custom_reset_http[] = "Reboot from HTTP";
PROGMEM const char custom_reset_nofuss[] = "Reboot after successful NoFUSS update";
PROGMEM const char custom_reset_upgrade[] = "Reboot after successful web update";
PROGMEM const char custom_reset_factory[] = "Factory reset";
PROGMEM const char* const custom_reset_string[] = {
    custom_reset_hardware, custom_reset_web, custom_reset_terminal,
    custom_reset_mqtt, custom_reset_rpc, custom_reset_ota,
    custom_reset_http, custom_reset_nofuss, custom_reset_upgrade,
    custom_reset_factory
};

//--------------------------------------------------------------------------------
// Sensors
//--------------------------------------------------------------------------------

#if SENSOR_SUPPORT

PROGMEM const unsigned char magnitude_decimals[] = {
    0,
    1, 0, 2,
    3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 3, 3
};

PROGMEM const char magnitude_unknown_topic[] = "unknown";
PROGMEM const char magnitude_temperature_topic[] =  "temperature";
PROGMEM const char magnitude_humidity_topic[] = "humidity";
PROGMEM const char magnitude_pressure_topic[] = "pressure";
PROGMEM const char magnitude_current_topic[] = "current";
PROGMEM const char magnitude_voltage_topic[] = "voltage";
PROGMEM const char magnitude_active_power_topic[] = "power";
PROGMEM const char magnitude_apparent_power_topic[] = "apparent";
PROGMEM const char magnitude_reactive_power_topic[] = "reactive";
PROGMEM const char magnitude_power_factor_topic[] = "factor";
PROGMEM const char magnitude_energy_topic[] = "energy";
PROGMEM const char magnitude_energy_delta_topic[] = "energy_delta";
PROGMEM const char magnitude_analog_topic[] = "analog";
PROGMEM const char magnitude_digital_topic[] = "digital";
PROGMEM const char magnitude_events_topic[] = "events";
PROGMEM const char magnitude_pm1dot0_topic[] = "pm1dot0";
PROGMEM const char magnitude_pm2dot5_topic[] = "pm2dot5";
PROGMEM const char magnitude_pm10_topic[] = "pm10";
PROGMEM const char magnitude_co2_topic[] = "co2";
PROGMEM const char magnitude_lux_topic[] = "lux";
PROGMEM const char magnitude_uv_topic[] = "uv";
PROGMEM const char magnitude_distance_topic[] = "distance";

PROGMEM const char* const magnitude_topics[] = {
    magnitude_unknown_topic, magnitude_temperature_topic, magnitude_humidity_topic,
    magnitude_pressure_topic, magnitude_current_topic, magnitude_voltage_topic,
    magnitude_active_power_topic, magnitude_apparent_power_topic, magnitude_reactive_power_topic,
    magnitude_power_factor_topic, magnitude_energy_topic, magnitude_energy_delta_topic,
    magnitude_analog_topic, magnitude_digital_topic, magnitude_events_topic,
    magnitude_pm1dot0_topic, magnitude_pm2dot5_topic, magnitude_pm10_topic,
    magnitude_co2_topic, magnitude_lux_topic, magnitude_uv_topic,
    magnitude_distance_topic
};

PROGMEM const char magnitude_empty[] = "";
PROGMEM const char magnitude_celsius[] =  "C";
PROGMEM const char magnitude_fahrenheit[] =  "F";
PROGMEM const char magnitude_percentage[] = "%";
PROGMEM const char magnitude_hectopascals[] = "hPa";
PROGMEM const char magnitude_amperes[] = "A";
PROGMEM const char magnitude_volts[] = "V";
PROGMEM const char magnitude_watts[] = "W";
PROGMEM const char magnitude_kw[] = "kW";
PROGMEM const char magnitude_joules[] = "J";
PROGMEM const char magnitude_kwh[] = "kWh";
PROGMEM const char magnitude_ugm3[] = "µg/m3";
PROGMEM const char magnitude_ppm[] = "ppm";
PROGMEM const char magnitude_lux[] = "lux";
PROGMEM const char magnitude_uv[] = "uv";
PROGMEM const char magnitude_distance[] = "m";

PROGMEM const char* const magnitude_units[] = {
    magnitude_empty, magnitude_celsius, magnitude_percentage,
    magnitude_hectopascals, magnitude_amperes, magnitude_volts,
    magnitude_watts, magnitude_watts, magnitude_watts,
    magnitude_percentage, magnitude_joules, magnitude_joules,
    magnitude_empty, magnitude_empty, magnitude_empty,
    magnitude_ugm3, magnitude_ugm3, magnitude_ugm3,
    magnitude_ppm, magnitude_lux, magnitude_uv,
    magnitude_distance

};

#endif
