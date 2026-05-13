#include "node_a.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "sensor_emulator.hpp"
#include "tcp_connection.hpp"

#include <nlohmann/json.hpp>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

namespace accel {

NodeA::NodeA(std::string server_host,
             uint16_t    server_port,
             float       sensor_freq_hz,
             std::string log_file,
             int         reconnect_delay_s)
    : server_host_(std::move(server_host))
    , server_port_(server_port)
    , sensor_freq_hz_(sensor_freq_hz)
    , log_file_(std::move(log_file))
    , reconnect_delay_s_(reconnect_delay_s)
{}

void NodeA::stop() { running_ = false; }

void NodeA::run() {
    Logger::instance().set_name("node_a");
    LOG_INFO("Node A starting. Server=" + server_host_ + ":" +
             std::to_string(server_port_) + "  freq=" +
             std::to_string(sensor_freq_hz_) + " Hz  log=" + log_file_);

    while (running_) {
        try {
            run_session();
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Session error: ") + e.what());
        }

        if (!running_) break;
        LOG_INFO("Reconnecting in " + std::to_string(reconnect_delay_s_) + "s...");
        std::this_thread::sleep_for(
            std::chrono::seconds(reconnect_delay_s_));
    }
}

void NodeA::run_session() {
    auto conn = connect_to(server_host_, server_port_);
    LOG_INFO("Connected to server.");

    std::atomic<bool> active{true};
    int fd = conn.fd();

    auto wake = [&] { if (fd >= 0) ::shutdown(fd, SHUT_RDWR); };

    // Send thread: read sensor at freq_hz and push packets
    auto t_send = std::thread([&] {
        SensorEmulator sensor(sensor_freq_hz_);
        auto interval = std::chrono::microseconds(
            static_cast<int64_t>(1'000'000.0 / sensor_freq_hz_));

        while (active) {
            auto start = std::chrono::steady_clock::now();

            auto pkt = sensor.next();
            if (!conn.send_all(serialize(pkt))) {
                LOG_WARN("Send failed — server disconnected?");
                active = false;
                wake();
                break;
            }
            LOG_DEBUG("Sent ts=" + std::to_string(pkt.timestamp) +
                      " x=" + std::to_string(pkt.x) +
                      " y=" + std::to_string(pkt.y) +
                      " z=" + std::to_string(pkt.z));

            // Sleep for the remainder of the period
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto remaining = interval - elapsed;
            if (remaining > std::chrono::microseconds(0))
                std::this_thread::sleep_for(remaining);
        }
    });

    // Receive thread: read magnitude results and append to log file
    auto t_recv = std::thread([&] {
        std::ofstream log(log_file_, std::ios::app);
        if (!log)
            LOG_WARN("Cannot open log file: " + log_file_);

        while (active) {
            std::string line;
            if (!conn.read_line(line)) {
                LOG_INFO("Server closed connection.");
                active = false;
                wake();
                break;
            }
            if (line.empty()) continue;
            try {
                auto mod = accel_module_from_json(nlohmann::json::parse(line));

                if (mod.version != PROTOCOL_VERSION) {
                    LOG_WARN("Version mismatch in response: " +
                             std::to_string(mod.version));
                }

                LOG_INFO("Module ts=" + std::to_string(mod.timestamp) +
                         "  |a|=" + std::to_string(mod.module));

                if (log)
                    log << mod.timestamp << " " << mod.module << "\n" << std::flush;

            } catch (const std::exception& e) {
                LOG_WARN(std::string("Parse error: ") + e.what());
            }
        }
    });

    t_send.join();
    t_recv.join();
}

} // namespace accel
