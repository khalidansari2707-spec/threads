#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include "api_server.hpp"

static multicorex::ApiServer* g_server = nullptr;

void handle_signal(int signal) {
    std::cout << "\n[MultiCoreX] Caught signal " << signal << ", initiating graceful shutdown...\n";
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string web_dir = "frontend";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--web" && i + 1 < argc) {
            web_dir = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "MultiCoreX Backend Server\n";
            std::cout << "Usage: multicorex_server [options]\n";
            std::cout << "  --host <ip>      Host to bind (default: 127.0.0.1)\n";
            std::cout << "  --port <port>    Port to listen on (default: 8080)\n";
            std::cout << "  --web  <path>    Path to frontend directory (default: frontend)\n";
            return 0;
        }
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "========================================================\n";
    std::cout << "   __  __       _ _   _  _____              __   __ \n";
    std::cout << "  |  \\/  |     | | | (_)/ ____|             \\ \\ / / \n";
    std::cout << "  | \\  / |_   _| | |_ _| |     ___  _ __ ___ \\ V /  \n";
    std::cout << "  | |\\/| | | | | | __| | |    / _ \\| '__/ _ \\ > <   \n";
    std::cout << "  | |  | | |_| | | |_| | |___| (_) | | |  __// . \\  \n";
    std::cout << "  |_|  |_|\\__,_|_|\\__|_|\\_____\\___/|_|  \\___/_/ \\_\\ \n";
    std::cout << "========================================================\n";
    std::cout << " Multithreading Visualizer & Performance Analysis System\n";
    std::cout << " Operating System Principles: Concurrency & Synchronization\n";
    std::cout << "========================================================\n\n";

    try {
        multicorex::ApiServer server(host, port, web_dir);
        g_server = &server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << "\n";
        return 1;
    }

    std::cout << "[MultiCoreX] Server exited successfully.\n";
    return 0;
}
