module Components {
    @ This is the main application manager for the project. It is active because it will be the invoker for everything.
    active component BeeLogic {

        @ RUN: This is for turning on the application or not to pull data
        # TODO: make this happen by default 
        async command RUN opcode 0

        @ STOP_RUNNING: This is to stop BeeLogic from running at all
        async command STOP_RUNNING opcode 1

        @ main input port for the application
        # this will run on every 'tick' given the rate group
        # NOTE: if we ever wanna get this to actually run off of a specific interval (e.g. 1hz) refrence what Josh did in the GASRATS project to make a rate group run off of the system clock
        sync input port run: Svc.Sched
        # sync input ports run on the thread of the invoking component, not the component it was declared in
        # async input ports run on the thread of the port it was declared in -- hence why we're using it here because this is an active component with its own thread
        # for passive or queued components we would wanna run sync input ports in those so it runs on the invokers thread because that's what it'll do anyways if we were to do async input ports

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

        @ Enables command handling
        import Fw.Command

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