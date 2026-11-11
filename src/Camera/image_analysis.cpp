#include "image_analysis.h"

namespace {

inline bool isRedPixel(uint8_t r, uint8_t g, uint8_t b, const RedThreshold& t) {
    return r >= t.minRed &&
           g <= t.maxGreenBlue &&
           b <= t.maxGreenBlue &&
           (r - g) >= t.minDominance &&
           (r - b) >= t.minDominance;
}

} // namespace

RedObjectResult findLargestRedObject(const std::vector<uint8_t>& rgbData,
                                      int width, int height,
                                      const RedThreshold& threshold) {
    RedObjectResult best;

    if (width <= 0 || height <= 0 ||
        rgbData.size() < static_cast<size_t>(width) * height * 3) {
        return best; // ungueltige Eingabe -> found bleibt false
    }

    const int numPixels = width * height;

    // Schritt 1: Maske erzeugen (1 Byte pro Pixel statt 3 -> spart RAM
    // fuer die restliche Verarbeitung).
    std::vector<uint8_t> mask(numPixels, 0);
    for (int i = 0; i < numPixels; ++i) {
        const uint8_t r = rgbData[i * 3 + 0];
        const uint8_t g = rgbData[i * 3 + 1];
        const uint8_t b = rgbData[i * 3 + 2];
        mask[i] = isRedPixel(r, g, b, threshold) ? 1 : 0;
    }

    // Schritt 2: Connected Components per iterativem Flood-Fill.
    // Iterativ (mit eigenem Stack als std::vector) statt rekursiv,
    // damit auch grosse zusammenhaengende Flaechen keinen Stack-Overflow
    // verursachen koennen.
    std::vector<uint8_t> visited(numPixels, 0);
    std::vector<int> stack;
    stack.reserve(1024);

    for (int startIdx = 0; startIdx < numPixels; ++startIdx) {
        if (mask[startIdx] == 0 || visited[startIdx] == 1) {
            continue;
        }

        int count = 0;
        int minX = width, minY = height, maxX = 0, maxY = 0;
        long long sumX = 0, sumY = 0;

        stack.clear();
        stack.push_back(startIdx);
        visited[startIdx] = 1;

        while (!stack.empty()) {
            const int idx = stack.back();
            stack.pop_back();

            const int x = idx % width;
            const int y = idx / width;

            ++count;
            sumX += x;
            sumY += y;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;

            // 4 Nachbarn pruefen (links, rechts, oben, unten)
            if (x > 0) {
                const int n = idx - 1;
                if (mask[n] && !visited[n]) { visited[n] = 1; stack.push_back(n); }
            }
            if (x < width - 1) {
                const int n = idx + 1;
                if (mask[n] && !visited[n]) { visited[n] = 1; stack.push_back(n); }
            }
            if (y > 0) {
                const int n = idx - width;
                if (mask[n] && !visited[n]) { visited[n] = 1; stack.push_back(n); }
            }
            if (y < height - 1) {
                const int n = idx + width;
                if (mask[n] && !visited[n]) { visited[n] = 1; stack.push_back(n); }
            }
        }

        if (count > best.pixelCount) {
            best.found = true;
            best.pixelCount = count;
            best.minX = minX;
            best.minY = minY;
            best.maxX = maxX;
            best.maxY = maxY;
            best.centerX = static_cast<int>(sumX / count);
            best.centerY = static_cast<int>(sumY / count);
        }
    }

    return best;
}
