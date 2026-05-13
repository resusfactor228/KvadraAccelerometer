#include "server.hpp"
#include "logger.hpp"
#include "protocol.hpp"

#include <nlohmann/json.hpp>
#include <sys/socket.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace accel {

Server::Server(uint16_t port_a, uint16_t port_b, int dup_precision)
    : port_a_(port_a), port_b_(port_b), filter_(dup_precision) {}

void Server::stop() { running_ = false; }

void Server::run() {
    Logger::instance().set_name("server");
    LOG_INFO("Node A port: " + std::to_string(port_a_) +
             "  Node B port: " + std::to_string(port_b_));

    listener_a_ = std::make_unique<TcpListener>(port_a_);
    listener_b_ = std::make_unique<TcpListener>(port_b_);

    LOG_INFO("Listening. Waiting for Node A and Node B to connect...");

    while (running_) {
        filter_.reset();

        // Accept one client on each port concurrently
        std::optional<TcpConnection> conn_a, conn_b;
        std::mutex mtx;
        std::condition_variable cv;
        bool a_done = false, b_done = false;

        auto t_accept_a = std::thread([&] {
            try {
                auto conn = listener_a_->accept();
                LOG_INFO("Node A connected from " +
                         TcpListener::peer_ip(conn.fd()));
                std::lock_guard lock(mtx);
                conn_a = std::move(conn);
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Accept A: ") + e.what());
            }
            {std::lock_guard lock(mtx); a_done = true;}
            cv.notify_all();
        });

        auto t_accept_b = std::thread([&] {
            try {
                auto conn = listener_b_->accept();
                LOG_INFO("Node B connected from " +
                         TcpListener::peer_ip(conn.fd()));
                std::lock_guard lock(mtx);
                conn_b = std::move(conn);
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Accept B: ") + e.what());
            }
            {std::lock_guard lock(mtx); b_done = true;}
            cv.notify_all();
        });

        {
            std::unique_lock lock(mtx);
            cv.wait(lock, [&] { return (a_done && b_done) || !running_; });
        }

        t_accept_a.join();
        t_accept_b.join();

        if (!conn_a || !conn_b || !running_) break;

        LOG_INFO("Both nodes connected. Starting routing.");
        route_session(std::move(*conn_a), std::move(*conn_b));
        LOG_INFO("Session ended. Waiting for new connections...");
    }
}

void Server::route_session(TcpConnection conn_a_in, TcpConnection conn_b_in) {
    auto conn_a = std::make_shared<TcpConnection>(std::move(conn_a_in));
    auto conn_b = std::make_shared<TcpConnection>(std::move(conn_b_in));

    std::atomic<bool> active{true};

    // Shutdown both sockets to unblock any pending recv/send in the other thread
    auto wake_both = [&] {
        int fa = conn_a->fd();
        int fb = conn_b->fd();
        if (fa >= 0) ::shutdown(fa, SHUT_RDWR);
        if (fb >= 0) ::shutdown(fb, SHUT_RDWR);
    };

    // Thread: Node A -> filter -> Node B
    auto t_a_to_b = std::thread([&] {
        while (active) {
            std::string line;
            if (!conn_a->read_line(line)) {
                LOG_INFO("Node A disconnected.");
                active = false;
                wake_both();
                break;
            }
            if (line.empty()) continue;
            try {
                auto pkt = accel_packet_from_json(nlohmann::json::parse(line));

                if (filter_.is_duplicate(pkt)) {
                    LOG_DEBUG("Duplicate dropped ts=" +
                              std::to_string(pkt.timestamp));
                    continue;
                }

                LOG_DEBUG("-> B ts=" + std::to_string(pkt.timestamp) +
                          " x=" + std::to_string(pkt.x) +
                          " y=" + std::to_string(pkt.y) +
                          " z=" + std::to_string(pkt.z));

                if (!conn_b->send_all(serialize(pkt))) {
                    LOG_WARN("Send to Node B failed.");
                    active = false;
                    wake_both();
                }
            } catch (const std::exception& e) {
                LOG_WARN(std::string("Parse error from A: ") + e.what());
            }
        }
    });

    // Thread: Node B -> Node A
    auto t_b_to_a = std::thread([&] {
        while (active) {
            std::string line;
            if (!conn_b->read_line(line)) {
                LOG_INFO("Node B disconnected.");
                active = false;
                wake_both();
                break;
            }
            if (line.empty()) continue;
            try {
                auto mod = accel_module_from_json(nlohmann::json::parse(line));

                if (mod.version != PROTOCOL_VERSION) {
                    LOG_WARN("Version mismatch from B: got " +
                             std::to_string(mod.version));
                }

                LOG_DEBUG("<- B module=" + std::to_string(mod.module) +
                          " ts=" + std::to_string(mod.timestamp));

                if (!conn_a->send_all(serialize(mod))) {
                    LOG_WARN("Send to Node A failed.");
                    active = false;
                    wake_both();
                }
            } catch (const std::exception& e) {
                LOG_WARN(std::string("Parse error from B: ") + e.what());
            }
        }
    });

    t_a_to_b.join();
    t_b_to_a.join();
    // shared_ptrs destroyed here -> fds closed
}

} // namespace accel
