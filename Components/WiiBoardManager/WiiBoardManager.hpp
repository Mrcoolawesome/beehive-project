// ======================================================================
// \title  WiiBoardManager.hpp
// \author devin
// \brief  hpp file for WiiBoardManager component implementation class
// ======================================================================

#ifndef Components_WiiBoardManager_HPP
#define Components_WiiBoardManager_HPP

#include "Components/WiiBoardManager/WiiBoardManagerComponentAc.hpp"

namespace Components {

class WiiBoardManager final : public WiiBoardManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct WiiBoardManager object
    WiiBoardManager(const char* const compName  //!< The component name
    );

    //! Destroy WiiBoardManager object
    ~WiiBoardManager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for pingIn
    //!
    //! Receives the trigger from BeeLogic
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        Bee::Ping data) override;

    //! Handler implementation for dataIn
    void dataIn_handler(FwIndexType portNum,
              Fw::Buffer& recvBuffer,
              const Drv::ByteStreamStatus& recvStatus) override;
};

}  // namespace Components

#endif
