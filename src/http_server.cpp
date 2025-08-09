#define CROW_MAIN
#include "../external/crow/app.h"
#include "../external/crow/middlewares/cors.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <regex>
#include <sw/redis++/redis++.h>
using namespace sw::redis;

int main() {
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)
        .headers("Content-Type");
    std::cout << "[viss-api] Crow REST API server starting on port 8000...\n";

    CROW_ROUTE(app, "/run_simulation").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        // --- CORS for OPTIONS ---
        if (req.method == "OPTIONS"_method) {
            crow::response res;
            res.code = 204;
            std::cout << "[LOG] OPTIONS preflight handled by CORS middleware." << std::endl;
            return res;
        }
        // --- Log raw request body ---
        std::cout << "[LOG] Raw request body: " << req.body << std::endl;

        // --- Parse JSON body ---
        auto body = crow::json::load(req.body);
        int men = -1, women = -1, time = -1, seed = -1;
        if (body) {
            if (body.has("men")) men = body["men"].i();
            if (body.has("women")) women = body["women"].i();
            if (body.has("time")) time = body["time"].i();
            if (body.has("seed")) seed = body["seed"].i();
        }
        std::cout << "[LOG] Parsed params - men: " << men << ", women: " << women << ", time: " << time << ", seed: " << seed << std::endl;

        // --- Update config file if needed ---
        bool config_updated = false;
        if (men != -1 || women != -1 || time != -1) {
            std::ifstream infile("test_config1.txt");
            std::vector<std::string> lines;
            std::string line;
            bool found_men = false, found_women = false, found_time = false;
            while (std::getline(infile, line)) {
                if (men != -1 && line.find("population.nummen") != std::string::npos) {
                    lines.push_back("population.nummen = " + std::to_string(men) + "\n");
                    found_men = true;
                } else if (women != -1 && line.find("population.numwomen") != std::string::npos) {
                    lines.push_back("population.numwomen = " + std::to_string(women) + "\n");
                    found_women = true;
                } else if (time != -1 && line.find("population.simtime") != std::string::npos) {
                    lines.push_back("population.simtime = " + std::to_string(time) + "\n");
                    found_time = true;
                } else {
                    lines.push_back(line + "\n");
                }
            }
            infile.close();
            if (men != -1 && !found_men) lines.push_back("population.nummen = " + std::to_string(men) + "\n");
            if (women != -1 && !found_women) lines.push_back("population.numwomen = " + std::to_string(women) + "\n");
            if (time != -1 && !found_time) lines.push_back("population.simtime = " + std::to_string(time) + "\n");
            std::ofstream outfile("test_config1.txt");
            for (const auto& l : lines) outfile << l;
            outfile.close();
            config_updated = true;
            std::cout << "[LOG] test_config1.txt updated with new values." << std::endl;
        } else {
            std::cout << "[LOG] No config update needed." << std::endl;
        }

        // --- Run simulation and capture output ---
        std::string cmd;
        if (seed != -1) {
            cmd = "MNRM_DEBUG_SEED=" + std::to_string(seed) + " ./build/viss-release test_config1.txt 0 opt -o 2>&1";
        } else {
            cmd = "./build/viss-release test_config1.txt 0 opt -o 2>&1";
        }
        std::string output;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            crow::json::wvalue res_json;
            res_json["success"] = false;
            res_json["error"] = "Failed to run simulation process.";
            return crow::response(500, res_json.dump());
        }
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        int returnCode = pclose(pipe);

        // --- Parse output for stats ---
        int start_population = -1, end_population = -1;
        double length_of_time = -1;
        std::smatch match;
        std::regex pop_re("# Started with ([0-9]+) people, ending with ([0-9]+) ");
        std::regex time_re("# Current simulation time is ([0-9.]+)");
        std::string::const_iterator searchStart(output.cbegin());
        if (std::regex_search(output, match, pop_re) && match.size() >= 3) {
            start_population = std::stoi(match[1]);
            end_population = std::stoi(match[2]);
        }
        if (std::regex_search(output, match, time_re) && match.size() >= 2) {
            length_of_time = std::stod(match[1]);
        }

        // --- Build minimal JSON response ---
        crow::json::wvalue result;
        result["time"] = length_of_time;
        result["start_population"] = start_population;
        result["end_population"] = end_population;
        result["seed"] = seed;
        result["return_code"] = returnCode;
        // Include raw simulation log output as well
        result["output"] = output;

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = result.dump();
        std::cout << "[LOG] Returning minimal JSON response to client (CORS handled by middleware)." << std::endl;
        return res;
    });

    // New endpoint: fetch_output_config - returns the dev_eventlog.csv produced by the last run
    CROW_ROUTE(app, "/fetch_output_config").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res;
            res.code = 204;
            return res;
        }
        std::ifstream infile("dev_eventlog.csv", std::ios::binary);
        if (!infile.is_open()) {
            return crow::response(404, "dev_eventlog.csv not found");
        }
        std::ostringstream ss;
        ss << infile.rdbuf();
        infile.close();

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "text/csv; charset=utf-8");
        res.body = ss.str();
        return res;
    });

    // New endpoint: fetch_input_config - returns the full test_config1.txt after substitutions
    CROW_ROUTE(app, "/fetch_input_config").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res;
            res.code = 204;
            return res;
        }
        std::ifstream infile("test_config1.txt");
        if (!infile.is_open()) {
            return crow::response(404, "test_config1.txt not found");
        }
        std::ostringstream ss;
        ss << infile.rdbuf();
        infile.close();

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "text/plain; charset=utf-8");
        res.body = ss.str();
        return res;
    });

    app.port(8000).multithreaded().run();
    std::cout << "[viss-api] Crow REST API server stopped.\n";
    return 0;
}