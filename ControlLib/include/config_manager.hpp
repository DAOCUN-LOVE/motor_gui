#pragma once
#include <string>
#include "pid.hpp"

class ConfigManager {
public:
    static ConfigManager& instance();
    bool loadConfig(const std::string& filename);
    Pid::PidConfig getPidConfig(const std::string& motorType, const std::string& controlType);
    
private:
    ConfigManager() = default;
    Pid::PidConfig m_currentConfig = {500.f, 40.f, 0.2f, 30000.0f, 15000.0f};
};