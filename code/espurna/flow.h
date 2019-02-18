#pragma once

#include <vector>
#include <map>

struct FlowConnections {
    int inputsNumber;
    const char* const* inputs;
    int outputsNumber;
    const char* const* outputs;
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
            if (data.is<int>()) {
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

class FlowComponentLibrary {
    private:
        std::vector<const char*> _jsons;
        std::map<String, const FlowConnections*> _connectionsMap;
        std::map<String, flow_component_factory_f> _factoryMap;

    public:
        void addType(String name, const FlowConnections* connections, const char* json, flow_component_factory_f factory) {
            _jsons.push_back(json);
            _connectionsMap[name] = connections;
            _factoryMap[name] = factory;
        }

        const char* getComponentJson(int index) {
            if (index >= _jsons.size())
                return NULL;
            return _jsons[index];
        }

        FlowComponent* createComponent(String& name, JsonObject& properties) {
            flow_component_factory_f& factory = _factoryMap[name];
            return factory != NULL ? factory(properties) : NULL;
        }

        int getInputNumber(String& name, String& input) {
            const FlowConnections* connections = _connectionsMap[name];
            if (connections == NULL)
                return -1;

            FlowConnections temp;
            memcpy_P (&temp, connections, sizeof (FlowConnections));
            for (int i = 0; i < temp.inputsNumber; i++) {
                if (strcmp_P(input.c_str(), temp.inputs[i]) == 0)
                    return i;
            }

            return -1;
        }

        int getOutputNumber(String& name, String& output) {
            const FlowConnections* connections = _connectionsMap[name];
            if (connections == NULL)
                return -1;

            FlowConnections temp;
            memcpy_P (&temp, connections, sizeof (FlowConnections));
            for (int i = 0; i < temp.outputsNumber; i++) {
                if (strcmp_P(output.c_str(), temp.outputs[i]) == 0)
                    return i;
            }

            return -1;
        }
};
