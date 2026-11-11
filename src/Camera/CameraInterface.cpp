#include "CameraInterface.h"
#include "capture.h"
#include "image_analysis.h"
#include <chrono>
#include <iostream>

namespace {

BallPosition whereIsObject(const RedObjectResult& result,
                           const CapturedImage& img,
                           int pixelTolerance) {
    /*
    if((img.height/2) - pixelTolerance > result.centerY){
        //OBJECT IS HIGHER
    }else if((img.height/2) + pixelTolerance < result.centerY){
        //OBJECT IS LOWER
    }
    */
    if ((img.width / 2) + pixelTolerance < result.centerX) {
        //OBJECT IS RIGHT
        return BallPosition::Right;
    } else if ((img.width / 2) - pixelTolerance > result.centerX) {
        //OBJECT IS LEFT
        return BallPosition::Left;
    } else {
        return BallPosition::Center;
    }
}

} // namespace

BallPosition fetchImageData() {
    CapturedImage img = captureImage();

    if (!img.success) {
        std::cerr << "Bildaufnahme fehlgeschlagen. Abbruch.\n";
        return BallPosition::ErrorCam;
    }

    RedObjectResult result = findLargestRedObject(img.data, img.width, img.height, RedThreshold());

    if (result.found) {
        return whereIsObject(result, img, 50);
    }

    return BallPosition::NotVisible;
}

CameraBallDetector::CameraBallDetector()
    : m_captureThread(&CameraBallDetector::captureLoop, this) {}

CameraBallDetector::~CameraBallDetector() {
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_running = false;
        m_active = false;
    }
    m_stateChanged.notify_all();

    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
}

BallPosition CameraBallDetector::detect() {
    return m_latestPosition.load(std::memory_order_acquire);
}

void CameraBallDetector::setActive(bool active) {
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_active == active) {
            return;
        }

        m_active = active;
        m_latestPosition.store(BallPosition::ErrorCam,
                               std::memory_order_release);
    }
    m_stateChanged.notify_all();
}

void CameraBallDetector::captureLoop() {
    constexpr auto framePause = std::chrono::milliseconds(50);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_stateMutex);
            m_stateChanged.wait(lock, [this] {
                return !m_running || m_active;
            });
            if (!m_running) {
                return;
            }
        }

        m_latestPosition.store(fetchImageData(), std::memory_order_release);

        std::unique_lock<std::mutex> lock(m_stateMutex);
        m_stateChanged.wait_for(lock, framePause, [this] {
            return !m_running || !m_active;
        });
        if (!m_running) {
            return;
        }
    }
}
