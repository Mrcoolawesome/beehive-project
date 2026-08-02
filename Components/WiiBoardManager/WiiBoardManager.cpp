// ======================================================================
// \title  WiiBoardManager.cpp
// \author devin
// \brief  cpp file for WiiBoardManager component implementation class
// ======================================================================

#include "Components/WiiBoardManager/WiiBoardManager.hpp"

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WiiBoardManager ::WiiBoardManager(const char* const compName) : WiiBoardManagerComponentBase(compName) {}

WiiBoardManager ::~WiiBoardManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void WiiBoardManager ::pingIn_handler(FwIndexType portNum, Bee::Ping data) {
    // TODO
}

}  // namespace Components
