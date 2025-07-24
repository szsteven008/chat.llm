#include "llm.h"
#include "openai.h"
#include <chrono>
#include <exception>
#include <format>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <thread>
#include <vector>

int LLM::init(const nlohmann::json& config, llama_generate_callback func, 
    llama_tool_callback tool_func, const bool verbos /* = false */) {
    base_url = config.value("base_url", 
        "http://127.0.0.1:8080");
    std::string token = config.value("token", "");
    std::string proxy_host_port = config.value("proxy_host_port", 
        "");
    openai::start(base_url, token, proxy_host_port, 
        verbos);

    llama_thread = std::thread([this, func, tool_func]() {
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
            nlohmann::json result = {};
            try {
                auto response = openai::chat().create(req);
                result = response["choices"][0];
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

            nlohmann::json result = chat_create(req);
            if (result.contains("finish_reason") && result.contains("message")) {
                std::string finish_reason = result["finish_reason"].get<std::string>();
                if (finish_reason == "tool_calls") {
                    nlohmann::json& message = result["message"];
                    if (message.contains("tool_calls")) {
                        std::vector<nlohmann::json> tool_calls = 
                            message["tool_calls"].get<std::vector<nlohmann::json>>();
                        auto& messages = req["messages"];
                        messages.push_back(message);
                        for (auto& tool: tool_calls) {
                            nlohmann::json tool_call_result;
                            tool_call_result["role"] = "tool";
                            tool_call_result["tool_call_id"] = tool["id"].get<std::string>();
                            tool_call_result["content"] = tool_func(tool);
                            messages.push_back(tool_call_result);
                        }
//                        std::cout << "req: " << req.dump('\t') << std::endl;
                        generate(req);
                    }
                } else {
                    std::string content = "";
                    nlohmann::json& message = result["message"];
                    if (message.contains("reasoning_content")) {
                        content += std::format("<think>{}</think>\n\n", 
                            message["reasoning_content"].get<std::string>());
                    }
                    if (message.contains("content")) {
                        content += message["content"].get<std::string>();
                    }

                    status = idle;
                    if (func) func(content);
                }
            } else {
                std::cout << "unsupported: " << result.dump('\t') << std::endl;
                status = idle;
            }
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
