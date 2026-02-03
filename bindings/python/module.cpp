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
#ifdef WITH_OAK_CAMERA
class PyOAKMaster {
public:
    PyOAKMaster() {
        master_ = OAKCameraBackend::create_master();
    }
    bool initialize(int width, int height, double fps) {
        CameraConfig config;
        config.width = width;
        config.height = height;
        config.fps = fps;
        return master_->initialize(config);
    }
    void start() { master_->start(); }
    void stop() { master_->stop(); }
    std::shared_ptr<OAKCameraBackend> get_ptr() { return master_; }
private:
    std::shared_ptr<OAKCameraBackend> master_;
};
#endif

class PyPipeline {
public:
    // Standard constructor
    PyPipeline(const std::string& output_dir, const std::string& session_name,
               const std::string& backend_name = "oak",
               int width = 1920, int height = 1080, double fps = 30.0,
               int device_id = 0, bool append = false) {
        
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
        } else {
#ifdef WITH_FFMPEG
            camera_ = std::make_unique<FFmpegCameraBackend>();
#else
            throw std::runtime_error("Webcam support not compiled");
#endif
        }

        if (!camera_->initialize(cam_config)) {
            throw std::runtime_error("Failed to initialize camera backend");
        }
        
        setup(output_dir, session_name, backend_name, width, height, fps, append);
    }

#ifdef WITH_OAK_CAMERA
    // Proxy constructor for OAK-4P
    PyPipeline(const std::string& output_dir, const std::string& session_name,
               std::shared_ptr<PyOAKMaster> master, int socket_index,
               int width, int height, double fps, bool append = false) {
        
        camera_ = master->get_ptr()->create_proxy(socket_index);
        setup(output_dir, session_name, "oak_proxy", width, height, fps, append);
    }
#endif

private:
    void setup(const std::string& output_dir, const std::string& session_name, 
               const std::string& backend, int w, int h, double fps, bool append) {
        SessionConfig config;
        config.output_dir = output_dir;
        config.session_name = session_name;
        config.width = w;
        config.height = h;
        config.fps = fps;
        config.camera_backend_name = backend;
        config.append = append;
        pipeline_ = std::make_unique<IngestionPipeline>(std::move(camera_), config);
    }
public:
    
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
#ifdef WITH_OAK_CAMERA
    py::class_<micecam::PyOAKMaster, std::shared_ptr<micecam::PyOAKMaster>>(m, "OAKMaster")
        .def(py::init<>())
        .def("initialize", &micecam::PyOAKMaster::initialize,
             py::arg("width"), py::arg("height"), py::arg("fps"))
        .def("start", &micecam::PyOAKMaster::start)
        .def("stop", &micecam::PyOAKMaster::stop);
#endif

    py::class_<micecam::PyPipeline>(m, "Pipeline")
        .def(py::init<const std::string&, const std::string&, const std::string&, int, int, double, int, bool>(),
             py::arg("output_dir"),
             py::arg("session_name"),
             py::arg("backend_name") = "oak",
             py::arg("width") = 1920,
             py::arg("height") = 1080,
             py::arg("fps") = 30.0,
             py::arg("device_id") = 0,
             py::arg("append") = false,
             "Create a camera pipeline")
#ifdef WITH_OAK_CAMERA
        .def(py::init<const std::string&, const std::string&, std::shared_ptr<micecam::PyOAKMaster>, int, int, int, double, bool>(),
             py::arg("output_dir"), py::arg("session_name"), py::arg("master"), py::arg("socket_index"),
             py::arg("width"), py::arg("height"), py::arg("fps"), py::arg("append") = false,
             "Create a camera pipeline using an OAKMaster proxy")
#endif
        .def("start", &micecam::PyPipeline::start, "Start the pipeline")
        .def("stop", &micecam::PyPipeline::stop, "Stop the pipeline")
        .def("attach_callback", &micecam::PyPipeline::attach_callback,
             py::arg("callback"),
             "Attach callback: (data: bytes, seq: int, timestamp: float)")
        .def("get_stats", &micecam::PyPipeline::get_stats)
        .def("is_running", &micecam::PyPipeline::is_running)
        .def("get_capabilities", [](micecam::PyPipeline& self) {
            py::dict caps;
            caps["resolutions"] = std::vector<std::string>{"1920x1080", "1280x720", "1280x800"};
            caps["fps"] = std::vector<int>{30, 60};
            return caps;
        })
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
