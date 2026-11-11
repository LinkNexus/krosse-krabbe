#include "capture.h"
#include "jpeg_codec.h"
#include <iostream>

CapturedImage captureImage() {
    CapturedImage img;

    const char* testImagePath = "test_images/sample_red.jpg";

    if (!loadJpegAsRGB(testImagePath, img.data, img.width, img.height)) {
        std::cerr << "Load failed. Abbruch.\n";
        return img; 
    }

    img.channels = 3;
    img.success = true;
    return img;
}
