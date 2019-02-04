/*

FLOW MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if FLOW_SUPPORT

#include <vector>
#include <map>

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

void flowOutputLibrary(AsyncResponseStream *response) {
    _library.toJSON(response);
}

void flowStart() {
    DEBUG_MSG("[FLOW] Starting\n");

    File file = SPIFFS.open("/flow.json", "r"); // TODO: file name to constant
    if (file) {
        size_t size = file.size();
        uint8_t* nbuf = (uint8_t*) malloc(size + 1);
        if (nbuf) {
            size = file.read(nbuf, size);
            file.close();
            nbuf[size] = 0;

            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject((char *) nbuf);
            if (root.success()) _flowStart(root);
            else DEBUG_MSG("[FLOW] Error: Flow cannot be parsed as correct JSON\n");

            free(nbuf);
        }
        file.close();
    } else {
        DEBUG_MSG("[FLOW] No flow found\n");
    }
}

void _flowStart(JsonObject& data) {
    std::map<String, FlowComponentType*> componentTypes;
    std::map<String, FlowComponent*> components;

    JsonObject& processes = data["processes"];
    for (auto process_kv: processes) {
        String id = process_kv.key;
        JsonVariant& value = process_kv.value;
        String componentName = value["component"];
        FlowComponentType* componentType = _library.getType(componentName);

        if (componentType != NULL) {
            JsonObject& metadata = value["metadata"];
            JsonObject& properties = metadata["properties"];
            FlowComponent* component = componentType->createComponent(properties);

            componentTypes[id] = componentType;
            components[id] = component;
        } else {
            DEBUG_MSG_P(PSTR("[FLOW] Error: component '%s' is not registered\n"), componentName.c_str());
        }
    }

    JsonArray& connections = data["connections"];
    for (JsonObject& connection: connections) {
        JsonObject& src = connection["src"];
        String srcProcess = src["process"];
        String srcPort = src["port"];

        JsonObject& tgt = connection["tgt"];
        String tgtProcess = tgt["process"];
        String tgtPort = tgt["port"];

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
            if (data.is<int>()) {
                DEBUG_MSG_P("[FLOW DEBUG] %s%i\n", _prefix.c_str(), data.as<int>());
            } else {
                DEBUG_MSG_P("[FLOW DEBUG] %s%s\n", _prefix.c_str(), data.as<const char*>());
            }
        }
};

class FlowPauseComponent : public FlowComponent {
    public:
        FlowPauseComponent(JsonObject& properties) {
        }
};

void flowSetup() {
   flowRegisterComponent("Debug", "eye", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDebugComponent(properties); }))
        ->addInput("Data", ANY)
        ->addProperty("Prefix", STRING)
        ;

   flowRegisterComponent("Delay", "pause", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowPauseComponent(properties); }))
        ->addInput("Payload", ANY)
        ->addOutput("Payload", ANY)
        ->addProperty("Time", INT)
        ;
}

#endif // FLOW_SUPPORT
