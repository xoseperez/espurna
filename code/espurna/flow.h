#pragma once

#include <vector>
#include <map>

enum FlowValueType {
    ANY,
    STRING,
    INT,
    DOUBLE,
    BOOL,
    LIST
};

class FlowComponent {
    private:
        typedef struct {
            FlowComponent* component;
            int inputNumber;
        } output_t;

        std::vector<std::vector<output_t>> _outputs;

    protected:
        void processOutput(JsonVariant& data, int outputNumber) {
            if (outputNumber < _outputs.size()) {
                for (output_t output : _outputs[outputNumber])
                    output.component->processInput(data, output.inputNumber);
            }
        }

        JsonVariant* clone(JsonVariant& data) {
            if (data.is<char*>()) {
                char *str = strdup(data.as<const char*>());
                return new JsonVariant(str);
            } else {
                return new JsonVariant(data);
            }
        }

        void release(JsonVariant* data) {
            if (data->is<char*>()) {
                void* str = (void*)data->as<char*>();
                free(str);
            }
            delete data;
        }

    public:
        FlowComponent() {
        }

        void addOutput(int outputNumber, FlowComponent* component, int inputNumber) {
            if (outputNumber >= _outputs.size())
                _outputs.resize(outputNumber + 1);
            _outputs[outputNumber].push_back({component, inputNumber});
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
        }
};

typedef std::function<FlowComponent* (JsonObject&)> flow_component_factory_f;

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
        std::map<String, std::vector<String>*> _list_values;

        void vectorToJSON(AsyncResponseStream *response, std::vector<name_type_t> &v, const char* name) {
            response->printf("\"%s\": [", name);
            if (v.size() > 0) {
                for (unsigned int i=0; i < v.size(); i++) {
                    if (i > 0)
                        response->print(",");
                    FlowValueType type = v[i].type;
                    const char *typeName = "unknown";
                    switch(type) {
                        case ANY: typeName = "any"; break;
                        case STRING: typeName = "string"; break;
                        case INT: typeName = "int"; break;
                        case DOUBLE: typeName = "double"; break;
                        case BOOL: typeName = "bool"; break;
                        case LIST: typeName = "list"; break;
                    }
                    response->printf("\n\t\t\t{\"name\": \"%s\", \"type\": \"%s\"", v[i].name.c_str(), typeName);
                    if (type == LIST) {
                        std::vector<String>* values = _list_values[v[i].name];
                        response->print(", \"values\": [");
                        for (unsigned int j=0; j < values->size(); j++) {
                            if (j > 0)
                                response->print(", ");
                            response->printf("\"%s\"", values->at(j).c_str());
                        }
                        response->print("]");
                    }
                    response->print("}");
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

        FlowComponentType* addProperty(String name, std::vector<String>* values) {
            _properties.push_back({name, LIST});
            _list_values[name] = values;
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
