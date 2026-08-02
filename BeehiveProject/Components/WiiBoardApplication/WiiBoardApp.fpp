#include <Fw/Port/Port.hpp>

// Component definition for the Wii Board Application layer.
// This component handles high-level business logic and telemetry dispatch.
class WiiBoardApp : public Fw::Component {
public:
    WiiBoardApp();

    void initialize() override;
    void run() override;
};

// Input port receiving processed weight data from the Manager.
Fw::Port<float> ProcessedWeight("ProcessedWeight");

// Standard telemetry output port for F Prime communication layer.
Fw::Port<float> TlmOutput("TlmOutput");

// Component definition structure
#define COMPONENT_DEFINITION(WiiBoardApp) \
    COMPONENT(WiiBoardApp, "Wii Board Application", 0) \
    PORT(ProcessedWeight, Input) \
    PORT(TlmOutput, Output)
