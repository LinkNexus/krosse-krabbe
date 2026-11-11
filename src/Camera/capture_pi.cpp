// Standard-Implementierung (siehe Makefile: USE_PI_CAMERA=1 ist Default,
// damit im Ernstfall auf dem Pi ohne Extra-Flag gebaut wird).
//
// Ruft rpicam-still auf (Teil von rpicam-apps, Standard auf aktuellem
// Raspberry Pi OS) und dekodiert das erzeugte JPEG per libjpeg direkt in
// ein RGB-Array im RAM.

#include "capture.h"
#include "jpeg_codec.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

CapturedImage captureImage() {
    CapturedImage img;

    const char* tmpPath = "/tmp/capture.jpg";

    // -n              : keine Vorschau
    // -o              : Ausgabedatei
    // --width/--height: niedriger Aufloesung
    // -t 200          : nur 200ms warten
    // -q 80           : JPEG-Qualitaet
    std::string cmd = "rpicam-still -n -o " + std::string(tmpPath) + " --width 640 --height 480 -t 200 -q 80 2>/tmp/capture_err.log";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        return img;
    }

    if (!loadJpegAsRGB(tmpPath, img.data, img.width, img.height)) {
        return img;
    }

    img.channels = 3;
    img.success = true;
    std::remove(tmpPath);
    return img;
}
