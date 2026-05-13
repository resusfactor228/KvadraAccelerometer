#pragma once
#include "duplicate_filter.hpp"
#include "tcp_connection.hpp"
#include <atomic>
#include <cstdint>
#include <memory>

namespace accel {

class Server {
public:
    Server(uint16_t port_a, uint16_t port_b, int dup_precision = 4);

    // Blocking: accepts connections and routes data until stop() is called.
    void run();
    void stop();

private:
    // Runs two routing threads (A->B and B->A) until either side disconnects.
    void route_session(TcpConnection conn_a, TcpConnection conn_b);

    uint16_t          port_a_;
    uint16_t          port_b_;
    std::atomic<bool> running_{true};
    DuplicateFilter   filter_;

    std::unique_ptr<TcpListener> listener_a_;
    std::unique_ptr<TcpListener> listener_b_;
};

} // namespace accel
