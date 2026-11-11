#pragma once

#include "BallPosition.h"
#include "IBallDetector.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

BallPosition fetchImageData();

class CameraBallDetector final : public IBallDetector {
public:
    CameraBallDetector();
    ~CameraBallDetector() override;

    CameraBallDetector(const CameraBallDetector&) = delete;
    CameraBallDetector& operator=(const CameraBallDetector&) = delete;

    BallPosition detect() override;
    void setActive(bool active);

private:
    void captureLoop();

    std::atomic<BallPosition> m_latestPosition{BallPosition::ErrorCam};
    std::mutex m_stateMutex;
    std::condition_variable m_stateChanged;
    bool m_running = true;
    bool m_active = false;
    std::thread m_captureThread;
};
