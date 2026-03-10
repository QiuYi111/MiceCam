#include "VideoFrameProvider.h"
#include <QDebug>
#include <cstring>
#include <opencv2/opencv.hpp>
#include "micecam/types.h"
#include "micecam/pipeline/ingestion_pipeline.h"

VideoFrameProvider::VideoFrameProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Initialize with a black placeholder image
    m_currentImage = QImage(640, 480, QImage::Format_RGB888);
    m_currentImage.fill(Qt::black);

    // Start worker thread
    m_workerThread = std::thread(&VideoFrameProvider::worker_loop, this);
}

VideoFrameProvider::~VideoFrameProvider() {
    m_running = false;
    m_cv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void VideoFrameProvider::setPipeline(micecam::IngestionPipeline* pipeline) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pipeline = pipeline;
    m_cv.notify_one();
}

QImage VideoFrameProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    Q_UNUSED(id);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (size) {
        *size = m_currentImage.size();
    }

    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return m_currentImage.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);
    }

    return m_currentImage;
}

// VideoFrameProvider::on_frame removed for Zero-Drop pull model

void VideoFrameProvider::worker_loop() {
    while (m_running) {
        std::unique_ptr<micecam::Frame> frame;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // Polling interval if pipeline is null, or wait for notification
            if (!m_pipeline) {
                m_cv.wait_for(lock, std::chrono::milliseconds(100), [this] { return !m_running || m_pipeline; });
            }

            if (!m_running) break;

            if (m_pipeline) {
                frame = m_pipeline->get_preview_frame();
            }
        }

        if (!frame) {
            // Throttled pull frequency (~30 FPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            continue;
        }

        // Perform expensive conversion outside the capture thread
        QImage nextImage;

        if (frame->format == micecam::PixelFormat::MONO8) {
            nextImage = QImage(frame->width, frame->height, QImage::Format_Grayscale8);
            std::memcpy(nextImage.bits(), frame->data->data(), frame->data->size());
        }
        else if (frame->format == micecam::PixelFormat::UYVY422) {
            cv::Mat yuv(frame->height, frame->width, CV_8UC2, (void*)frame->data->data());
            cv::Mat rgb;
            cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB_UYVY);
            nextImage = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
        }
        else if (frame->format == micecam::PixelFormat::RGB24) {
            nextImage = QImage(frame->width, frame->height, QImage::Format_RGB888);
            std::memcpy(nextImage.bits(), frame->data->data(), frame->data->size());
        }
        else {
            // Try MJPEG
            nextImage.loadFromData(frame->data->data(), frame->data->size(), "JPEG");
        }

        if (!nextImage.isNull()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentImage = std::move(nextImage);
        }

        // Throttled pull frequency (~30 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
