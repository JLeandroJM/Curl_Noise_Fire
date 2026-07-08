#include "FrameExporter.h"
#include <iostream>

#ifdef _WIN32
// If OpenCV is available, we would include it.
// For now, we stub it to allow compilation without OpenCV,
// or we can use stb_image_write. We'll use stb_image_write for PNGs.
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// We use an opaque pointer for OpenCV to avoid making it a strict dependency 
// in the header. If OpenCV is installed, we use cv::VideoWriter.
// For this stub, we just simulate video export or skip it if OpenCV isn't configured.
#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#define HAS_OPENCV 1
#else
#define HAS_OPENCV 0
#endif

struct FrameExporter::Impl {
#if HAS_OPENCV
    cv::VideoWriter videoWriter;
#endif
    bool isRecording = false;
};

FrameExporter::FrameExporter() : pimpl(std::make_unique<Impl>()) {}
FrameExporter::~FrameExporter() { finish(); }

bool FrameExporter::initVideoWriter(const std::string& filename, int width, int height, double fps) {
#if HAS_OPENCV
    // OpenCV expects BGR by default, but we'll feed it RGB and convert.
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    pimpl->videoWriter.open(filename, fourcc, fps, cv::Size(width, height), true);
    if (!pimpl->videoWriter.isOpened()) {
        std::cerr << "Error: No se pudo abrir VideoWriter de OpenCV para " << filename << "\n";
        return false;
    }
    pimpl->isRecording = true;
    return true;
#else
    std::cerr << "OpenCV no está disponible. Compila con OpenCV para exportar video.\n";
    return false;
#endif
}

void FrameExporter::addFrame(int width, int height) {
    if (!pimpl->isRecording) return;

    pixelBuffer.resize(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());

#if HAS_OPENCV
    // OpenGL's origin is bottom-left, OpenCV expects top-left.
    // So we flip vertically.
    cv::Mat img(height, width, CV_8UC3, pixelBuffer.data());
    cv::Mat flipped;
    cv::flip(img, flipped, 0); // vertical flip
    
    // OpenGL gives RGB, OpenCV expects BGR
    cv::Mat bgr;
    cv::cvtColor(flipped, bgr, cv::COLOR_RGB2BGR);

    pimpl->videoWriter.write(bgr);
#endif
}

void FrameExporter::finish() {
#if HAS_OPENCV
    if (pimpl->isRecording) {
        pimpl->videoWriter.release();
        pimpl->isRecording = false;
    }
#endif
}

bool FrameExporter::saveFrameAsPNG(const std::string& filename, int width, int height) {
    pixelBuffer.resize(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());

    // Flip vertically for stb_image_write
    int row_stride = width * 3;
    std::vector<uint8_t> flipped(pixelBuffer.size());
    for (int y = 0; y < height; ++y) {
        int src_y = y;
        int dst_y = height - 1 - y;
        memcpy(&flipped[dst_y * row_stride], &pixelBuffer[src_y * row_stride], row_stride);
    }

    int res = stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), row_stride);
    return res != 0;
}
