#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <stdexcept>

namespace nlohmann {

class json {
public:
    using value_t = std::variant<std::nullptr_t, bool, int, double, std::string, std::vector<json>, std::map<std::string, json>>;

    json() : value(nullptr) {}
    json(std::nullptr_t) : value(nullptr) {}
    json(bool b) : value(b) {}
    json(int i) : value(i) {}
    json(double d) : value(d) {}
    json(const std::string& s) : value(s) {}
    json(const char* s) : value(std::string(s)) {}
    json(const std::vector<json>& v) : value(v) {}
    json(const std::map<std::string, json>& m) : value(m) {}

    static json parse(const std::string& s) {
        // 简化版解析，仅支持基本结构
        // 实际项目中应使用完整的JSON解析库
        throw std::runtime_error("Not implemented");
    }

    json& operator[](const std::string& key) {
        if (std::holds_alternative<std::map<std::string, json>>(value)) {
            return std::get<std::map<std::string, json>>(value)[key];
        } else {
            value = std::map<std::string, json>();
            return std::get<std::map<std::string, json>>(value)[key];
        }
    }

    template<typename T>
    T get() const {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        } else if (std::holds_alternative<int>(value) && std::is_same<T, double>::value) {
            return static_cast<double>(std::get<int>(value));
        } else {
            throw std::runtime_error("Type mismatch");
        }
    }

    friend std::istream& operator>>(std::istream& is, json& j) {
        // 简化版输入，仅支持基本结构
        // 实际项目中应使用完整的JSON解析库
        throw std::runtime_error("Not implemented");
    }

private:
    value_t value;
};

} // namespace nlohmann