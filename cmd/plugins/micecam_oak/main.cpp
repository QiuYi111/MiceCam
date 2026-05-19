#include <atomic>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "OAKPluginServer.h"

static std::unique_ptr<grpc::Server> g_server;
static std::atomic<bool> g_shutdown_requested{false};

static void signal_handler(int) {
    g_shutdown_requested.store(true);
}

#ifdef _WIN32
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT ||
        ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_SHUTDOWN_EVENT) {
        g_shutdown_requested.store(true);
        return TRUE;
    }
    return FALSE;
}
#endif

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--port=PORT]\n\n"
              << "Options:\n"
              << "  --port=PORT       gRPC listen port (default: 50052, env: MiceCAM_PLUGIN_PORT)\n"
              << "  --help, -h        Print this help\n";
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    std::string port = "50052";

    const char* env_port = std::getenv("MiceCAM_PLUGIN_PORT");
    if (env_port && env_port[0] != '\0') {
        port = env_port;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        if (arg.starts_with("--port=")) {
            port = arg.substr(7);
        }
    }

    spdlog::info("MiceCam OAK Plugin v0.1.0 starting on port {}", port);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#endif

    micecam::plugin::OAKPluginServer service;
    grpc::ServerBuilder builder;
    std::string listen_addr = "0.0.0.0:" + port;
    builder.AddListeningPort(listen_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    g_server = builder.BuildAndStart();
    if (!g_server) {
        spdlog::critical("Failed to start gRPC server on {}", listen_addr);
        return 1;
    }

    spdlog::info("gRPC server listening on {}", listen_addr);

    std::thread shutdown_watcher([] {
        while (!g_shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        spdlog::info("Shutdown requested");
        g_server->Shutdown();
    });

    g_server->Wait();
    shutdown_watcher.join();

    spdlog::info("Server shutdown complete");
    return 0;
}
