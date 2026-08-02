#include <Fw/Port/Port.hpp>

// Component definition for the Wii Board Driver layer.
// This component handles low-level hardware interfacing and raw I/O.
class WiiBoardDriver : public Fw::Component {
public:
    WiiBoardDriver();

    void initialize() override;
    void run() override;
};

// Define a port for outputting raw sensor data to the Manager layer.
Fw::Port<uint8_t[]> RawSensorData("RawSensorData");

// Component definition structure
#define COMPONENT_DEFINITION(WiiBoardDriver) \
    COMPONENT(WiiBoardDriver, "Wii Board Driver", 0) \
    PORT(RawSensorData, Output)
