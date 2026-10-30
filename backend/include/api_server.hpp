#pragma once

#include <string>
#include <memory>
#include "httplib.h"

namespace multicorex {

class ApiServer {
public:
    explicit ApiServer(const std::string& host = "0.0.0.0", int port = 8080, const std::string& static_dir = "frontend");
    ~ApiServer();

    void start();
    void stop();

private:
    void setup_routes();
    void apply_cors(httplib::Response& res);

    std::string host_;
    int port_;
    std::string static_dir_;
    std::unique_ptr<httplib::Server> server_;
};

} // namespace multicorex
