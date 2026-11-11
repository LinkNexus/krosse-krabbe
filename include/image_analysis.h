#pragma once
#include <cstdint>
#include <vector>

struct RedObjectResult {
    bool found = false;
    int centerX = 0;
    int centerY = 0;
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    int pixelCount = 0;
};

struct RedThreshold {
    int minRed = 100;    
    int maxGreenBlue = 90;  
    int minDominance = 30;  
};

RedObjectResult findLargestRedObject(const std::vector<uint8_t>& rgbData, int width, int height,
                                     const RedThreshold& threshold = RedThreshold());
