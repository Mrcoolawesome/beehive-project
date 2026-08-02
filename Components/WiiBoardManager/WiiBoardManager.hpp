// ======================================================================
// \title  WiiBoardManager.hpp
// \author devin
// \brief  hpp file for WiiBoardManager component implementation class
// ======================================================================

#ifndef Components_WiiBoardManager_HPP
#define Components_WiiBoardManager_HPP

#include "Components/WiiBoardManager/WiiBoardManagerComponentAc.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Components {

struct WiiBoardBluetoothStatus {
    bool paired;
    bool trusted;
    bool connected;
};

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
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context) override;

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

    //! Emit the current weight once a full input frame has been collected
    void emitWeight();

    //! Report a connection loss and close the current board handle
    void handleConnectionLost();

    //! Close the current board handle
    void closeBoard();

    //! Refresh the bluetooth connection state and issue retry commands when needed
    void maintainBluetoothConnection();

    //! Query the current bluetoothctl status for the balance board
    bool queryBluetoothStatus(WiiBoardBluetoothStatus& status);

    //! Send a sequence of bluetoothctl commands
    bool sendBluetoothCommands(const char* const* commands, std::size_t count);

    //! Mark the board as connected and raise the connection event once
    void notifyConnectedIfNeeded();

    //! Start a new timed connected session
    void startConnectedSession();

    //! End the current connected session and disconnect the board over bluetooth
    void disconnectBoard();

    //! Update cached calibration values when parameters change
    void parameterUpdated(FwPrmIdType id) override;

  private:
    int m_boardFd;
    std::string m_boardPath;
    std::unordered_map<unsigned int, int> m_sensorValues;
    F32 m_lastWeightKg;
    F32 m_tareKg;
    F32 m_scaleFactor;
    bool m_connectionEventRaised;
    bool m_sessionActive;
    bool m_weightDirty;
    U32 m_connectedSecondsRemaining;
    std::mutex m_stateMutex;
};

}  // namespace Components

#endif
