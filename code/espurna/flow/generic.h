class FlowDebugComponent : public FlowComponent {
    private:
        String _prefix;
    public:
        FlowDebugComponent(JsonObject& properties) {
            const char * prefix = properties["Prefix"];
            _prefix = String(prefix != NULL ? prefix : "");
        }

        virtual void processInput(JsonVariant& data, int inputNumber) {
            DEBUG_MSG_P("[FLOW DEBUG] %s%s\n", _prefix.c_str(), data.as<const char*>());
        }
};

class FlowPauseComponent : public FlowComponent {
    public:
        FlowPauseComponent(JsonObject& properties) {
        }
};

void flowRegisterGeneric(FlowComponentLibrary& library) {
    library.addType("Debug", "eye", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowDebugComponent(properties); }))
        ->addInput("Data", ANY)
        ->addProperty("Prefix", STRING)
        ;

    library.addType("Delay", "pause", (flow_component_factory_f)([] (JsonObject& properties) { return new FlowPauseComponent(properties); }))
        ->addInput("Payload", ANY)
        ->addOutput("Payload", ANY)
        ->addProperty("Time", INT)
        ;
}