#include "jpeg_codec.h"
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    const int width = 640;
    const int height = 480;
    std::vector<uint8_t> img(static_cast<size_t>(width) * height * 3);

    // Grauer Hintergrund mit leichtem Rauschen
    std::srand(42);
    for (int i = 0; i < width * height; ++i) {
        uint8_t base = 120 + (std::rand() % 20);
        img[i * 3 + 0] = base;
        img[i * 3 + 1] = base;
        img[i * 3 + 2] = base;
    }

    // Grosses rotes Objekt
    const int cx = 420, cy = 260, radius = 70;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                const int idx = (y * width + x) * 3;
                img[idx + 0] = 200;
                img[idx + 1] = 20;
                img[idx + 2] = 20;
            }
        }
    }

    // Kleineres rotes Objekt - testet, ob wirklich das GROESSTE gefunden wird
    const int cx2 = 100, cy2 = 100, radius2 = 25;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int dx = x - cx2;
            const int dy = y - cy2;
            if (dx * dx + dy * dy <= radius2 * radius2) {
                const int idx = (y * width + x) * 3;
                img[idx + 0] = 190;
                img[idx + 1] = 25;
                img[idx + 2] = 15;
            }
        }
    }

    const char* outPath = "test_images/sample_red.jpg";
    if (!saveJpegRGB(outPath, img, width, height, 90)) {
        std::cerr << "Konnte Testbild nicht schreiben\n";
        return 1;
    }

    std::cout << "Testbild erzeugt: " << outPath << " (" << width << "x" << height << ")\n";
    std::cout << "Grosses rotes Objekt bei ca. (" << cx << "," << cy << "), Radius " << radius << "\n";
    std::cout << "Kleines rotes Objekt bei ca. (" << cx2 << "," << cy2 << "), Radius " << radius2 << "\n";
    return 0;
}
