#pragma once
#include "protocol.hpp"
#include <cmath>
#include <optional>

namespace accel {

class DuplicateFilter {
public:
    explicit DuplicateFilter(int decimal_places = 4)
        : scale_(std::pow(10.0f, static_cast<float>(decimal_places))) {}

    // Returns true if packet is an exact duplicate of the previous one.
    // Always accepts (returns false) the very first packet.
    bool is_duplicate(const AccelPacket& pkt) {
        if (!prev_) {
            prev_ = pkt;
            return false;
        }
        bool dup = (rnd(pkt.x) == rnd(prev_->x)) &&
                   (rnd(pkt.y) == rnd(prev_->y)) &&
                   (rnd(pkt.z) == rnd(prev_->z));
        if (!dup) prev_ = pkt;
        return dup;
    }

    void reset() { prev_.reset(); }

private:
    float rnd(float v) const { return std::round(v * scale_) / scale_; }

    float                      scale_;
    std::optional<AccelPacket> prev_;
};

} // namespace accel
