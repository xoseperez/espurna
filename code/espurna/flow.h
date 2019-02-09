#pragma once

#include <vector>
#include <map>

enum FlowValueType {
    ANY,
    STRING,
    INT,
    DOUBLE,
    BOOL,
    LIST,
    TIME,
    WEEKDAYS
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
            if (data == NULL) {
                return new JsonVariant(false); // workaround for JSON parsing issue
            } else if (data.is<int>()) {
                return new JsonVariant(data.as<int>());
            } else if (data.is<double>()) {
                return new JsonVariant(data.as<double>());
            } else if (data.is<bool>()) {
                return new JsonVariant(data.as<bool>());
            } else if (data.is<char*>()) {
                char *str = strdup(data.as<const char*>());
                return new JsonVariant(str);
            } else {
                return new JsonVariant(data);
            }
        }

        void release(JsonVariant* data) {
            if (data == NULL)
                return;

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

        void vectorToJSON(JsonObject& root, std::vector<name_type_t> &v, const char* name) {
            JsonArray& array = root.createNestedArray(name);
            for (unsigned int i=0; i < v.size(); i++) {
                name_type_t& element = v[i];
                JsonObject& elementObject = array.createNestedObject();
                elementObject["name"] = element.name;

                FlowValueType type = element.type;
                const char *typeName = "unknown";
                switch(type) {
                    case ANY: typeName = "any"; break;
                    case STRING: typeName = "string"; break;
                    case INT: typeName = "int"; break;
                    case DOUBLE: typeName = "double"; break;
                    case BOOL: typeName = "bool"; break;
                    case LIST: typeName = "list"; break;
                    case TIME: typeName = "time"; break;
                    case WEEKDAYS: typeName = "weekdays"; break;
                }
                elementObject["type"] = typeName;

                if (type == LIST) {
                    std::vector<String>* values = _list_values[element.name];
                    JsonArray& valuesArray = elementObject.createNestedArray("values");
                    for (unsigned int j=0; j < values->size(); j++) {
                        valuesArray.add(values->at(j));
                    }
                }
            }
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

        void toJSON(JsonObject& root) {
            root["name"] = _name;
            root["icon"] = _icon;

            vectorToJSON(root, _inputs, "inports");
            vectorToJSON(root, _outputs, "outports");
            vectorToJSON(root, _properties, "properties");
        }

        FlowComponent* createComponent(JsonObject& properties) {
            return _factory(properties);
        }
};

class FlowComponentLibrary {
    private:
        std::vector<FlowComponentType*> _types;
        std::map<String, FlowComponentType*> _typesMap;

    public:
        FlowComponentType* addType(String name, String icon, flow_component_factory_f factory) {
            FlowComponentType* type = new FlowComponentType(name, icon, factory);
            _types.push_back(type);
            _typesMap[name] = type;
            return type;
        }

        FlowComponentType* getComponent(int index) {
            if (index >= _types.size())
                return NULL;
            return _types[index];
        }

        FlowComponentType* getType(String name) {
            auto it = _typesMap.find(name);
            if (it == _typesMap.end()) return NULL;
            return it->second;
        }
};
