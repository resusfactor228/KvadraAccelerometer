#include "node_a.hpp"
#include "config.hpp"
#include "logger.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>

static accel::NodeA* g_node = nullptr;

static void sig_handler(int) {
    if (g_node) g_node->stop();
}

int main(int argc, char* argv[]) {
    std::string server_host      = "127.0.0.1";
    uint16_t    server_port      = 8080;
    float       sensor_freq_hz   = 50.0f;
    std::string log_file         = "accel_module.log";
    int         reconnect_delay  = 3;

    accel::Config cfg;
    if (argc >= 2) {
        try {
            cfg.load(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Warning: " << e.what() << "\n";
        }
        server_host     = cfg.get("server_host", server_host);
        server_port     = static_cast<uint16_t>(cfg.get_int("server_port", server_port));
        sensor_freq_hz  = cfg.get_float("sensor_freq_hz", sensor_freq_hz);
        log_file        = cfg.get("log_file", log_file);
        reconnect_delay = cfg.get_int("reconnect_delay_s", reconnect_delay);
    }
    // CLI overrides: node_a [config] [host] [port] [freq_hz] [log_file]
    if (argc >= 3) server_host    = argv[2];
    if (argc >= 4) server_port    = static_cast<uint16_t>(std::atoi(argv[3]));
    if (argc >= 5) sensor_freq_hz = std::atof(argv[4]);
    if (argc >= 6) log_file       = argv[5];

    std::signal(SIGINT,  sig_handler);
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGPIPE, SIG_IGN);

    try {
        accel::NodeA node(server_host, server_port, sensor_freq_hz,
                          log_file, reconnect_delay);
        g_node = &node;
        node.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
