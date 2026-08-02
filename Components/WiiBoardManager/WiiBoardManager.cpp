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
    // If your Python TCP script is constantly streaming data on its own, 
    // you actually don't need to do anything here! 
    // The TCP driver will automatically push data into dataIn_handler as it arrives.
}

void WiiBoardManager ::dataIn_handler(FwIndexType portNum,
                                      Fw::Buffer& recvBuffer,
                                      const Drv::ByteStreamStatus& recvStatus) {
    
    // 1. Only process if the TCP read was successful and has enough bytes (4 bytes for an F32)
    if (recvStatus == Drv::ByteStreamStatus::OP_OK && recvBuffer.getSize() >= sizeof(F32)) {
        
        // 2. Wrap the raw bytes in a serializer to easily extract the float
        Fw::ExternalSerializeBuffer extBuf(recvBuffer.getData(), recvBuffer.getSize());
        F32 weight = 0.0f;
        
        // 3. Unpack the bytes back into the F32 variable
        extBuf.deserializeTo(weight);
        
        // 4. Send the final float back to BeeLogic
        if (this->isConnected_weightOut_OutputPort(0)) {
            this->weightOut_out(0, weight);
        }
    }

    // 5. CRITICAL: Give the memory buffer back to the Buffer Manager!
    // If you forget this step, the TCP driver will run out of memory after a few seconds.
    if (this->isConnected_deallocate_OutputPort(0)) {
        this->deallocate_out(0, recvBuffer);
    }
}

}  // namespace Components
