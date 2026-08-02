# F' Architecture & Patterns

Every functional unit is split into three layers that flow unidirectionally, keeping I/O separate from logic and telemetry dispatch. This separation makes adding new sensors or changing how data routes a one-layer task rather than a rewrite:

1. Driver — own low-level hardware interfacing /dev/input/* (evdev) directly; only reads raw bytes, no calibration, no units
2. Manager — owns state and conversion; applies tare offsets, converts raw readings to kg or lb, filters noise with exponential moving averages (alpha ~0.9), publishes standardized measurements
3. Application — high-level business logic: sampling rates (~1Hz), alert thresholds (rapid weight change = evacuation signal) , dispatches telemetry via fprime's comms layer

Data flows Driver -> Manager -> Application; no cross-layer leaks, every transition has a single contract file (.fpp). New hardware is added as a new set of components in the same folder — CMake picks it up automatically through project.cmake, so you never edit CMakeLists.txt manually
