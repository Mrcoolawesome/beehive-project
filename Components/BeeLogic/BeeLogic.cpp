// ======================================================================
// \title  BeeLogic.cpp
// \author devin
// \brief  cpp file for BeeLogic component implementation class
// ======================================================================

#include "Components/BeeLogic/BeeLogic.hpp"

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

BeeLogic ::BeeLogic(const char* const compName) : BeeLogicComponentBase(compName) {}

BeeLogic ::~BeeLogic() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void BeeLogic ::receiveWeight_handler(FwIndexType portNum, F32 weight) {
    // Write the received weight to the telemetry channel
    this->tlmWrite_BoardWeight(weight);
}

void BeeLogic ::run_handler(FwIndexType portNum, U32 context) {
    if (this->m_isRunning) {
         // REQUEST THE DATA: constantly ask for the weight
         I32 myPingValue = 1; // this can be whatever integer it doesn't matter
        // It is best practice to check if the port is connected first
        if (this->isConnected_requestWeight_OutputPort(0)) {
            // Send the data out the port
            // Argument 1: port index (0)
            // Argument 2: the data (myPingValue)
            this->requestWeight_out(0, myPingValue);
        }
    }
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void BeeLogic :: RUN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_isRunning = true; // set the running boolean to true
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BeeLogic :: STOP_RUNNING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_isRunning = false; // set the running boolean to false
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Components
