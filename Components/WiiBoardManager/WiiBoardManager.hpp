// ======================================================================
// \title  WiiBoardManager.hpp
// \author devin
// \brief  hpp file for WiiBoardManager component implementation class
// ======================================================================

#ifndef Components_WiiBoardManager_HPP
#define Components_WiiBoardManager_HPP

#include "Components/WiiBoardManager/WiiBoardManagerComponentAc.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

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
    //! Polls the Wii Balance Board when BeeLogic requests an update
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        Bee::Ping data) override;

    //! Try to find and open the balance board input device
    bool openBoard();

    //! Drain pending input events and emit a new weight if available
    void pollBoard();

    //! Process a single EV_ABS event
    void processAbsEvent(unsigned int code, int value);

    //! Close the current board handle
    void closeBoard();

  private:
    int m_boardFd;
    std::string m_boardPath;
    std::unordered_map<unsigned int, int> m_sensorValues;
    F32 m_lastWeightKg;
    std::mutex m_stateMutex;
};

}  // namespace Components

#endif
