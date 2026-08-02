// ======================================================================
// \title  BeeLogic.hpp
// \author devin
// \brief  hpp file for BeeLogic component implementation class
// ======================================================================

#ifndef Components_BeeLogic_HPP
#define Components_BeeLogic_HPP

#include "Components/BeeLogic/BeeLogicComponentAc.hpp"

namespace Components {

class BeeLogic final : public BeeLogicComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct BeeLogic object
    BeeLogic(const char* const compName  //!< The component name
    );

    //! Destroy BeeLogic object
    ~BeeLogic();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for receiveWeight
    //!
    //! Async input port to handle the weight data once the driver fetches it
    void receiveWeight_handler(FwIndexType portNum,  //!< The port number
                               F32 weight) override;

    //! Handler implementation for run
    //!
    //! main input port for the application
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RUN
    //!
    //! RUN: This is for turning on the application or not to pull data
    void RUN_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                        U32 cmdSeq            //!< The command sequence number
                        ) override;

    //! Handler implementation for command STOP_RUNNING
    //!
    //! STOP_RUNNING: This is to stop BeeLogic from running at all
    void STOP_RUNNING_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;
};

}  // namespace Components

#endif
