module Components {
    @ This will be what actually gets the data from the driver.
    passive component WiiBoardManager {

        @ Runs the Bluetooth reconnect loop once a second.
        sync input port run: Svc.Sched

        @ Receives the trigger from BeeLogic
        sync input port pingIn: Bee.PingPort

        @ Sends the final weight calculation back to BeeLogic
        output port weightOut: Bee.WeightTelemetryPort

        @ Raised when the reconnect loop is actively retrying Bluetooth pairing and connection
        event BluetoothReconnectAttempt() severity activity high format "Retrying Wii Balance Board Bluetooth connection"

        @ Raised when the board is not trusted or paired yet and the manager is issuing the setup commands
        event BluetoothSetupAttempt() severity activity high format "Trusting and pairing Wii Balance Board"

        @ Raised when the board is already trusted and paired and only the connect command is being retried
        event BluetoothConnectAttempt() severity activity high format "Attempting Wii Balance Board connection"

        @ Raised when the manager has opened the board input device and is receiving updates
        event BluetoothConnected() severity activity high format "Wii Balance Board connected"

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

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}