#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace accel {

constexpr int PROTOCOL_VERSION = 1;

struct AccelPacket {
    int     version   = PROTOCOL_VERSION;
    int64_t timestamp = 0;
    float   x         = 0.0f;
    float   y         = 0.0f;
    float   z         = 0.0f;
};

struct AccelModule {
    int     version   = PROTOCOL_VERSION;
    int64_t timestamp = 0;
    float   module    = 0.0f;
};

inline nlohmann::json to_json(const AccelPacket& p) {
    return {
        {"version",   p.version},
        {"timestamp", p.timestamp},
        {"x",         p.x},
        {"y",         p.y},
        {"z",         p.z}
    };
}

inline AccelPacket accel_packet_from_json(const nlohmann::json& j) {
    AccelPacket p;
    p.version   = j.value("version", PROTOCOL_VERSION);
    p.timestamp = j.at("timestamp").get<int64_t>();
    p.x         = j.at("x").get<float>();
    p.y         = j.at("y").get<float>();
    p.z         = j.at("z").get<float>();
    return p;
}

inline nlohmann::json to_json(const AccelModule& m) {
    return {
        {"version",   m.version},
        {"timestamp", m.timestamp},
        {"module",    m.module}
    };
}

inline AccelModule accel_module_from_json(const nlohmann::json& j) {
    AccelModule m;
    m.version   = j.value("version", PROTOCOL_VERSION);
    m.timestamp = j.at("timestamp").get<int64_t>();
    m.module    = j.at("module").get<float>();
    return m;
}

inline std::string serialize(const AccelPacket& p) {
    return to_json(p).dump() + "\n";
}

inline std::string serialize(const AccelModule& m) {
    return to_json(m).dump() + "\n";
}

} // namespace accel
