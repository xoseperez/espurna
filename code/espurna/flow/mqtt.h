class FlowMqttSubscribeComponent : public FlowComponent {
    private:
        String _topic;

        void mqttCallback(unsigned int type, const char * topic, const char * payload) {
            if (type == MQTT_CONNECT_EVENT) {
                mqttSubscribeRaw(_topic.c_str());
            }

            if (type == MQTT_MESSAGE_EVENT) {
                if (strcmp(topic, _topic.c_str()) == 0) {
                    JsonVariant data(payload);
                    processOutput(data, 0);
                }
            }
        }

    public:
        FlowMqttSubscribeComponent(JsonObject& properties) {
            const char * topic = properties["Topic"];
            _topic = String(topic != NULL ? topic : "");

            mqtt_callback_f callback = [this](unsigned int type, const char * topic, const char * payload){ this->mqttCallback(type, topic, payload); };
            mqttRegister(callback);
        }
};

class FlowMqttPublishComponent : public FlowComponent {
    private:
        String _topic;
        bool _retain;

    public:
        FlowMqttPublishComponent(JsonObject& properties) {
            const char * topic = properties["Topic"];
            _topic = String(topic != NULL ? topic : "");
            _retain = properties["Retain"];
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            mqttSendRaw(_topic.c_str(), data.as<const char*>(), _retain);
        }
};

void flowRegisterMqtt(FlowComponentLibrary& library) {
    library.addType("MQTT subscribe", "sign-out", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowMqttSubscribeComponent(properties); }))
        ->addOutput("Payload", STRING)
        ->addProperty("Topic", STRING)
        ;

    library.addType("MQTT publish", "sign-in", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowMqttPublishComponent(properties); }))
        ->addInput("Payload", STRING)
        ->addProperty("Topic", STRING)
        ->addProperty("Retain", BOOL)
        ;
}