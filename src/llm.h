#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

typedef std::function<void (const std::string&)> llama_generate_callback;
typedef std::function<std::string (const nlohmann::json&)> llama_tool_callback;
class LLM {
public:
    static LLM& instance() {
        static LLM _inst;
        return _inst;
    }

    LLM(const LLM&) = delete;
    LLM& operator=(const LLM&) = delete;

    int init(const nlohmann::json& config, llama_generate_callback func, 
        llama_tool_callback tool_func, 
        const bool verbos = false);
    int shutdown();
    int generate(const nlohmann::json& req);

    std::string llm_base_url() { return base_url; };
    bool llm_idle() const { return (status == idle); };
    bool llm_running() const { return !(status == none); };

private:
    LLM() = default;
    ~LLM() = default;

    typedef enum {
        none = 0,
        idle,
        busy
    } enuLLMStatus;

    std::string base_url = "";

    std::thread llama_thread;
    enuLLMStatus status = none;
    bool llama_thread_running = false;

    nlohmann::json request;
    std::mutex mtx;
    std::condition_variable cv;
};