module Bee {
    
    @ A custom type alias for a 32-bit signed integer
    type Ping = I32

    @ Port definition that carries our custom Ping type
    port PingPort(
        data: Bee.Ping
    )

    @ Port to carry the telemetry back to BeeLogic
    port WeightTelemetryPort(
        weight: F32
    )

}