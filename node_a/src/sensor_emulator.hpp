#pragma once
#include "protocol.hpp"
#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace accel {

// Generates sinusoidal accelerometer data that mimics a device held still
// but with slight vibration. Produces a non-constant signal to exercise
// the server's duplicate-filter.
class SensorEmulator {
public:
    explicit SensorEmulator(float freq_hz = 50.0f) : freq_hz_(freq_hz) {}

    AccelPacket next() {
        auto now = std::chrono::system_clock::now();
        int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()).count();

        // Relative time in seconds from program start — keeps float precision.
        // (Epoch seconds ~1.7e9 exceed float mantissa precision, causing stale sin values.)
        float t = std::chrono::duration<float>(
                      std::chrono::steady_clock::now() - start_).count();

        AccelPacket p;
        p.timestamp = ts;
        // Gravity on Y axis plus small sinusoidal noise on each axis
        p.x = 0.5f  * std::sin(2.0f * static_cast<float>(M_PI) * 1.0f  * t);
        p.y = 9.81f + 0.1f * std::cos(2.0f * static_cast<float>(M_PI) * 0.5f * t);
        p.z = 0.2f  * std::sin(2.0f * static_cast<float>(M_PI) * 2.0f  * t + 0.5f);
        return p;
    }

    float freq_hz() const { return freq_hz_; }

private:
    float                                              freq_hz_;
    std::chrono::steady_clock::time_point             start_ = std::chrono::steady_clock::now();
};

} // namespace accel
