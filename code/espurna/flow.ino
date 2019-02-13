/*

FLOW MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if FLOW_SUPPORT

#include <vector>
#include <map>
#include <set>

#include "flow.h"

// -----------------------------------------------------------------------------
// FLOW
// -----------------------------------------------------------------------------

FlowComponentLibrary _library;

FlowComponentType* flowRegisterComponent(String name, String icon, flow_component_factory_f factory) {
    return _library.addType(name, icon, factory);
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
        DEBUG_MSG("[FLOW] Error saving data\n");
    }
    return result;
}

FlowComponentType* flowGetComponent(int index) {
    return _library.getComponent(index);
}

void flowStart() {
    DEBUG_MSG("[FLOW] Starting\n");

    File file = SPIFFS.open("/flow.json", "r"); // TODO: file name to constant
    if (file) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(file);
        if (root.success()) _flowStart(root);
        else DEBUG_MSG("[FLOW] Error: flow cannot be parsed as correct JSON\n");

        file.close();
    } else {
        DEBUG_MSG("[FLOW] No flow found\n");
    }
}

void _flowStart(JsonObject& data) {
    std::map<String, FlowComponentType*> componentTypes;
    std::map<String, FlowComponent*> components;

    JsonObject& processes = data.containsKey("P") ? data["P"] : data["processes"];
    for (auto process_kv: processes) {
        String id = process_kv.key;
        JsonObject& value = process_kv.value;
        String componentName = value.containsKey("C") ? value["C"] : value["component"];
        FlowComponentType* componentType = _library.getType(componentName);

        if (componentType != NULL) {
            JsonObject& metadata = value.containsKey("M") ? value["M"] : value["metadata"];
            JsonObject& properties = metadata.containsKey("R") ? metadata["R"] : metadata["properties"];
            FlowComponent* component = componentType->createComponent(properties);

            componentTypes[id] = componentType;
            components[id] = component;
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

        FlowComponentType* srcComponentType = componentTypes[srcProcess];
        FlowComponentType* tgtComponentType = componentTypes[tgtProcess];

        int srcNumber = srcComponentType->getOutputNumber(srcPort);
        if (srcNumber < 0) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no output named '%s'\n"), srcComponentType->name().c_str(), srcPort.c_str());
            continue;
        }

        int tgtNumber = tgtComponentType->getInputNumber(tgtPort);
        if (tgtNumber < 0) {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' has no input named '%s'\n"), tgtComponentType->name().c_str(), tgtPort.c_str());
            continue;
        }

        srcComponent->addOutput(srcNumber, tgtComponent, tgtNumber);
    }
}

class FlowDebugComponent : public FlowComponent {
    private:
        String _prefix;

    public:
        FlowDebugComponent(JsonObject& properties) {
            const char * prefix = properties["Prefix"];
            _prefix = String(prefix != NULL ? prefix : "");
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
//            if (data == NULL) {
//            if (data.as<const char *>() == NULL) {
//                DEBUG_MSG_P("[FLOW DEBUG] %sEMPTY\n", _prefix.c_str());
//            } else
            if (data.is<int>()) {
                DEBUG_MSG_P("[FLOW DEBUG] %s%i\n", _prefix.c_str(), data.as<int>());
            } else if (data.is<double>()) {
                char buffer[64];
                dtostrf(data.as<double>(), 1 - sizeof(buffer), 3, buffer);
                DEBUG_MSG_P("[FLOW DEBUG] %s%s\n", _prefix.c_str(), buffer);
            } else if (data.is<bool>()) {
                DEBUG_MSG_P("[FLOW DEBUG] %s%s\n", _prefix.c_str(), data.as<bool>() ? "true" : "false");
            } else if (data.is<char*>()) {
                DEBUG_MSG_P("[FLOW DEBUG] %s%s\n", _prefix.c_str(), data.as<const char*>());
            } else {
                DEBUG_MSG_P("[FLOW DEBUG] %sUNKNOWN\n", _prefix.c_str());
            }
        }
};

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

class FlowDelayComponent;

struct delay_queue_element_t {
    JsonVariant *data;
    unsigned long time;
    FlowDelayComponent *component;

    bool operator() (const delay_queue_element_t& lhs, const delay_queue_element_t& rhs) const {
        return lhs.time < rhs.time;
    }
};

std::set<delay_queue_element_t, delay_queue_element_t> _delay_queue;

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
            _delay_queue.insert({clone(data), millis() + _time, this});
            _queueSize++;
        }

        void processElement(JsonVariant *data) {
            if (!_lastOnly || _queueSize == 1)
                processOutput(*data, 0);

            _queueSize--;
            release(data);
        }
};

class FlowTimerComponent;
std::vector<FlowTimerComponent*> _timer_components;

class FlowTimerComponent : public FlowComponent {
    private:
        JsonVariant *_data = new JsonVariant(true);
        long _period;
        long _lastMillis;

    public:
        FlowTimerComponent(JsonObject& properties) {
            int seconds = properties["Seconds"];
            _period = 1000 * (int)seconds;
            _lastMillis = millis();

            if (_period > 0)
                _timer_components.push_back(this);
            else
                DEBUG_MSG_P("[FLOW] Incorrect timer delay: %i\n", seconds);
        }

        void check() {
            long now = millis();
            if (now >= _lastMillis + _period) {
                processOutput(*_data, 0);
                _lastMillis = now;
            }
        }
};

class FlowStartComponent;
std::vector<FlowStartComponent*> _start_components;

class FlowStartComponent : public FlowComponent {
    private:
        JsonVariant *_data = new JsonVariant(true);

    public:
        FlowStartComponent(JsonObject& properties) {
            _start_components.push_back(this);
        }

        void start() {
            processOutput(*_data, 0);
        }
};

void _flowComponentLoop() {
    // TODO: have single scheduler map for all components
    if (!_start_components.empty()) {
        for (unsigned int i=0; i < _start_components.size(); i++) {
            _start_components[i]->start();
        }
        _start_components.clear();
    }

    if (!_delay_queue.empty()) {
        auto it = _delay_queue.begin();
        const delay_queue_element_t element = *it;
        if (element.time <= millis()) {
            element.component->processElement(element.data);
            _delay_queue.erase(it);
        }
    }

    if (!_timer_components.empty()) {
        for (unsigned int i=0; i < _timer_components.size(); i++) {
            _timer_components[i]->check();
        }
    }
}

class FlowGateComponent : public FlowComponent {
    private:
        bool _state = true;

    public:
        FlowGateComponent(JsonObject& properties) {
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            if (inputNumber == 0) { // data
                if (_state) {
                    processOutput(data, 0);
                }
            } else { // state
                _state = data.as<bool>();
            }
        }
};

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

void flowSetup() {
   flowRegisterComponent("Start", "play", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowStartComponent(properties); }))
        ->addOutput("Event", BOOL)
        ;

   flowRegisterComponent("Debug", "bug", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDebugComponent(properties); }))
        ->addInput("Data", ANY)
        ->addProperty("Prefix", STRING)
        ;

    flowRegisterComponent("Change", "edit", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowChangeComponent(properties); }))
        ->addInput("Data", ANY)
        ->addOutput("Data", ANY)
        ->addProperty("Value", ANY)
        ;

   flowRegisterComponent("Delay", "pause", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDelayComponent(properties); }))
        ->addInput("Data", ANY)
        ->addOutput("Data", ANY)
        ->addProperty("Seconds", INT)
        ->addProperty("Last only", BOOL)
        ;

   flowRegisterComponent("Timer", "clock-o", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowTimerComponent(properties); }))
        ->addOutput("Event", BOOL)
        ->addProperty("Seconds", INT)
        ;

   flowRegisterComponent("Gate", "unlock", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowGateComponent(properties); }))
        ->addInput("Data", ANY)
        ->addInput("State", BOOL)
        ->addOutput("Data", ANY)
        ;

   flowRegisterComponent("Hysteresis", "line-chart", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowHysteresisComponent(properties); }))
        ->addInput("Value", DOUBLE)
        ->addInput("Min", DOUBLE)
        ->addInput("Max", DOUBLE)
        ->addOutput("Rise", DOUBLE)
        ->addOutput("Fall", DOUBLE)
        ->addProperty("Min", DOUBLE)
        ->addProperty("Max", DOUBLE)
        ;

    // TODO: each component should have its own loop lambda function, in this case deque instead of set could be used
    // for delay elements container
    espurnaRegisterLoop(_flowComponentLoop);
}

#endif // FLOW_SUPPORT
