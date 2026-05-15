#include <cstdlib>
#include <csignal>
#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "OAKPluginServer.h"

static std::unique_ptr<grpc::Server> g_server;

static void signal_handler(int sig) {
    spdlog::info("Received signal {}, shutting down...", sig);
    if (g_server) {
        g_server->Shutdown();
    }
}

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
    g_server->Wait();

    spdlog::info("Server shutdown complete");
    return 0;
}
