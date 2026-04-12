#pragma once
/// @file ConfigManager.hh
/// @brief Singleton that loads and stores flat JSON config key-value pairs.
///
/// Implements a simple hand-rolled parser that handles:
///   - String values  (quoted)
///   - Numeric values (integer / floating-point)
///   - Boolean values (true / false)
/// No external JSON library is required.

#include <map>
#include <stdexcept>
#include <string>

class ConfigManager {
public:
    /// Return the single global instance.
    static ConfigManager& Instance();

    /// Load (or merge) a JSON file into the internal map.
    /// Missing or unreadable files are silently ignored (defaults apply).
    void LoadFile(const std::string& filename);

    std::string GetString(const std::string& key, const std::string& def = "") const;
    double      GetDouble(const std::string& key, double def = 0.0) const;
    int         GetInt   (const std::string& key, int    def = 0)   const;
    bool        GetBool  (const std::string& key, bool   def = false) const;
    bool        HasKey   (const std::string& key) const;

private:
    ConfigManager() = default;
    std::map<std::string, std::string> fConfig;
};
