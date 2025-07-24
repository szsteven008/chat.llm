#include "server.h"
#include <memory>
#include <vector>
#include <boost/process.hpp>

int Server::init(const nlohmann::json& config) {
    std::string bin = config.value("bin", 
        "tools/llama-server");
    std::vector<std::string> args = 
        config.value<std::vector<std::string>>("args", 
            {
            "--model", "models/Qwen3-0.6B-Q8_0.gguf", 
            "--ctx-size", "2048"
            });

    bool on = config.value("on", false);
    if (on) {
        proc.reset(new boost::process::process(
            ctx.get_executor(), 
            bin, 
            args));
    }

    return 0;
}

int Server::shutdown() {
    if (proc->running()) {
        proc->interrupt();
        proc->wait();
    }
    return 0;
}
