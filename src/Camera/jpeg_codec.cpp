#include "jpeg_codec.h"
#include <jpeglib.h>
#include <csetjmp>
#include <cstdio>
#include <iostream>

namespace {

struct JpegErrorMgr {
    jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
};

void jpegErrorExit(j_common_ptr cinfo) {
    auto* err = reinterpret_cast<JpegErrorMgr*>(cinfo->err);
    (*cinfo->err->output_message)(cinfo);
    std::longjmp(err->setjmpBuffer, 1);
}

} // namespace

bool loadJpegAsRGB(const std::string& path,
                   std::vector<uint8_t>& outData,
                   int& outWidth, int& outHeight) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        std::cerr << "[jpeg_codec] Konnte Datei nicht oeffnen: " << path << "\n";
        return false;
    }

    jpeg_decompress_struct cinfo;
    JpegErrorMgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.setjmpBuffer)) {
        jpeg_destroy_decompress(&cinfo);
        std::fclose(file);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    outWidth = static_cast<int>(cinfo.output_width);
    outHeight = static_cast<int>(cinfo.output_height);
    const int channels = cinfo.output_components; // erwartungsgemaess 3

    outData.resize(static_cast<size_t>(outWidth) * outHeight * channels);
    const int rowStride = outWidth * channels;

    std::vector<JSAMPROW> rowPointer(1);
    while (cinfo.output_scanline < cinfo.output_height) {
        rowPointer[0] = &outData[static_cast<size_t>(cinfo.output_scanline) * rowStride];
        jpeg_read_scanlines(&cinfo, rowPointer.data(), 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    std::fclose(file);
    return true;
}

bool saveJpegRGB(const std::string& path,
                 const std::vector<uint8_t>& data,
                 int width, int height, int quality) {
    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        std::cerr << "[jpeg_codec] Konnte Datei nicht zum Schreiben oeffnen: " << path << "\n";
        return false;
    }

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, file);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);
    const int rowStride = width * 3;
    std::vector<JSAMPROW> rowPointer(1);
    while (cinfo.next_scanline < cinfo.image_height) {
        // libjpeg verlangt einen nicht-const Pointer, obwohl es nur liest.
        rowPointer[0] = const_cast<JSAMPROW>(&data[static_cast<size_t>(cinfo.next_scanline) * rowStride]);
        jpeg_write_scanlines(&cinfo, rowPointer.data(), 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    std::fclose(file);
    return true;
}
