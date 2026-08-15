// ======================================================================
// \title  WiiBoardManager.cpp
// \author devin
// \brief  cpp file for WiiBoardManager component implementation class
// ======================================================================

#include "Components/WiiBoardManager/WiiBoardManager.hpp"

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Components {

namespace {

constexpr const char* BOARD_NAME = "Nintendo Wii Remote Balance Board";
constexpr const char* BOARD_ADDRESS = "00:1F:32:22:03:BF";
constexpr U32 CONNECTED_SESSION_SECONDS = 60U;

// The balance board predates Secure Simple Pairing, so the over-the-air
// pairing handshake is slower than a modern device's and needs real wall
// time to finish rather than being torn down as soon as commands are
// written to bluetoothctl's stdin.
constexpr unsigned int PAIRING_HANDSHAKE_SECONDS = 8U;
constexpr unsigned int PAIRING_CONNECT_SETTLE_SECONDS = 2U;
// The board only becomes a known/pairable Device object to bluetoothd after
// an active inquiry scan finds it - without this, `trust`/`pair`/`connect`
// all fail instantly with "Device ... not available" instead of actually
// attempting anything over the air.
constexpr unsigned int DISCOVERY_SCAN_SECONDS = 10U;
// Minimum gap between fresh trust/pair/connect attempts (in 1 Hz run_handler
// ticks) so a new bluetoothctl process isn't launched on top of one that's
// still mid-handshake.
constexpr U32 PAIRING_RETRY_COOLDOWN_TICKS = 15U;

}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WiiBoardManager ::WiiBoardManager(const char* const compName)
        : WiiBoardManagerComponentBase(compName),
            m_boardFd(-1),
            m_lastWeightKg(0.0f),
            m_connectionEventRaised(false),
            m_sessionActive(false),
            m_archivePending(false),
            m_weightDirty(false),
            m_connectedSecondsRemaining(0U),
            m_pairingRetryCooldown(0U),
            m_bootPairingModeResolved(false),
            m_boardPairedAtBoot(false),
            m_bluetoothAttemptInProgress(false),
            m_sessionSamples{},
            m_sessionSampleCount(0U),
            m_sessionDpContainer() {}

WiiBoardManager ::~WiiBoardManager() {
    if (this->m_bluetoothWorker.joinable()) {
        this->m_bluetoothWorker.join();
    }
    this->closeBoard();
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void WiiBoardManager ::run_handler(FwIndexType portNum, U32 context) {
    (void)portNum;
    (void)context;
    this->pollBoard();

    if (this->m_boardFd >= 0 && this->m_sessionActive) {
        this->captureSessionSample();

        if (this->m_connectedSecondsRemaining > 0U) {
            --this->m_connectedSecondsRemaining;
        }

        if (this->m_connectedSecondsRemaining == 0U) {
            this->disconnectBoard();
        }
    }
}

void WiiBoardManager ::pingIn_handler(FwIndexType portNum, Bee::Ping data) {
    (void)portNum;
    (void)data;
    this->pollBoard();
}

bool WiiBoardManager ::queryBluetoothStatus(WiiBoardBluetoothStatus& status) {
    status.paired = false;
    status.trusted = false;
    status.connected = false;

    std::string command = std::string("bluetoothctl info ") + BOARD_ADDRESS;
    FILE* pipe = ::popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return false;
    }

    char buffer[256] = {0};
    while (::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        if (std::strstr(buffer, "Paired: yes") != nullptr) {
            status.paired = true;
        } else if (std::strstr(buffer, "Trusted: yes") != nullptr) {
            status.trusted = true;
        } else if (std::strstr(buffer, "Connected: yes") != nullptr) {
            status.connected = true;
        }
    }

    (void)::pclose(pipe);
    return true;
}

bool WiiBoardManager ::sendBluetoothCommands(const char* const* commands, std::size_t count) {
    FILE* pipe = ::popen("bluetoothctl", "w");
    if (pipe == nullptr) {
        return false;
    }

    bool writeOk = true;
    for (std::size_t index = 0; index < count; ++index) {
        if (::fprintf(pipe, "%s\n", commands[index]) < 0) {
            writeOk = false;
            break;
        }
    }

    (void)::fprintf(pipe, "quit\n");
    int closeStatus = ::pclose(pipe);
    return writeOk && closeStatus != -1;
}

bool WiiBoardManager ::attemptPairing(const std::string& trustCommand,
                                      const std::string& pairCommand,
                                      const std::string& connectCommand) {
    FILE* pipe = ::popen("bluetoothctl", "w");
    if (pipe == nullptr) {
        return false;
    }

    // Bring the board into bluetoothd's known-device cache first - `trust`/
    // `pair` fail instantly with "not available" against a device that
    // hasn't been (re)discovered, they don't just wait/retry on their own.
    bool writeOk = ::fprintf(pipe, "scan on\n") >= 0;
    ::fflush(pipe);
    ::sleep(DISCOVERY_SCAN_SECONDS);
    writeOk = writeOk && ::fprintf(pipe, "scan off\n") >= 0;
    ::fflush(pipe);

    writeOk = writeOk && ::fprintf(pipe, "%s\n", trustCommand.c_str()) >= 0;
    writeOk = writeOk && ::fprintf(pipe, "%s\n", pairCommand.c_str()) >= 0;
    ::fflush(pipe);
    // Give bluetoothd time to actually complete the legacy pairing
    // handshake with the board before anything tells bluetoothctl to quit.
    ::sleep(PAIRING_HANDSHAKE_SECONDS);

    writeOk = writeOk && ::fprintf(pipe, "%s\n", connectCommand.c_str()) >= 0;
    ::fflush(pipe);
    ::sleep(PAIRING_CONNECT_SETTLE_SECONDS);

    (void)::fprintf(pipe, "quit\n");
    int closeStatus = ::pclose(pipe);
    return writeOk && closeStatus != -1;
}

bool WiiBoardManager ::attemptConnectOnly(const std::string& connectCommand) {
    FILE* pipe = ::popen("bluetoothctl", "w");
    if (pipe == nullptr) {
        return false;
    }

    bool writeOk = ::fprintf(pipe, "%s\n", connectCommand.c_str()) >= 0;
    ::fflush(pipe);
    ::sleep(PAIRING_CONNECT_SETTLE_SECONDS);

    (void)::fprintf(pipe, "quit\n");
    int closeStatus = ::pclose(pipe);
    return writeOk && closeStatus != -1;
}

void WiiBoardManager ::runBluetoothAttempt(bool alreadyPaired) {
    const std::string connectCommand = std::string("connect ") + BOARD_ADDRESS;

    if (alreadyPaired) {
        (void)this->attemptConnectOnly(connectCommand);
    } else {
        const std::string trustCommand = std::string("trust ") + BOARD_ADDRESS;
        const std::string pairCommand = std::string("pair ") + BOARD_ADDRESS;
        (void)this->attemptPairing(trustCommand, pairCommand, connectCommand);
    }

    this->m_bluetoothAttemptInProgress = false;
}

void WiiBoardManager ::notifyConnectedIfNeeded() {
    if (!this->m_connectionEventRaised) {
        this->log_ACTIVITY_HI_BluetoothConnected();
        this->m_connectionEventRaised = true;
    }
}

void WiiBoardManager ::startConnectedSession() {
    this->m_sessionActive = true;
    this->m_archivePending = false;
    this->m_connectedSecondsRemaining = CONNECTED_SESSION_SECONDS;
    this->m_sessionSampleCount = 0U;
    this->m_sessionSamples.fill(0.0f);
    this->log_ACTIVITY_HI_BluetoothSessionStarted();
}

void WiiBoardManager ::captureSessionSample() {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    if (!this->m_sessionActive || this->m_archivePending || this->m_sessionSampleCount >= SESSION_SAMPLE_CAPACITY) {
        return;
    }

    this->m_sessionSamples[this->m_sessionSampleCount] = this->m_lastWeightKg;
    ++this->m_sessionSampleCount;
}

void WiiBoardManager ::requestSessionArchive() {
    if (this->m_archivePending || this->m_sessionSampleCount == 0U) {
        return;
    }

    if (!this->isConnected_productRequestOut_OutputPort(0) || !this->isConnected_productSendOut_OutputPort(0)) {
        return;
    }

    const FwSizeType dpSize = Components::WiiBoardManager_WeightSession::SERIALIZED_SIZE + sizeof(FwDpIdType);
    this->m_archivePending = true;
    this->dpRequest_WeightSessionContainer(dpSize);
}

void WiiBoardManager ::disconnectBoard() {
    this->log_ACTIVITY_HI_BluetoothAutoDisconnect();

    const std::string disconnectCommand = std::string("disconnect ") + BOARD_ADDRESS;
    const char* commands[] = {disconnectCommand.c_str()};
    (void)this->sendBluetoothCommands(commands, 1U);

    this->requestSessionArchive();
    this->log_WARNING_HI_BluetoothDisconnected();
    this->closeBoard();
}

void WiiBoardManager ::dpRecv_WeightSessionContainer_handler(DpContainer& container, Fw::Success::T status) {
    if (status != Fw::Success::SUCCESS) {
        this->m_archivePending = false;
        return;
    }

    this->m_sessionDpContainer = container;
    this->m_sessionDpContainer.setPriority(FwDpPriorityType(10));

    Components::WiiBoardManager_WeightSessionSamples samples;
    for (U32 i = 0; i < SESSION_SAMPLE_CAPACITY; ++i) {
        samples[i] = this->m_sessionSamples[i];
    }

    Components::WiiBoardManager_WeightSession session(this->m_sessionSampleCount, samples);
    Fw::SerializeStatus serializeStatus = this->m_sessionDpContainer.serializeRecord_WeightSessionRecord(session);
    FW_ASSERT(serializeStatus == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(serializeStatus));

    this->dpSend(this->m_sessionDpContainer);
    this->m_archivePending = false;
    this->m_sessionSampleCount = 0U;
    this->m_sessionSamples.fill(0.0f);
}

void WiiBoardManager ::maintainBluetoothConnection() {
    if (this->m_boardFd >= 0 || this->m_archivePending) {
        return;
    }

    this->log_ACTIVITY_HI_BluetoothReconnectAttempt();

    // Resolve once, at first use after boot, whether the board already came
    // paired/trusted (bonding persists in bluetoothd's own storage across
    // reboots). That one-time result decides which mode this deployment
    // stays in for the rest of its runtime: keep running the full
    // trust/pair/connect setup sequence, or just keep trying to connect to
    // an already-known device.
    if (!this->m_bootPairingModeResolved) {
        WiiBoardBluetoothStatus status{};
        bool statusKnown = this->queryBluetoothStatus(status);
        this->m_boardPairedAtBoot = statusKnown && status.paired && status.trusted;
        this->m_bootPairingModeResolved = true;
    }

    // A background attempt (scan/pair/connect, up to ~20s) may already be
    // in flight from a previous tick - don't pile another one on top of it.
    if (this->m_bluetoothAttemptInProgress.load()) {
        return;
    }

    // Only kick off a fresh attempt once any prior one has had its cooldown
    // period to actually resolve.
    if (this->m_pairingRetryCooldown > 0U) {
        --this->m_pairingRetryCooldown;
        return;
    }
    this->m_pairingRetryCooldown = PAIRING_RETRY_COOLDOWN_TICKS;

    if (this->m_bluetoothWorker.joinable()) {
        this->m_bluetoothWorker.join();
    }

    if (this->m_boardPairedAtBoot) {
        this->log_ACTIVITY_HI_BluetoothConnectAttempt();
    } else {
        this->log_ACTIVITY_HI_BluetoothSetupAttempt();
    }

    // The scan/pair/connect sequence blocks on real over-the-air wall time
    // (up to ~20s); run it on a background thread instead of this rate
    // group's thread so it can't stall telemetry, file downlink, etc.
    this->m_bluetoothAttemptInProgress = true;
    this->m_bluetoothWorker = std::thread(&WiiBoardManager::runBluetoothAttempt, this, this->m_boardPairedAtBoot);
}

bool WiiBoardManager ::openBoard() {
    DIR* dir = ::opendir("/dev/input");
    if (dir == nullptr) {
        return false;
    }

    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        if (std::strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        std::string candidatePath = std::string("/dev/input/") + entry->d_name;
        int fd = ::open(candidatePath.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }

        char deviceName[256] = {0};
        if (::ioctl(fd, EVIOCGNAME(sizeof(deviceName)), deviceName) >= 0 &&
            std::strstr(deviceName, BOARD_NAME) != nullptr) {
            this->m_boardFd = fd;
            this->m_boardPath = candidatePath;
            this->startConnectedSession();
            this->notifyConnectedIfNeeded();
            ::closedir(dir);
            return true;
        }

        ::close(fd);
    }

    ::closedir(dir);
    return false;
}

void WiiBoardManager ::closeBoard() {
    if (this->m_boardFd >= 0) {
        ::close(this->m_boardFd);
        this->m_boardFd = -1;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        this->m_sensorValues.clear();
        this->m_lastWeightKg = 0.0f;
    }

    this->m_boardPath.clear();
    this->m_connectionEventRaised = false;
    this->m_sessionActive = false;
    this->m_connectedSecondsRemaining = 0U;
}

void WiiBoardManager ::processAbsEvent(unsigned int code, int value) {
    {
        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        this->m_sensorValues[code] = value;
        this->m_weightDirty = true;

        I32 rawWeight = 0;
        for (const auto& sensorEntry : this->m_sensorValues) {
            rawWeight += sensorEntry.second;
        }

        F32 rawWeightKg = static_cast<F32>(rawWeight) / 100.0f;
        this->m_lastWeightKg = rawWeightKg;
    }
}

void WiiBoardManager ::emitWeight() {
    F32 weightKg = 0.0f;

    {
        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        if (!this->m_weightDirty) {
            return;
        }

        weightKg = this->m_lastWeightKg;
        this->m_weightDirty = false;
    }

    if (this->isConnected_weightOut_OutputPort(0)) {
        this->weightOut_out(0, weightKg);
    }
}

void WiiBoardManager ::handleConnectionLost() {
    this->requestSessionArchive();
    this->log_WARNING_HI_BluetoothConnectionLost();
    this->log_WARNING_HI_BluetoothDisconnected();
    this->closeBoard();
}

void WiiBoardManager ::pollBoard() {
    if (this->m_boardFd < 0) {
        this->maintainBluetoothConnection();

        if (!this->openBoard()) {
            return;
        }
    }

    if (this->m_boardFd < 0) {
        return;
    }

    while (true) {
        struct input_event event;
        ssize_t bytesRead = ::read(this->m_boardFd, &event, sizeof(event));

        if (bytesRead == static_cast<ssize_t>(sizeof(event))) {
            if (event.type == EV_ABS) {
                this->processAbsEvent(event.code, event.value);
            } else if (event.type == EV_SYN) {
                this->emitWeight();
            }
            continue;
        }

        if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            return;
        }

        this->handleConnectionLost();
        return;
    }
}

}  // namespace Components
