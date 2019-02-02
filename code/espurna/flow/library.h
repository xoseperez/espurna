#pragma once

#include "component.h"
#include <map>

typedef std::function<FlowComponent* (JsonObject&)> flow_component_factory_f;

enum FlowValueType {
    ANY,
    STRING,
    INT,
    DOUBLE,
    BOOL
};

class FlowComponentType {
    private:
        typedef struct {
            String name;
            FlowValueType type;
        } name_type_t;

        String _name;
        String _icon;
        flow_component_factory_f _factory;
        std::vector<name_type_t> _inputs;
        std::vector<name_type_t> _outputs;
        std::vector<name_type_t> _properties;

        void vectorToJSON(AsyncResponseStream *response, std::vector<name_type_t> &v, const char* name) {
            response->printf("\"%s\": [", name);
            if (v.size() > 0) {
                for (unsigned int i=0; i < v.size(); i++) {
                    if (i > 0)
                        response->print(",");
                    const char *typeName = "unknown";
                    switch(v[i].type) {
                        case ANY: typeName = "any"; break;
                        case STRING: typeName = "string"; break;
                        case INT: typeName = "int"; break;
                        case DOUBLE: typeName = "double"; break;
                        case BOOL: typeName = "bool"; break;
                    }
                    response->printf("\n\t\t\t{\"name\": \"%s\", \"type\": \"%s\"}", v[i].name.c_str(), typeName);
                }
            }
            response->print("]");
        }

    public:
        FlowComponentType(String name, String icon, flow_component_factory_f factory) {
            _name = name;
            _icon = icon;
            _factory = factory;
        }

        String name() {
            return _name;
        }

        FlowComponentType* addInput(String name, FlowValueType type) {
            _inputs.push_back({name, type});
            return this;
        }

        FlowComponentType* addOutput(String name, FlowValueType type) {
            _outputs.push_back({name, type});
            return this;
        }

        FlowComponentType* addProperty(String name, FlowValueType type) {
            _properties.push_back({name, type});
            return this;
        }

        int getInputNumber(String& name) {
            for (int i = 0; i < _inputs.size(); i++) {
                if (_inputs[i].name.equalsIgnoreCase(name))
                    return i;
            }
            return -1;
        }

        int getOutputNumber(String& name) {
            for (int i = 0; i < _outputs.size(); i++) {
                if (_outputs[i].name.equalsIgnoreCase(name))
                    return i;
            }
            return -1;
        }

        void toJSON(AsyncResponseStream *response) {
            response->print("{\n\t\t");
            response->printf("\"name\": \"%s\",\n\t\t", _name.c_str());
            response->printf("\"icon\": \"%s\",\n\t\t", _icon.c_str());
            vectorToJSON(response, _inputs, "inports");
            response->print(",\n\t\t");
            vectorToJSON(response, _outputs, "outports");
            response->print(",\n\t\t");
            vectorToJSON(response, _properties, "properties");
            response->print("}");
        }

        FlowComponent* createComponent(JsonObject& properties) {
            return _factory(properties);
        }
};

class FlowComponentLibrary {
    private:
        std::map<String, FlowComponentType*> _types;

    public:
        FlowComponentType* addType(String name, String icon, flow_component_factory_f factory) {
            FlowComponentType* type = new FlowComponentType(name, icon, factory);
            _types[name] = type;
            return type;
        }

        void toJSON(AsyncResponseStream *response) {
            response->print("{");
            bool first = true;
            for (auto pair : _types) {
                FlowComponentType* type = pair.second;
                if (first) first = false; else response->print(",");
                response->printf("\n\t\"%s\": ", type->name().c_str());
                type->toJSON(response);
            }
            response->print("}");
        }

        FlowComponentType* getType(String name) {
            return _types[name];
        }
};
