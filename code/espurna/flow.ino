/*

FLOW MODULE

Copyright (C) 2016-2018 by Xose P�rez <xose dot perez at gmail dot com>

*/

#if FLOW_SUPPORT

#include <vector>
#include <map>
#include <set>

// -----------------------------------------------------------------------------
// FLOW
// -----------------------------------------------------------------------------

FlowComponentLibrary _library;

typedef struct {
    String component;
    String property;
    std::vector<String>* values;
} component_property_values_t;

std::vector<component_property_values_t> _component_property_values_list;

void flowRegisterComponent(String name, const FlowConnections* connections, const char* json, flow_component_factory_f factory) {
    _library.addType(name, connections, json, factory);
}

void flowRegisterComponentValues(String component, String property, std::vector<String>* values) {
    _component_property_values_list.push_back({component, property, values});
}

void flowGetComponentValuesJson(JsonArray& root) {
    for (component_property_values_t entry : _component_property_values_list) {
        JsonObject& object = root.createNestedObject();
        object["component"] = entry.component;
        object["property"] = entry.property;
        JsonArray& valuesArray = object.createNestedArray("values");
        for (unsigned int j=0; j < entry.values->size(); j++) {
            valuesArray.add(entry.values->at(j));
        }
    }
}

String flowGetDataPath() {
    return "/flow.json"; // TODO: file name to constant
}

bool flowSaveData(char* data) {
    bool result = false;
    File file = SPIFFS.open("/flow.json", "w"); // TODO: file name to constant
    if (file) {
        result = file.print(data);
        file.close();
    } else {
        DEBUG_MSG_P(PSTR("[FLOW] Error saving data\n"));
    }
    return result;
}

const char* flowGetComponentJson(int index) {
    return _library.getComponentJson(index);
}

void flowStart() {
    DEBUG_MSG_P(PSTR("[FLOW] Starting\n"));

    File file = SPIFFS.open("/flow.json", "r"); // TODO: file name to constant
    if (file) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(file);
        if (root.success()) _flowStart(root);
        else DEBUG_MSG_P(PSTR("[FLOW] Error: flow cannot be parsed as correct JSON\n"));

        file.close();
    } else {
        DEBUG_MSG_P(PSTR("[FLOW] No flow found\n"));
    }
}

void _flowStart(JsonObject& data) {
    std::map<String, FlowComponent*> components;
    std::map<String, String> componentsNames;

    JsonObject& processes = data.containsKey("P") ? data["P"] : data["processes"];
    for (auto process_kv: processes) {
        String id = process_kv.key;
        JsonObject& value = process_kv.value;

        String componentName = value.containsKey("C") ? value["C"] : value["component"];
        JsonObject& metadata = value.containsKey("M") ? value["M"] : value["metadata"];
        JsonObject& properties = metadata.containsKey("R") ? metadata["R"] : metadata["properties"];

        FlowComponent* component = _library.createComponent(componentName, properties);

        if (component != NULL) {
            components[id] = component;
            componentsNames[id] = componentName;
        } else {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' is not registered\n"), componentName.c_str());
        }
    }

    JsonArray& connections = data.containsKey("X") ? data["X"] : data["connections"];
    for (JsonObject& connection: connections) {
        JsonObject& src = connection.containsKey("S") ? connection["S"] : connection["src"];
        String srcProcess = src.containsKey("I") ? src["I"] : src["process"];
        String srcPort = src.containsKey("N") ? src["N"] : src["port"];

        JsonObject& tgt = connection.containsKey("T") ? connection["T"] : connection["tgt"];
        String tgtProcess = tgt.containsKey("I") ? tgt["I"] : tgt["process"];
        String tgtPort = tgt.containsKey("N") ? tgt["N"] : tgt["port"];

        FlowComponent* srcComponent = components[srcProcess];
        if (srcComponent == NULL) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component ID='%s' is not registered\n"), srcProcess.c_str());
            continue;
        }

        FlowComponent* tgtComponent = components[tgtProcess];
        if (tgtComponent == NULL) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component ID='%s' is not registered\n"), tgtProcess.c_str());
            continue;
        }

        int srcNumber = _library.getOutputNumber(componentsNames[srcProcess], srcPort);
        if (srcNumber < 0) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no output named '%s'\n"), componentsNames[srcProcess].c_str(), srcPort.c_str());
            continue;
        }

        int tgtNumber = _library.getInputNumber(componentsNames[tgtProcess], tgtPort);
        if (tgtNumber < 0) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no input named '%s'\n"), componentsNames[tgtProcess].c_str(), tgtPort.c_str());
            continue;
        }

        srcComponent->addOutput(srcNumber, tgtComponent, tgtNumber);
    }
}

