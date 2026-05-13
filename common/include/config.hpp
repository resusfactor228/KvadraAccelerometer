#pragma once
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace accel {

class Config {
public:
    void load(const std::string& path) {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("Cannot open config: " + path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            data_[trim(line.substr(0, pos))] = trim(line.substr(pos + 1));
        }
    }

    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = data_.find(key);
        return it != data_.end() ? it->second : def;
    }

    int get_int(const std::string& key, int def = 0) const {
        auto it = data_.find(key);
        if (it == data_.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    }

    float get_float(const std::string& key, float def = 0.0f) const {
        auto it = data_.find(key);
        if (it == data_.end()) return def;
        try { return std::stof(it->second); } catch (...) { return def; }
    }

    void set(const std::string& key, const std::string& val) { data_[key] = val; }

private:
    std::map<std::string, std::string> data_;

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end   = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }
};

} // namespace accel
