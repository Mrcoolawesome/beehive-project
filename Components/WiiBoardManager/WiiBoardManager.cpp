// ======================================================================
// \title  WiiBoardManager.cpp
// \author devin
// \brief  cpp file for WiiBoardManager component implementation class
// ======================================================================

#include "Components/WiiBoardManager/WiiBoardManager.hpp"

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

}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WiiBoardManager ::WiiBoardManager(const char* const compName)
        : WiiBoardManagerComponentBase(compName),
            m_boardFd(-1),
            m_lastWeightKg(0.0f) {}

WiiBoardManager ::~WiiBoardManager() { this->closeBoard(); }

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void WiiBoardManager ::pingIn_handler(FwIndexType portNum, Bee::Ping data) {
    this->pollBoard();
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

    this->m_boardPath.clear();
}

void WiiBoardManager ::processAbsEvent(unsigned int code, int value) {
    F32 weightKg = 0.0f;

    {
        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        this->m_sensorValues[code] = value;

        I32 rawWeight = 0;
        for (const auto& sensorEntry : this->m_sensorValues) {
            rawWeight += sensorEntry.second;
        }

        weightKg = static_cast<F32>(rawWeight) / 100.0f;
        this->m_lastWeightKg = weightKg;
    }

    if (this->isConnected_weightOut_OutputPort(0)) {
        this->weightOut_out(0, weightKg);
    }
}

void WiiBoardManager ::pollBoard() {
    if (this->m_boardFd < 0 && !this->openBoard()) {
        return;
    }

    while (true) {
        struct input_event event;
        ssize_t bytesRead = ::read(this->m_boardFd, &event, sizeof(event));

        if (bytesRead == static_cast<ssize_t>(sizeof(event))) {
            if (event.type == EV_ABS) {
                this->processAbsEvent(event.code, event.value);
            }
            continue;
        }

        if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            return;
        }

        this->closeBoard();
        return;
    }
}

}  // namespace Components
