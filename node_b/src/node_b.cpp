#include "node_b.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "tcp_connection.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cmath>
#include <thread>

namespace accel {

NodeB::NodeB(std::string server_host, uint16_t server_port,
             int reconnect_delay_s)
    : server_host_(std::move(server_host))
    , server_port_(server_port)
    , reconnect_delay_s_(reconnect_delay_s)
{}

void NodeB::stop() { running_ = false; }

void NodeB::run() {
    Logger::instance().set_name("node_b");
    LOG_INFO("Node B starting. Server=" + server_host_ + ":" +
             std::to_string(server_port_));

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

void NodeB::run_session() {
    auto conn = connect_to(server_host_, server_port_);
    LOG_INFO("Connected to server.");

    while (running_) {
        std::string line;
        if (!conn.read_line(line)) {
            LOG_INFO("Server closed connection.");
            break;
        }
        if (line.empty()) continue;

        try {
            auto pkt = accel_packet_from_json(nlohmann::json::parse(line));

            if (pkt.version != PROTOCOL_VERSION) {
                LOG_WARN("Version mismatch: got " +
                         std::to_string(pkt.version) + " — processing anyway");
            }

            float mag = std::sqrt(pkt.x * pkt.x + pkt.y * pkt.y + pkt.z * pkt.z);

            LOG_INFO("ts=" + std::to_string(pkt.timestamp) +
                     "  |a|=" + std::to_string(mag));

            AccelModule mod;
            mod.timestamp = pkt.timestamp;
            mod.module    = mag;

            if (!conn.send_all(serialize(mod))) {
                LOG_WARN("Send failed — server disconnected?");
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN(std::string("Parse error: ") + e.what());
        }
    }
}

} // namespace accel
