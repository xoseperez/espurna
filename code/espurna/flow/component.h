#pragma once

#include <vector>

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