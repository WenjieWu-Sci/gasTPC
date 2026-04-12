/// @file ConfigManager.cc
/// @brief Simple flat-JSON parser – no external library required.

#include "ConfigManager.hh"

#include <fstream>
#include <iostream>
#include <sstream>

// ---- internal helpers ----------------------------------------------------

static std::string stripWS(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

// ---- public API ----------------------------------------------------------

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::LoadFile(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "[ConfigManager] Note: '" << filename
                  << "' not found – using defaults.\n";
        return;
    }

    std::string line;
    while (std::getline(f, line)) {
        line = stripWS(line);

        // Skip empty lines, structural JSON tokens, and comment lines
        if (line.empty()) continue;
        if (line == "{" || line == "}" || line[0] == '[' || line[0] == ']') continue;
        if (line[0] == '/' || line[0] == '#') continue;

        // Expect  "key" : value[,]
        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key   = stripWS(line.substr(0, colonPos));
        std::string value = stripWS(line.substr(colonPos + 1));

        // Remove trailing comma
        if (!value.empty() && value.back() == ',') {
            value.pop_back();
            value = stripWS(value);
        }

        key = stripQuotes(key);
        if (key.empty() || value.empty()) continue;

        fConfig[key] = value;
    }
}

std::string ConfigManager::GetString(const std::string& key,
                                      const std::string& def) const {
    const auto it = fConfig.find(key);
    return (it == fConfig.end()) ? def : stripQuotes(it->second);
}

double ConfigManager::GetDouble(const std::string& key, double def) const {
    const auto it = fConfig.find(key);
    if (it == fConfig.end()) return def;
    try {
        return std::stod(stripQuotes(it->second));
    } catch (...) {
        return def;
    }
}

int ConfigManager::GetInt(const std::string& key, int def) const {
    const auto it = fConfig.find(key);
    if (it == fConfig.end()) return def;
    try {
        return std::stoi(stripQuotes(it->second));
    } catch (...) {
        return def;
    }
}

bool ConfigManager::GetBool(const std::string& key, bool def) const {
    const auto it = fConfig.find(key);
    if (it == fConfig.end()) return def;
    const std::string v = stripQuotes(it->second);
    if (v == "true"  || v == "1" || v == "yes") return true;
    if (v == "false" || v == "0" || v == "no")  return false;
    return def;
}

bool ConfigManager::HasKey(const std::string& key) const {
    return fConfig.find(key) != fConfig.end();
}
