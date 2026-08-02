module Components {
    @ This will be what actually gets the data from the driver.
    passive component WiiBoardManager {

    constant SESSION_SAMPLE_CAPACITY = 60

        @ Runs the Bluetooth reconnect loop once a second.
        sync input port run: Svc.Sched

        @ Receives the trigger from BeeLogic
        sync input port pingIn: Bee.PingPort

        @ Sends the final weight calculation back to BeeLogic
        output port weightOut: Bee.WeightTelemetryPort

        @ Requests a data product container for the 1-minute session archive
        product request port productRequestOut

        @ Receives the allocated data product container
        sync product recv port productRecvIn

        @ Sends the completed session archive to the data product manager
        product send port productSendOut

        @ Raised when the reconnect loop is actively retrying Bluetooth pairing and connection
        event BluetoothReconnectAttempt() severity activity high format "Retrying Wii Balance Board Bluetooth connection"

        @ Raised when the board is not trusted or paired yet and the manager is issuing the setup commands
        event BluetoothSetupAttempt() severity activity high format "Trusting and pairing Wii Balance Board"

        @ Raised when the board is already trusted and paired and only the connect command is being retried
        event BluetoothConnectAttempt() severity activity high format "Attempting Wii Balance Board connection"

        @ Raised when the manager has opened the board input device and is receiving updates
        event BluetoothConnected() severity activity high format "Wii Balance Board connected"

        @ Raised when the manager begins its timed one-minute data collection window
        event BluetoothSessionStarted() severity activity high format "Wii Balance Board session started"

        @ Raised when the manager intentionally disconnects the board after the timed window expires
        event BluetoothAutoDisconnect() severity activity high format "Disconnecting Wii Balance Board after timed session"

        @ Raised when the manager detects the board dropped off after it had been connected
        event BluetoothDisconnected() severity warning high format "Wii Balance Board disconnected"

        @ Raised when the board stops responding while the manager is polling it
        event BluetoothConnectionLost() severity warning high format "Lost Wii Balance Board connection"

        ##############################################################################
        #### Uncomment the following examples to start customizing your component ####
        ##############################################################################

        # @ Example async command
        # async command COMMAND_NAME(param_name: U32)

        # @ Example telemetry counter
        # telemetry ExampleCounter: U64

        # @ Example event
        # event ExampleStateEvent(example_state: Fw.On) severity activity high id 0 format "State set to {}"

        # @ Example port: receiving calls from the rate group
        # sync input port run: Svc.Sched

        # @ Example parameter
        # param PARAMETER_NAME: U32

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables command handling for parameter updates
        import Fw.Command

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

        @ Empty-board tare in kilograms
        param tareKg: F32 default 3.80 id 0

        @ Scale factor applied after tare correction
        param scaleFactor: F32 default 1.30 id 1

        @ Session archive sample buffer
        array WeightSessionSamples = [SESSION_SAMPLE_CAPACITY] F32

        @ Archived 1-minute capture from the board
        struct WeightSession {
            sampleCount: U32
            samples: WeightSessionSamples
        }

        @ Data product record for the 1-minute capture
        product record WeightSessionRecord: WeightSession id 0

        @ Data product container used to send the archived session to GDS
        product container WeightSessionContainer id 0 default priority 10

    }
}