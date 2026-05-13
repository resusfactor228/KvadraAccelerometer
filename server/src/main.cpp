#include "server.hpp"
#include "config.hpp"
#include "logger.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>

static accel::Server* g_server = nullptr;

static void sig_handler(int) {
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    uint16_t port_a        = 8080;
    uint16_t port_b        = 8081;
    int      dup_precision = 4;

    accel::Config cfg;
    if (argc >= 2) {
        try {
            cfg.load(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Warning: " << e.what() << "\n";
        }
        port_a        = static_cast<uint16_t>(cfg.get_int("port_a", port_a));
        port_b        = static_cast<uint16_t>(cfg.get_int("port_b", port_b));
        dup_precision = cfg.get_int("dup_precision", dup_precision);
    }
    // CLI overrides: server [config] [port_a] [port_b]
    if (argc >= 3) port_a = static_cast<uint16_t>(std::atoi(argv[2]));
    if (argc >= 4) port_b = static_cast<uint16_t>(std::atoi(argv[3]));

    std::signal(SIGINT,  sig_handler);
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGPIPE, SIG_IGN);

    try {
        accel::Server server(port_a, port_b, dup_precision);
        g_server = &server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
