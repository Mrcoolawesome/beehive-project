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
    // TODO
}

void BeeLogic ::run_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void BeeLogic ::RUN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BeeLogic ::STOP_RUNNING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Components
