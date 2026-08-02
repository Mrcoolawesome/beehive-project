#include <Fw/Port/Port.hpp>

// Component definition for the Wii Board Manager layer.
// This component handles intermediate processing: calibration and filtering.
class WiiBoardManager : public Fw::Component {
public:
    WiiBoardManager();

    void initialize() override;
    void run() override;
};

// Input port receiving raw sensor data from the Driver.
Fw::Port<uint8_t[]> RawSensorData("RawSensorData");

// Output port providing standardized weight metrics (e.g., in kilograms) to the Application layer.
Fw::Port<float> ProcessedWeight("ProcessedWeight");

// Component definition structure
#define COMPONENT_DEFINITION(WiiBoardManager) \
    COMPONENT(WiiBoardManager, "Wii Board Manager", 0) \
    PORT(RawSensorData, Input) \
    PORT(ProcessedWeight, Output)
