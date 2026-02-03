/**
 * @file module.cpp
 * @brief Pybind11 bindings for MiceCam SDK
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "micecam/types.h"
#include "micecam/observer.h"
#include "micecam/pipeline/frame_dispatcher.h"
#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/camera_backend.h"

#ifdef WITH_OAK_CAMERA
#include "micecam/camera/oak_camera_backend.h"
#endif

#ifdef WITH_CAMERA_BACKEND
#include "micecam/camera/ffmpeg_camera_backend.h"
#endif

namespace py = pybind11;

namespace micecam {

/**
 * @brief Python wrapper for IngestionPipeline
 */
class PyPipeline {
public:
    PyPipeline(const std::string& output_dir, const std::string& session_name,
               const std::string& backend_name = "oak",
               int width = 1920, int height = 1080, double fps = 30.0,
               int device_id = 0) {
        
        CameraConfig cam_config;
        cam_config.width = width;
        cam_config.height = height;
        cam_config.fps = fps;
        cam_config.device_id = device_id;

        if (backend_name == "oak") {
#ifdef WITH_OAK_CAMERA
            camera_ = std::make_unique<OAKCameraBackend>();
#else
            throw std::runtime_error("OAK camera support not compiled");
#endif
        } else if (backend_name == "usb" || backend_name == "ffmpeg") {
#ifdef WITH_FFMPEG
            camera_ = std::make_unique<FFmpegCameraBackend>();
#else
            throw std::runtime_error("FFmpeg/USB camera support not compiled");
#endif
        } else {
            throw std::runtime_error("Unknown camera backend: " + backend_name);
        }

        if (!camera_->initialize(cam_config)) {
            throw std::runtime_error("Failed to initialize camera backend: " + backend_name);
        }
        
        // Setup session config
        SessionConfig config;
        config.output_dir = output_dir;
        config.session_name = session_name;
        config.ring_buffer_size = 256; // Larger buffer for 4K
        config.enable_checksums = true;
        config.width = width;
        config.height = height;
        config.fps = fps;
        config.camera_backend_name = backend_name;
        
        pipeline_ = std::make_unique<IngestionPipeline>(std::move(camera_), config);
        std::cout << "[PyPipeline] Created " << backend_name << " pipeline: " 
                  << width << "x" << height << " @ " << fps << " fps\n";
    }
    
    void start() {
        py::gil_scoped_release release;
        if (!pipeline_->start()) {
            throw std::runtime_error("Failed to start pipeline");
        }
        std::cout << "[PyPipeline] Started\n";
    }
    
    void stop() {
        py::gil_scoped_release release;
        pipeline_->stop();
        std::cout << "[PyPipeline] Stopped\n";
    }
    
    void attach_callback(std::function<void(py::bytes, uint64_t, double)> callback) {
        class PyCallback : public IFrameObserver {
        public:
            std::function<void(py::bytes, uint64_t, double)> cb_;
            explicit PyCallback(std::function<void(py::bytes, uint64_t, double)> cb) : cb_(std::move(cb)) {}
            void on_frame(const FrameView& frame) override {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes data(reinterpret_cast<const char*>(frame.data), frame.size);
                    cb_(data, frame.sequence_id, frame.timestamp);
                } catch (const std::exception& e) {
                    std::cerr << "[PyCallback] Error: " << e.what() << "\n";
                }
            }
        };
        
        auto observer = std::make_shared<PyCallback>(std::move(callback));
        observers_.push_back(observer);
        pipeline_->attach_observer(observer);
    }
    
    py::dict get_stats() const {
        auto stats = pipeline_->get_stats();
        py::dict result;
        result["captured_frames"] = stats.captured_frames;
        result["dropped_frames"] = stats.dropped_frames;
        result["drop_rate"] = stats.drop_rate;
        result["throughput_mbps"] = stats.current_throughput_mbps;
        result["pending_buffer"] = stats.pending_buffer_size;
        return result;
    }
    
    bool is_running() const { return pipeline_->is_running(); }
    
private:
    std::unique_ptr<ICameraBackend> camera_;
    std::unique_ptr<IngestionPipeline> pipeline_;
    std::vector<std::shared_ptr<IFrameObserver>> observers_;
};

} // namespace micecam

PYBIND11_MODULE(_micecam, m) {
    m.doc() = "MiceCam SDK Python bindings";
    m.attr("__version__") = "1.0.0";
    
    // PixelFormat enum
    py::enum_<micecam::PixelFormat>(m, "PixelFormat")
        .value("MJPEG", micecam::PixelFormat::MJPEG)
        .value("RGB24", micecam::PixelFormat::RGB24)
        .value("MONO8", micecam::PixelFormat::MONO8)
        .value("MONO16", micecam::PixelFormat::MONO16)
        .value("NV12", micecam::PixelFormat::NV12)
        .export_values();
    
    // Pipeline class
    py::class_<micecam::PyPipeline>(m, "Pipeline")
        .def(py::init<const std::string&, const std::string&, const std::string&, int, int, double, int>(),
             py::arg("output_dir"),
             py::arg("session_name"),
             py::arg("backend_name") = "oak",
             py::arg("width") = 1920,
             py::arg("height") = 1080,
             py::arg("fps") = 30.0,
             py::arg("device_id") = 0,
             "Create a camera pipeline")
        .def("start", &micecam::PyPipeline::start, "Start the pipeline")
        .def("stop", &micecam::PyPipeline::stop, "Stop the pipeline")
        .def("attach_callback", &micecam::PyPipeline::attach_callback,
             py::arg("callback"),
             "Attach callback: (data: bytes, seq: int, timestamp: float)")
        .def("get_stats", &micecam::PyPipeline::get_stats)
        .def("is_running", &micecam::PyPipeline::is_running)
        .def("__enter__", [](micecam::PyPipeline& self) -> micecam::PyPipeline& {
            self.start();
            return self;
        })
        .def("__exit__", [](micecam::PyPipeline& self, py::object, py::object, py::object) {
            self.stop();
        });
    
    m.def("has_oak_support", []() {
#ifdef WITH_OAK_CAMERA
        return true;
#else
        return false;
#endif
    });

    m.def("has_webcam_support", []() {
#ifdef WITH_CAMERA_BACKEND
        return true;
#else
        return false;
#endif
    });
}