typedef std::function<void(unsigned long time, JsonVariant *data)> flow_scheduled_task_callback_f;

struct flow_scheduled_task_t {
    JsonVariant *data;
    unsigned long time;
    flow_scheduled_task_callback_f callback;

    bool operator() (const flow_scheduled_task_t& lhs, const flow_scheduled_task_t& rhs) const {
        return lhs.time < rhs.time;
    }
};

std::set<flow_scheduled_task_t, flow_scheduled_task_t> _flow_scheduled_tasks_queue;

void _flowComponentLoop() {
    if (!_flow_scheduled_tasks_queue.empty()) {
        auto it = _flow_scheduled_tasks_queue.begin();
        const flow_scheduled_task_t element = *it;
        unsigned long now = millis();
        if (element.time <= now) {
            element.callback(now, element.data);
            _flow_scheduled_tasks_queue.erase(it);
        }
    }
}

// -----------------------------------------------------------------------------
// Start component
// -----------------------------------------------------------------------------

PROGMEM const char flow_data[] = "Data";
PROGMEM const char* const flow_data_array[] = {flow_data};

PROGMEM const FlowConnections flow_start_component = {
    0, NULL,
    1, flow_data_array,
};

PROGMEM const char flow_start_component_json[] =
    "\"Start\": "
    "{"
        "\"name\":\"Start\","
        "\"icon\":\"play\","
        "\"inports\":[],"
        "\"outports\":[{\"name\":\"Data\",\"type\":\"bool\"}],"
        "\"properties\":[]"
    "}";

class FlowStartComponent : public FlowComponent {
    private:
        JsonVariant *_data = new JsonVariant(true);

    public:
        FlowStartComponent(JsonObject& properties) {
            flow_scheduled_task_callback_f callback = [this](unsigned long time, JsonVariant *data){ this->onStart(); };
            _flow_scheduled_tasks_queue.insert({NULL, 0, callback});
        }

        void onStart() {
            processOutput(*_data, 0);
        }
};

// -----------------------------------------------------------------------------
// Debug component
// -----------------------------------------------------------------------------

PROGMEM const FlowConnections flow_debug_component = {
    1, flow_data_array,
    0, NULL,
};

PROGMEM const char flow_debug_component_json[] =
    "\"Debug\": "
    "{"
        "\"name\":\"Debug\","
        "\"icon\":\"bug\","
        "\"inports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"outports\":[],"
        "\"properties\":[{\"name\":\"Prefix\",\"type\":\"string\"}]"
    "}";

PROGMEM const char flow_debug_int[] = "[FLOW DEBUG] %s%i\n";
PROGMEM const char flow_debug_string[] = "[FLOW DEBUG] %s%s\n";
PROGMEM const char flow_debug_unknown[] = "[FLOW DEBUG] %sUNKNOWN\n";

class FlowDebugComponent : public FlowComponent {
    private:
        String _prefix;

    public:
        FlowDebugComponent(JsonObject& properties) {
            const char * prefix = properties["Prefix"];
            _prefix = String(prefix != NULL ? prefix : "");
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (data.is<int>()) {
                DEBUG_MSG_P(flow_debug_int, _prefix.c_str(), data.as<int>());
            } else if (data.is<double>()) {
                char buffer[64];
                dtostrf(data.as<double>(), 1 - sizeof(buffer), 3, buffer);
                DEBUG_MSG_P(flow_debug_string, _prefix.c_str(), buffer);
            } else if (data.is<bool>()) {
                DEBUG_MSG_P(flow_debug_string, _prefix.c_str(), data.as<bool>() ? "true" : "false");
            } else if (data.is<char*>()) {
                DEBUG_MSG_P(flow_debug_string, _prefix.c_str(), data.as<const char*>());
            } else {
                DEBUG_MSG_P(flow_debug_unknown, _prefix.c_str());
            }
        }
};

// -----------------------------------------------------------------------------
// Change component
// -----------------------------------------------------------------------------

PROGMEM const FlowConnections flow_change_component = {
    1, flow_data_array,
    1, flow_data_array,
};

