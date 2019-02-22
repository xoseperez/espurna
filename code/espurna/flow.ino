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

#if !SPIFFS_SUPPORT
String _flow;
unsigned long _mqtt_flow_sent_at = 0;
#endif

bool _flow_started = false;
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

AsyncWebServerResponse* flowGetConfigResponse(AsyncWebServerRequest *request) {
    #if SPIFFS_SUPPORT
        return request->beginResponse(SPIFFS, FLOW_SPIFFS_FILE, "text/json");
    #else
        return request->beginResponse(200, "text/json", _flow);
    #endif
}

bool flowSaveConfig(char* data) {
    bool result = false;

    #if SPIFFS_SUPPORT
        File file = SPIFFS.open(FLOW_SPIFFS_FILE, "w");
        if (file) {
            result = file.print(data);
            file.close();
        } else {
            DEBUG_MSG_P(PSTR("[FLOW] Error saving flow to file\n"));
        }
    #elif MQTT_SUPPORT
        result = mqttConnected();
        _flow = String(data);
        if (result) {
            _mqtt_flow_sent_at = millis();
            mqttSendRaw(mqttTopic("flow", true).c_str(), data, true);
        }
        else {
            DEBUG_MSG_P(PSTR("[FLOW] Error publishing flow because MQTT is disconnected\n"));
        }
    #else
        _flow = String(data);
        DEBUG_MSG_P(PSTR("[FLOW] Error saving flow\n"));
    #endif

    return result;
}

const char* flowGetComponentJson(int index) {
    return _library.getComponentJson(index);
}

void flowStart() {
    if (_flow_started) {
        DEBUG_MSG_P(PSTR("[FLOW] Started already\n"));
        return;
    }

    DEBUG_MSG_P(PSTR("[FLOW] Starting\n"));

    #if SPIFFS_SUPPORT
        File source = SPIFFS.open(FLOW_SPIFFS_FILE, "r");
        if (!source) {
            DEBUG_MSG_P(PSTR("[FLOW] No flow file found\n"));
            return;
        }
    #else
        String& source = _flow;
    #endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(source);
    if (root.success()) _flowStart(root);
    else DEBUG_MSG_P(PSTR("[FLOW] Error: flow cannot be parsed as correct JSON\n"));

    #if SPIFFS_SUPPORT
        source.close();
    #endif

    _flow_started = true;
}

void _flowAddConnection(std::map<String, FlowComponent*>& components, std::map<String, String>& componentsNames,
                        String& srcProcess, String& srcPort, String& tgtProcess, String& tgtPort) {
    FlowComponent* srcComponent = components[srcProcess];
    if (srcComponent == NULL) {
        DEBUG_MSG_P(PSTR("[FLOW] Error: component ID='%s' is not registered\n"), srcProcess.c_str());
        return;
    }

    FlowComponent* tgtComponent = components[tgtProcess];
    if (tgtComponent == NULL) {
        DEBUG_MSG_P(PSTR("[FLOW] Error: component ID='%s' is not registered\n"), tgtProcess.c_str());
        return;
    }

    int srcNumber = _library.getOutputNumber(componentsNames[srcProcess], srcPort);
    if (srcNumber < 0) {
        DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no output named '%s'\n"), componentsNames[srcProcess].c_str(), srcPort.c_str());
        return;
    }

    int tgtNumber = _library.getInputNumber(componentsNames[tgtProcess], tgtPort);
    if (tgtNumber < 0) {
        DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no input named '%s'\n"), componentsNames[tgtProcess].c_str(), tgtPort.c_str());
        return;
    }

    srcComponent->addOutput(srcNumber, tgtComponent, tgtNumber);
}

