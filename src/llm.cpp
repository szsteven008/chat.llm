#include "llm.h"
#include "openai.h"
#include <chrono>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

int LLM::init(const nlohmann::json& config, llama_generate_callback func, 
    const bool verbos /* = false */) {
    base_url = config.value("base_url", 
        "http://127.0.0.1:8080");
    std::string token = config.value("token", "");
    std::string proxy_host_port = config.value("proxy_host_port", 
        "");
    openai::start(base_url, token, proxy_host_port, 
        verbos);

    llama_thread = std::thread([this, func]() {
        llama_thread_running = true;

        auto health_check = []() {
            try {
                auto result = openai::instance().get("/health");
                if (result.contains("status") && 
                    result["status"].get<std::string>() == "ok") {
                    return true;
                }
            } catch(std::exception const& e) {
                std::cerr << "health error: " << e.what() << std::endl;
            }
            return false;
        };
        status = health_check() ? idle : none;

        auto chat_create = [](const nlohmann::json& req) {
            std::string result = "";
            try {
                auto response = openai::chat().create(req);
                result = response["choices"][0]["message"]["content"].get<std::string>();
            } catch(std::exception& e) {
                std::cerr << "Error during LLM generation: " << e.what() << std::endl;
            }
            return result;
        };

        auto start = std::chrono::system_clock::now();
        while (llama_thread_running) {
            auto now = std::chrono::system_clock::now();
            auto diff = 
                std::chrono::duration_cast<std::chrono::seconds>(now - start);
            if (diff.count() >= 10) {
                status = health_check() ? idle : none;
                start = now;
            }

            nlohmann::json req;
            {
                std::unique_lock<std::mutex> lk(mtx);
                if (!cv.wait_for(lk, std::chrono::milliseconds(100), 
                    [this]() { return (status == busy); })) {
                    continue;
                }
                req = std::move(request);
            }

            std::string result = chat_create(req);
            status = idle;
            if (func) func(result);
        }
    });
    return 0;
}

int LLM::shutdown() {
    openai::stop();
    llama_thread_running = false;
    if (llama_thread.joinable()) llama_thread.join();
    return 0;
}

int LLM::generate(const nlohmann::json& req) {
    std::lock_guard<std::mutex> lk(mtx);
    request = req;
    status = busy;
    cv.notify_one();
    return 0;
}