PROGMEM const char flow_change_component_json[] =
    "\"Change\": "
    "{"
        "\"name\":\"Change\","
        "\"icon\":\"edit\","
        "\"inports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"outports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"properties\":[{\"name\":\"Value\",\"type\":\"any\"}]"
    "}";


class FlowChangeComponent : public FlowComponent {
    private:
        JsonVariant* _value;

    public:
        FlowChangeComponent(JsonObject& properties) {
            JsonVariant value = properties["Value"];
            _value = clone(value);
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            processOutput(*_value, 0);
        }
};

// -----------------------------------------------------------------------------
// Delay component
// -----------------------------------------------------------------------------

PROGMEM const FlowConnections flow_delay_component = {
    1, flow_data_array,
    1, flow_data_array,
};

PROGMEM const char flow_delay_component_json[] =
    "\"Delay\": "
    "{"
        "\"name\":\"Delay\","
        "\"icon\":\"pause\","
        "\"inports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"outports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"properties\":[{\"name\":\"Seconds\",\"type\":\"int\"}, {\"name\":\"Last only\",\"type\":\"bool\"}]"
    "}";

class FlowDelayComponent : public FlowComponent {
    private:
        long _time;
        bool _lastOnly;
        int _queueSize = 0;

    public:
        FlowDelayComponent(JsonObject& properties) {
            _time = 1000 * (int)properties["Seconds"];
            _lastOnly = properties["Last only"];
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            flow_scheduled_task_callback_f callback = [this](unsigned long time, JsonVariant *data){ this->onDelay(data); };
            _flow_scheduled_tasks_queue.insert({clone(data), millis() + _time, callback});
            _queueSize++;
        }

        void onDelay(JsonVariant *data) {
            if (!_lastOnly || _queueSize == 1)
                processOutput(*data, 0);

            _queueSize--;
            release(data);
        }
};

// -----------------------------------------------------------------------------
// Timer component
// -----------------------------------------------------------------------------

PROGMEM const FlowConnections flow_timer_component = {
    0, NULL,
    1, flow_data_array,
};

PROGMEM const char flow_timer_component_json[] =
    "\"Timer\": "
    "{"
        "\"name\":\"Timer\","
        "\"icon\":\"clock-o\","
        "\"inports\":[],"
        "\"outports\":[{\"name\":\"Data\",\"type\":\"bool\"}],"
        "\"properties\":[{\"name\":\"Seconds\",\"type\":\"int\"}]"
    "}";

PROGMEM const char flow_incorrect_timer_delay[] = "[FLOW] Incorrect timer delay: %i\n";

class FlowTimerComponent : public FlowComponent {
    private:
        JsonVariant *_data = new JsonVariant(true);
        unsigned long _period;
        flow_scheduled_task_callback_f _callback;

    public:
        FlowTimerComponent(JsonObject& properties) {
            int seconds = properties["Seconds"];
            _period = 1000 * (int)seconds;

            if (_period > 0) {
                _callback = [this](unsigned long time, JsonVariant *data){ this->onSchedule(time); };
                _flow_scheduled_tasks_queue.insert({NULL, millis() + _period, _callback});
            } else {
                DEBUG_MSG_P(flow_incorrect_timer_delay, seconds);
            }
        }

        void onSchedule(unsigned long now) {
            processOutput(*_data, 0);

            // reschedule
            _flow_scheduled_tasks_queue.insert({NULL, now + _period, _callback});
        }
};

// -----------------------------------------------------------------------------
// Gate component
// -----------------------------------------------------------------------------

PROGMEM const char flow_state[] = "State";
PROGMEM const char flow_open[] = "Open";
PROGMEM const char flow_close[] = "Close";
PROGMEM const char* const flow_gate_component_inputs[] = {flow_data, flow_state};
PROGMEM const char* const flow_gate_component_outputs[] = {flow_open, flow_close};
PROGMEM const FlowConnections flow_gate_component = {
    2, flow_gate_component_inputs,
    2, flow_gate_component_outputs,
};

PROGMEM const char flow_gate_component_json[] =
    "\"Gate\": "
    "{"
        "\"name\":\"Gate\","
        "\"icon\":\"unlock\","
        "\"inports\":[{\"name\":\"Data\",\"type\":\"any\"}, {\"name\":\"State\",\"type\":\"bool\"}],"
        "\"outports\":[{\"name\":\"Open\",\"type\":\"any\"}, {\"name\":\"Close\",\"type\":\"any\"}],"
        "\"properties\":[]"
    "}";

