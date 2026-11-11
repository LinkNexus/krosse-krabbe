#pragma once
#include <cstdint>
#include <vector>

struct CapturedImage {
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool success = false;
};

CapturedImage captureImage();
