#define CROW_MAIN
#include "../external/crow_all.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>

int main() {
    crow::SimpleApp app;
    std::cout << "[viss-api] Crow REST API server starting on port 8000...\n";

    CROW_ROUTE(app, "/run_simulation")([](const crow::request& req){
        std::cout << "[viss-api] /run_simulation endpoint called. Running simulation...\n";
        int ret = std::system("./build/viss-release test_config1.txt 0 opt -o > output.txt 2>&1");
        std::ifstream t("output.txt");
        std::string output((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        std::cout << "[viss-api] Contents of output.txt:\n" << output << std::endl;
        std::cout << "[viss-api] Simulation complete. Returning output.\n";
        return crow::response(output);
    });

    // Serve dev_eventlog.csv as text/csv
    CROW_ROUTE(app, "/get-eventlog")([](const crow::request& req){
        std::ifstream file("dev_eventlog.csv", std::ios::binary);
        if (!file) {
            std::cout << "[viss-api] /get-eventlog requested but dev_eventlog.csv not found!\n";
            return crow::response(404, "dev_eventlog.csv not found");
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        crow::response res(ss.str());
        res.add_header("Content-Type", "text/csv");
        res.code = 200;
        std::cout << "[viss-api] /get-eventlog served dev_eventlog.csv (" << ss.str().size() << " bytes)\n";
        return res;
    });

    app.port(8000).multithreaded().run();
    std::cout << "[viss-api] Crow REST API server stopped.\n";
    return 0;
}