void _flowStart(JsonObject& data) {
    std::map<String, FlowComponent*> components;
    std::map<String, String> componentsNames;

    JsonVariant processes = data.containsKey("P") ? data["P"] : data["processes"];
    if (processes.is<JsonObject>()) {
        for (auto process_kv: processes.as<JsonObject>()) {
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
    } else {
        for (JsonArray& process: processes.as<JsonArray>()) {
            String id = process[0];
            String componentName = process[1];
            JsonObject& properties = process[5];

            FlowComponent* component = _library.createComponent(componentName, properties);

            if (component != NULL) {
                components[id] = component;
                componentsNames[id] = componentName;
            } else {
                DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' is not registered\n"), componentName.c_str());
            }
        }
    }

    JsonArray& connections = data.containsKey("X") ? data["X"] : data["connections"];
    for (JsonVariant& connectionVariant: connections) {
        if (connectionVariant.is<JsonObject>()) {
            JsonObject& connection = connectionVariant.as<JsonObject>();
            JsonObject& src = connection.containsKey("S") ? connection["S"] : connection["src"];
            JsonObject& tgt = connection.containsKey("T") ? connection["T"] : connection["tgt"];

            String srcProcess = src.containsKey("I") ? src["I"] : src["process"];
            String srcPort = src.containsKey("N") ? src["N"] : src["port"];
            String tgtProcess = tgt.containsKey("I") ? tgt["I"] : tgt["process"];
            String tgtPort = tgt.containsKey("N") ? tgt["N"] : tgt["port"];

            _flowAddConnection(components, componentsNames, srcProcess, srcPort, tgtProcess, tgtPort);
        } else {
            JsonArray& connection = connectionVariant.as<JsonArray>();

            String srcProcess = connection[0];
            String srcPort = connection[1];
            String tgtProcess = connection[2];
            String tgtPort = connection[3];

            _flowAddConnection(components, componentsNames, srcProcess, srcPort, tgtProcess, tgtPort);
        }
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
        "\"properties\":[{\"name\":\"Value\",\"type\":\"any\"}]"
    "}";

class FlowStartComponent : public FlowComponent {
    private:
        JsonVariant *_value;

    public:
        FlowStartComponent(JsonObject& properties) {
            JsonVariant value = properties["Value"];
            _value = clone(value);

            flow_scheduled_task_callback_f callback = [this](unsigned long time, JsonVariant *data){ this->onStart(); };
            _flow_scheduled_tasks_queue.insert({NULL, 0, callback});
        }

        void onStart() {
            processOutput(*_value, 0);
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
                DEBUG_MSG_P(flow_debug_string, _prefix.c_str(), data.as<bool>() ? "<true>" : "<false>");
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
// Math component
// -----------------------------------------------------------------------------

PROGMEM const char flow_input1[] = "Input 1";
PROGMEM const char flow_input2[] = "Input 2";
PROGMEM const char* const flow_inputs_array[] = {flow_input1, flow_input2};

PROGMEM const FlowConnections flow_math_component = {
    2, flow_inputs_array,
    1, flow_data_array,
};

PROGMEM const char flow_math_component_json[] =
    "\"Math\": "
    "{"
        "\"name\":\"Math\","
        "\"icon\":\"plus-circle\","
        "\"inports\":[{\"name\":\"Input 1\",\"type\":\"any\"},{\"name\":\"Input 2\",\"type\":\"any\"}],"
        "\"outports\":[{\"name\":\"Data\",\"type\":\"any\"}],"
        "\"properties\":[{\"name\":\"Operation\",\"type\":\"list\","
            "\"values\":[\"+\",\"-\",\"*\",\"/\"]}]"
    "}";

class FlowMathComponent : public FlowComponent {
    private:
        String _operation;
        JsonVariant *_input1, *_input2;

    public:
        FlowMathComponent(JsonObject& properties) {
            const char * operation = properties["Operation"];
            _operation = String(operation != NULL ? operation : "");
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (inputNumber == 0) {
                if (_input1 != NULL) release(_input1);
                _input1 = clone(data);
            } else if (inputNumber == 1) {
                if (_input2 != NULL) release(_input2);
                _input2 = clone(data);
            }

            if (_input1 != NULL && _input2 != NULL) {
                if (_input1->is<int>()) {
                    int i1 = _input1->as<int>();
                    int i2 = _input2->as<int>();
                    JsonVariant r(
                        _operation.equals("+") ? i1 + i2 :
                        _operation.equals("-") ? i1 - i2 :
                        _operation.equals("*") ? i1 * i2 :
                        /*_operation.equals("/") ?*/ i1 / i2
                    );
                    processOutput(r, 0);
                } else if (_input1->is<double>()) {
                    double d1 = _input1->as<double>();
                    double d2 = _input2->as<double>();
                    JsonVariant r(
                        _operation.equals("+") ? d1 + d2 :
                        _operation.equals("-") ? d1 - d2 :
                        _operation.equals("*") ? d1 * d2 :
                        /*_operation.equals("/") ?*/ d1 / d2
                    );
                    processOutput(r, 0);
                } else if (_input1->is<char*>()) {
                    // only + is supported
                    const char *s1 = _input1->as<char*>();
                    const char *s2 = _input2->as<char*>();
                    char *concat = new char[strlen(s1) + (s2 != NULL ? strlen(s2) : 0) + 1];
                    strcpy(concat, s1);
                    if (s2 != NULL)
                        strcat(concat, s2);
                    JsonVariant r(concat);
                    processOutput(r, 0);
                    free(concat);
                } else if (_input1->is<bool>()) {
                     bool b1 = _input1->as<bool>();
                     bool b2 = _input2->as<bool>();
                     JsonVariant r(
                         _operation.equals("+") ? b1 || b2 :
                         _operation.equals("-") ? !b1 : // NOT for first only
                         _operation.equals("*") ? b1 && b2 :
                         /*_operation.equals("/") ?*/ (b1 && !b2) || (!b1 && b2) // XOR
                     );
                     processOutput(r, 0);
                 }
            }
        }

        static void reg() {
            flowRegisterComponent("Math", &flow_math_component, flow_math_component_json,
                (flow_component_factory_f)([] (JsonObject& properties) { return new FlowMathComponent(properties); }));
        }
};

// -----------------------------------------------------------------------------
// Compare component
// -----------------------------------------------------------------------------

PROGMEM const char flow_true[] = "True";
PROGMEM const char flow_false[] = "False";
PROGMEM const char flow_test[] = "Test";
PROGMEM const char* const flow_compare_inputs[] = {flow_data, flow_test};
PROGMEM const char* const flow_compare_outputs[] = {flow_true, flow_false};

PROGMEM const FlowConnections flow_compare_component = {
    2, flow_compare_inputs,
    2, flow_compare_outputs,
};

PROGMEM const char flow_compare_component_json[] =
    "\"Compare\": "
    "{"
        "\"name\":\"Compare\","
        "\"icon\":\"chevron-circle-right\","
        "\"inports\":[{\"name\":\"Data\",\"type\":\"any\"},{\"name\":\"Test\",\"type\":\"any\"}],"
        "\"outports\":[{\"name\":\"True\",\"type\":\"any\"},{\"name\":\"False\",\"type\":\"any\"}],"
        "\"properties\":[{\"name\":\"Operation\",\"type\":\"list\",\"values\":[\"=\",\">\",\"<\"]},{\"name\":\"Test\",\"type\":\"any\"}]"
    "}";

class FlowCompareComponent : public FlowComponent {
    private:
        String _operation;
        JsonVariant *_data, *_test;

    public:
        FlowCompareComponent(JsonObject& properties) {
            const char * operation = properties["Operation"];
            _operation = String(operation != NULL ? operation : "");

            JsonVariant test = properties["Test"];
            _test = clone(test);
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (inputNumber == 0) {
                if (_data != NULL) release(_data);
                _data = clone(data);
            } else if (inputNumber == 1) {
                if (_test != NULL) release(_test);
                _test = clone(data);
            }

            if (_data != NULL && _test != NULL) {
                bool r;
                if (_data->is<double>()) {
                    double d1 = _data->as<double>();
                    double d2 = _test->as<double>();
                    r = _operation.equals("=") ? d1 == d2 :
                        _operation.equals(">") ? d1 > d2 :
                        /*_operation.equals("<") ?*/ d1 < d2
                    ;
                } else if (_data->is<char*>()) {
                    int cmp = strcmp(_data->as<char*>(), _test->as<char*>());
                    r = _operation.equals("=") ? cmp == 0 :
                        _operation.equals(">") ? cmp > 0 :
                        /*_operation.equals("<") ?*/ cmp < 0
                    ;
                } else if (_data->is<bool>()) {
                    bool b1 = _data->as<bool>();
                    bool b2 = _test->as<bool>();
                    r = _operation.equals("=") ? b1 == b2 :
                        _operation.equals(">") ? b1 > b2 :
                        /*_operation.equals("<") ?*/ b1 < b2
                    ;
                }
                processOutput(*_data, r ? 0 : 1);
            }
        }

        static void reg() {
            flowRegisterComponent("Compare", &flow_compare_component, flow_compare_component_json,
                (flow_component_factory_f)([] (JsonObject& properties) { return new FlowCompareComponent(properties); }));
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
        "\"properties\":[{\"name\":\"Seconds\",\"type\":\"int\"},{\"name\":\"Value\",\"type\":\"any\"}]"
    "}";

PROGMEM const char flow_incorrect_timer_delay[] = "[FLOW] Incorrect timer delay: %i\n";

class FlowTimerComponent : public FlowComponent {
    private:
        JsonVariant *_value;
        unsigned long _period;
        flow_scheduled_task_callback_f _callback;

    public:
        FlowTimerComponent(JsonObject& properties) {
            JsonVariant value = properties["Value"];
            _value = clone(value);

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
            processOutput(*_value, 0);

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
                if ((_state && _value >= _max) || (!_state && _value <= _min)) {
                    _state = !_state;
                    processOutput(data, _state ? 0 : 1);
                }
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

#if !SPIFFS_SUPPORT && MQTT_SUPPORT
void _flowMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(FLOW_MQTT_TOPIC);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals(FLOW_MQTT_TOPIC) && millis() - _mqtt_flow_sent_at > MQTT_SKIP_TIME) {
            _flow = String(payload);
            flowStart();
        }
    }
}
#endif

void flowSetup() {
    #if !SPIFFS_SUPPORT && MQTT_SUPPORT
        mqttRegister(_flowMQTTCallback);
    #endif

    flowRegisterComponent("Start", &flow_start_component, flow_start_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowStartComponent(properties); }));

    flowRegisterComponent("Debug", &flow_debug_component, flow_debug_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDebugComponent(properties); }));

    flowRegisterComponent("Change", &flow_change_component, flow_change_component_json,
        (flow_component_factory_f)([] (JsonObject& properties) { return new FlowChangeComponent(properties); }));

    FlowMathComponent::reg();
    FlowCompareComponent::reg();

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
