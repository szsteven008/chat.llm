#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <boost/process.hpp>

class Server {
public:
    static Server& instance() {
        static Server _inst;
        return _inst;
    };
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    int init(const nlohmann::json& config);
    int shutdown();

private:
    Server() = default;
    ~Server() = default;

    boost::asio::io_context ctx;
    std::unique_ptr<boost::process::process> proc;
};