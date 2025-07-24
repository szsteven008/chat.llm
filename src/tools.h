#pragma once

#include <cstdlib>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <vector>

typedef std::function<std::string (const nlohmann::json&)> llm_tool;

inline std::string get_weather(const nlohmann::json& argv) {
    std::vector<std::string> weather = {
        "sunny", "cloudy", "rainy", "snowy", "windy"
    };
    return weather[std::rand() % weather.size()];
}

static nlohmann::json get_weather_properties = R"(
    {
        "name": "get_weather",
        "description": "Get current temperature for a given location.",
        "parameters": {
            "type": "object",
            "properties": {
                "location": {
                    "type": "string",
                    "description": "City and country e.g. Bogotá, Colombia"
                }
            },
            "required": [
                "location"
            ],
            "additionalProperties": false
        },
        "strict": true
    }
)"_json;

class LLMTools {
public:
    static LLMTools& instance() {
        static LLMTools _inst;
        return _inst;
    }
    LLMTools(const LLMTools&) = delete;
    LLMTools& operator=(const LLMTools&) = delete;

    int init(const nlohmann::json& config);
    
    const std::vector<std::string>& names() {
        return tool_names;
    }

    nlohmann::json operator[](const std::string& name) {
        if (!tool_info_map.contains(name)) return {};
        return tool_info_map[name];
    }

    nlohmann::json response(const std::string& name, 
        const nlohmann::json& argv) {
        if (!tool_func_map.contains(name)) return {};
        return tool_func_map[name](argv);
    }

private:
    LLMTools() = default;
    ~LLMTools() = default;

    std::vector<std::string> tool_names = {
        "get_weather"
    };
    std::unordered_map<std::string, nlohmann::json> tool_info_map = {
        {"get_weather", get_weather_properties}
    };
    std::unordered_map<std::string, llm_tool> tool_func_map = {
        {"get_weather", get_weather}
    };
};