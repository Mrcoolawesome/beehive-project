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

}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WiiBoardManager ::WiiBoardManager(const char* const compName)
        : WiiBoardManagerComponentBase(compName),
            m_boardFd(-1),
            m_lastWeightKg(0.0f),
            m_tareKg(3.80f),
            m_scaleFactor(1.30f),
            m_connectionEventRaised(false),
            m_sessionActive(false),
            m_archivePending(false),
            m_weightDirty(false),
            m_connectedSecondsRemaining(0U),
            m_sessionSamples{},
            m_sessionSampleCount(0U),
            m_sessionDpContainer() {}

WiiBoardManager ::~WiiBoardManager() { this->closeBoard(); }

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

void WiiBoardManager ::parameterUpdated(FwPrmIdType id) {
    switch (id) {
        case PARAMID_TAREKG: {
            Fw::ParamValid valid;
            this->m_tareKg = this->paramGet_tareKg(valid);
            break;
        }
        case PARAMID_SCALEFACTOR: {
            Fw::ParamValid valid;
            this->m_scaleFactor = this->paramGet_scaleFactor(valid);
            break;
        }
        default:
            break;
    }
}

void WiiBoardManager ::maintainBluetoothConnection() {
    if (this->m_boardFd >= 0 || this->m_archivePending) {
        return;
    }

    this->log_ACTIVITY_HI_BluetoothReconnectAttempt();

    WiiBoardBluetoothStatus status{};
    bool statusKnown = this->queryBluetoothStatus(status);

    const std::string trustCommand = std::string("trust ") + BOARD_ADDRESS;
    const std::string pairCommand = std::string("pair ") + BOARD_ADDRESS;
    const std::string connectCommand = std::string("connect ") + BOARD_ADDRESS;

    if (statusKnown && status.paired && status.trusted) {
        this->log_ACTIVITY_HI_BluetoothConnectAttempt();
        const char* commands[] = {connectCommand.c_str()};
        (void)this->sendBluetoothCommands(commands, 1U);
        return;
    }

    this->log_ACTIVITY_HI_BluetoothSetupAttempt();
    const char* commands[] = {trustCommand.c_str(), pairCommand.c_str(), connectCommand.c_str()};
    (void)this->sendBluetoothCommands(commands, 3U);
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
        F32 calibratedWeightKg = (rawWeightKg - this->m_tareKg) * this->m_scaleFactor;
        if (calibratedWeightKg < 0.0f) {
            calibratedWeightKg = 0.0f;
        }

        this->m_lastWeightKg = calibratedWeightKg;
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
