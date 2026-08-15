// ======================================================================
// \title  WiiBoardManager.hpp
// \author devin
// \brief  hpp file for WiiBoardManager component implementation class
// ======================================================================

#ifndef Components_WiiBoardManager_HPP
#define Components_WiiBoardManager_HPP

#include "Components/WiiBoardManager/WiiBoardManagerComponentAc.hpp"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <array>

namespace Components {

constexpr std::size_t SESSION_SAMPLE_CAPACITY = 60U;

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

    //! Capture the current calibrated weight into the timed archive buffer
    void captureSessionSample();

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

    //! Run a full trust/pair/connect attempt, giving the (pre-SSP) board's
    //! over-the-air pairing handshake real time to complete before quitting
    bool attemptPairing(const std::string& trustCommand,
                        const std::string& pairCommand,
                        const std::string& connectCommand);

    //! Connect to a board that's already paired/trusted, giving the HID
    //! session real time to come up before quitting
    bool attemptConnectOnly(const std::string& connectCommand);

    //! Run a (potentially many-second) bluetoothctl attempt on a background
    //! thread so it doesn't stall the rate group this component runs on
    void runBluetoothAttempt(bool alreadyPaired);

    //! Mark the board as connected and raise the connection event once
    void notifyConnectedIfNeeded();

    //! Start a new timed connected session
    void startConnectedSession();

    //! Request the archived session data product after disconnect
    void requestSessionArchive();

    //! End the current connected session and disconnect the board over bluetooth
    void disconnectBoard();

    //! Receive a data product container and serialize the archived session into it
    void dpRecv_WeightSessionContainer_handler(DpContainer& container, Fw::Success::T status) override;

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
    bool m_archivePending;
    bool m_weightDirty;
    U32 m_connectedSecondsRemaining;
    U32 m_pairingRetryCooldown;
    bool m_bootPairingModeResolved;
    bool m_boardPairedAtBoot;
    std::atomic<bool> m_bluetoothAttemptInProgress;
    std::thread m_bluetoothWorker;
    std::array<F32, SESSION_SAMPLE_CAPACITY> m_sessionSamples;
    U32 m_sessionSampleCount;
    DpContainer m_sessionDpContainer;
    std::mutex m_stateMutex;
};

}  // namespace Components

#endif
