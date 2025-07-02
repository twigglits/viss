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

    app.port(8000).multithreaded().run();
    std::cout << "[viss-api] Crow REST API server stopped.\n";
    return 0;
}

