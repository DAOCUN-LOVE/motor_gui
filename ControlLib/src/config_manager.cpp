#include "config_manager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << filename << std::endl;
        return false;
    }
    
    try {
        // 简化版JSON解析，仅支持M6020电机的速度PID配置
        std::string line;
        std::string content;
        while (std::getline(file, line)) {
            content += line;
        }
        
        // 简单解析，提取PID参数
        // 实际项目中应使用完整的JSON解析库
        size_t kp_pos = content.find("\"kp\":");
        size_t ki_pos = content.find("\"ki\":");
        size_t kd_pos = content.find("\"kd\":");
        size_t max_out_pos = content.find("\"max_out\":");
        size_t max_iout_pos = content.find("\"max_iout\":");
        
        if (kp_pos != std::string::npos && ki_pos != std::string::npos && kd_pos != std::string::npos) {
            // 提取kp值
            size_t kp_start = content.find(":", kp_pos) + 1;
            size_t kp_end = content.find(",", kp_start);
            std::string kp_str = content.substr(kp_start, kp_end - kp_start);
            m_currentConfig.kp = std::stof(kp_str);
            
            // 提取ki值
            size_t ki_start = content.find(":", ki_pos) + 1;
            size_t ki_end = content.find(",", ki_start);
            std::string ki_str = content.substr(ki_start, ki_end - ki_start);
            m_currentConfig.ki = std::stof(ki_str);
            
            // 提取kd值
            size_t kd_start = content.find(":", kd_pos) + 1;
            size_t kd_end = content.find(",", kd_start);
            std::string kd_str = content.substr(kd_start, kd_end - kd_start);
            m_currentConfig.kd = std::stof(kd_str);
            
            // 提取max_out值
            if (max_out_pos != std::string::npos) {
                size_t max_out_start = content.find(":", max_out_pos) + 1;
                size_t max_out_end = content.find(",", max_out_start);
                std::string max_out_str = content.substr(max_out_start, max_out_end - max_out_start);
                m_currentConfig.max_out = std::stof(max_out_str);
            }
            
            // 提取max_iout值
            if (max_iout_pos != std::string::npos) {
                size_t max_iout_start = content.find(":", max_iout_pos) + 1;
                size_t max_iout_end = content.find(",", max_iout_start);
                if (max_iout_end == std::string::npos) {
                    max_iout_end = content.find("}", max_iout_start);
                }
                std::string max_iout_str = content.substr(max_iout_start, max_iout_end - max_iout_start);
                m_currentConfig.max_iout = std::stof(max_iout_str);
            }
            
            std::cout << "配置文件加载成功: " << std::endl;
            std::cout << "kp: " << m_currentConfig.kp << std::endl;
            std::cout << "ki: " << m_currentConfig.ki << std::endl;
            std::cout << "kd: " << m_currentConfig.kd << std::endl;
            std::cout << "max_out: " << m_currentConfig.max_out << std::endl;
            std::cout << "max_iout: " << m_currentConfig.max_iout << std::endl;
            return true;
        } else {
            std::cerr << "配置文件格式错误" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "配置文件解析错误: " << e.what() << std::endl;
        return false;
    }
}

Pid::PidConfig ConfigManager::getPidConfig(const std::string& motorType, const std::string& controlType) {
    // 目前仅支持M6020电机的速度PID配置
    // 实际项目中应根据电机类型和控制模式返回不同的配置
    return m_currentConfig;
}