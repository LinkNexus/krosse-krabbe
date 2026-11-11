#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Duenner Wrapper um libjpeg, damit der Rest des Codes (capture_*.cpp,
// gen_test_image.cpp) nicht direkt mit der libjpeg-C-API arbeiten muss.

// Laedt eine JPEG-Datei und dekodiert sie zu RGB-Rohdaten (3 Byte pro Pixel).
// Gibt false zurueck bei Fehlern (Datei fehlt, korruptes JPEG, etc.);
// outData/outWidth/outHeight sind dann undefiniert.
bool loadJpegAsRGB(const std::string& path,
                   std::vector<uint8_t>& outData,
                   int& outWidth, int& outHeight);

// Schreibt RGB-Rohdaten (3 Byte pro Pixel) als JPEG-Datei.
// quality: 0-100, 90 ist ein guter Standardwert.
bool saveJpegRGB(const std::string& path,
                 const std::vector<uint8_t>& data,
                 int width, int height, int quality = 90);