class FlowGateComponent : public FlowComponent {
    private:
        bool _state = true;

    public:
        FlowGateComponent(JsonObject& properties) {
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (inputNumber == 0) { // data
                processOutput(data, _state ? 0 : 1);
            } else { // state
                _state = data.as<bool>();
            }
        }
};

// -----------------------------------------------------------------------------
// Hysteresis component
// -----------------------------------------------------------------------------

PROGMEM const char flow_value[] = "Value";
PROGMEM const char flow_min[] = "Min";
PROGMEM const char flow_max[] = "Max";
PROGMEM const char flow_rise[] = "Rise";
PROGMEM const char flow_fall[] = "Fall";
PROGMEM const char* const flow_hysteresis_component_inputs[] = {flow_value, flow_min, flow_max};
PROGMEM const char* const flow_hysteresis_component_outputs[] = {flow_rise, flow_fall};
PROGMEM const FlowConnections flow_hysteresis_component = {
    3, flow_hysteresis_component_inputs,
    2, flow_hysteresis_component_outputs,
};

PROGMEM const char flow_hysteresis_component_json[] =
    "\"Hysteresis\": "
    "{"
        "\"name\":\"Hysteresis\","
        "\"icon\":\"line-chart\","
        "\"inports\":[{\"name\":\"Value\",\"type\":\"double\"}, {\"name\":\"Min\",\"type\":\"double\"}, {\"name\":\"Max\",\"type\":\"double\"}],"
        "\"outports\":[{\"name\":\"Rise\",\"type\":\"double\"}, {\"name\":\"Fall\",\"type\":\"double\"}],"
        "\"properties\":[{\"name\":\"Min\",\"type\":\"double\"}, {\"name\":\"Max\",\"type\":\"double\"}]"
    "}";

class FlowHysteresisComponent : public FlowComponent {
    private:
        bool _state = false;
        double _min = NAN;
        double _max = NAN;
        double _value = NAN;

    public:
        FlowHysteresisComponent(JsonObject& properties) {
            JsonVariant min = properties["Min"];
            JsonVariant max = properties["Max"];
            _min = min.success() && min.is<double>() ? min.as<double>() : NAN;
            _max = max.success() && max.is<double>() ? max.as<double>() : NAN;
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (inputNumber == 0) { // value
                _value = data.as<double>();
                if ((_state && _value >= _max) || (!_state && _value <= _min))
                    _state = !_state;
                processOutput(data, _state ? 0 : 1);
            } else if (inputNumber == 1) { // min
                _min = data.as<double>();
                if (!_state && _value <= _min) {
                    _state = true;
                    JsonVariant value(_value);
                    processOutput(value, 0);
                }
            } else { // max
                _max = data.as<double>();
                if (_state && _value >= _max) {
                    _state = false;
                    JsonVariant value(_value);
                    processOutput(value, 1);
                }
            }
        }
};

//void _flowMQTTCallback(unsigned int type, const char * topic, const char * payload) {
//
//    if (type == MQTT_CONNECT_EVENT) {
//        mqttSubscribe("flow");
//    }
//
//    if (type == MQTT_MESSAGE_EVENT) {
//
//        // Match topic
//        String t = mqttMagnitude((char *) topic);
//        if (t.equals("flow")) {
//            flowStart();
//        }
//
//    }
//}

void flowSetup() {
//    mqttRegister(_flowMQTTCallback);

    flowRegisterComponent("Start", &flow_start_component, flow_start_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowStartComponent(properties); }));

    flowRegisterComponent("Debug", &flow_debug_component, flow_debug_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDebugComponent(properties); }));

    flowRegisterComponent("Change", &flow_change_component, flow_change_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowChangeComponent(properties); }));

    flowRegisterComponent("Delay", &flow_delay_component, flow_delay_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDelayComponent(properties); }));

    flowRegisterComponent("Timer", &flow_timer_component, flow_timer_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowTimerComponent(properties); }));

    flowRegisterComponent("Gate", &flow_gate_component, flow_gate_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowGateComponent(properties); }));

    flowRegisterComponent("Hysteresis", &flow_hysteresis_component, flow_hysteresis_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowHysteresisComponent(properties); }));

    espurnaRegisterLoop(_flowComponentLoop);
}

#endif // FLOW_SUPPORT
