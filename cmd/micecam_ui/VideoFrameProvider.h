#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include "micecam/types.h"
#include "domain/frame.h"

namespace micecam { class IngestionPipeline; }

// Acts as a Qt Image Provider (for QML) and pulls frames from the pipeline
class VideoFrameProvider : public QQuickImageProvider {
public:
    VideoFrameProvider();
    ~VideoFrameProvider() override;

    void setPipeline(micecam::IngestionPipeline* pipeline);
    void setPreviewImage(const QImage& image);

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    void worker_loop();

    QImage m_currentImage;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    micecam::IngestionPipeline* m_pipeline = nullptr;

    // Async processing state
    std::thread m_workerThread;
    std::atomic<bool> m_running{true};
};
