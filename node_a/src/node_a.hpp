#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace accel {

class NodeA {
public:
    NodeA(std::string server_host,
          uint16_t    server_port,
          float       sensor_freq_hz,
          std::string log_file,
          int         reconnect_delay_s = 3);

    // Blocking: runs until stop() is called.
    void run();
    void stop();

private:
    // Runs one connected session; returns when connection is lost.
    void run_session();

    std::string       server_host_;
    uint16_t          server_port_;
    float             sensor_freq_hz_;
    std::string       log_file_;
    int               reconnect_delay_s_;
    std::atomic<bool> running_{true};
};

} // namespace accel
