#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace accel {

class NodeB {
public:
    NodeB(std::string server_host, uint16_t server_port,
          int reconnect_delay_s = 3);

    void run();
    void stop();

private:
    void run_session();

    std::string       server_host_;
    uint16_t          server_port_;
    int               reconnect_delay_s_;
    std::atomic<bool> running_{true};
};

} // namespace accel
