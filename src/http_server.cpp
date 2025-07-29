#define CROW_MAIN
#include "../external/crow/app.h"
#include "../external/crow/middlewares/cors.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <regex>
#include <sw/redis++/redis++.h>
// /home/jeannaude/epi/viss/redis-plus-plus/src/sw/redis++/redis.cpp
using namespace sw::redis;

int main() {
    // --- GLOBAL LOG SUPPRESSION ---
    static std::ofstream null_stream("/dev/null");
    std::cout.rdbuf(null_stream.rdbuf());
    std::cerr.rdbuf(null_stream.rdbuf());
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

        // // --- Log config file before update ---
        // std::cout << "[LOG] test_config1.txt BEFORE update:\n";
        // {
        //     std::ifstream infile("test_config1.txt");
        //     std::string line;
        //     while (std::getline(infile, line)) std::cout << line << std::endl;
        // }

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

        // --- Build JSON response ---
        crow::json::wvalue result;
        result["success"] = true;
        result["men"] = men;
        result["women"] = women;
        result["time"] = time;
        // result["output"] = output;

        // Try to parse some stats from the output log
        int start_population = -1, end_population = -1, length_of_time = -1;
        try {
            std::smatch match;
            std::regex start_pop_re("Started with ([0-9]+) people");
            std::regex end_pop_re("ending with ([0-9]+) ");
            std::regex time_re("Current simulation time is ([0-9.]+)");
            // if (std::regex_search(output, match, start_pop_re)) {
            //     start_population = std::stoi(match[1]);
            // }
            // if (std::regex_search(output, match, end_pop_re)) {
            //     end_population = std::stoi(match[1]);
            // }
            // if (std::regex_search(output, match, time_re)) {
            //     length_of_time = std::stof(match[1]);
            // }
        } catch (...) {
            std::cout << "[LOG] Failed to parse stats from output." << std::endl;
        }
        result["start_population"] = start_population;
        result["end_population"] = end_population;
        result["length_of_time"] = length_of_time;

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = result.dump();
        std::cout << "[LOG] Returning JSON response to client (CORS handled by middleware)." << std::endl;
        return res;
    });

    app.port(8000).multithreaded().run();
    std::cout << "[viss-api] Crow REST API server stopped.\n";
    return 0;
}
    // Serve dev_eventlog.csv as text/csv
//     CROW_ROUTE(app, "/get-eventlog")([](const crow::request& req){
//         (void)req; // silence unused parameter warning
//         std::ifstream file("dev_eventlog.csv", std::ios::binary);
//         if (!file) {
//             std::cout << "[viss-api] /get-eventlog requested but dev_eventlog.csv not found!\n";
//             return crow::response(404, "dev_eventlog.csv not found");
//         }
//         std::ostringstream ss;
//         ss << file.rdbuf();
//         crow::response res(ss.str());
//         res.add_header("Content-Type", "text/csv");
//         res.code = 200;
//         std::cout << "[viss-api] /get-eventlog served dev_eventlog.csv (" << ss.str().size() << " bytes)\n";
//         return res;
//     });

//     // Serve population over time as JSON
//     CROW_ROUTE(app, "/population-over-time")([](const crow::request& req){
//         // Get initial population from test_config1.txt
//         int men = 0, women = 0;
//         {
//             std::ifstream config("test_config1.txt");
//             std::string line;
//             std::regex men_re("population.nummen\\s*=\\s*([0-9]+)");
//             std::regex women_re("population.numwomen\\s*=\\s*([0-9]+)");
//             std::smatch match;
//             while (std::getline(config, line)) {
//                 if (std::regex_search(line, match, men_re)) {
//                     men = std::stoi(match[1]);
//                 }
//                 if (std::regex_search(line, match, women_re)) {
//                     women = std::stoi(match[1]);
//                 }
//             }
//         }
//         int current_population = men + women;

//         std::ifstream file("dev_eventlog.csv");
//         if (!file) {
//             std::cout << "[viss-api] /population-over-time requested but dev_eventlog.csv not found!\n";
//             return crow::response(404, "dev_eventlog.csv not found");
//         }
//         std::string line;
//         std::vector<std::pair<int, int>> pop_over_time;
//         // skip header
//         std::getline(file, line);
//         while (std::getline(file, line)) {
//             std::istringstream ss(line);
//             std::string time_str, event;
//             std::getline(ss, time_str, ',');
//             std::getline(ss, event, ',');
//             int time = std::stoi(time_str);
//             if (event == "death") {
//                 current_population -= 1;
//             }
//             pop_over_time.push_back({time, current_population});
//         }
//         crow::json::wvalue arr = crow::json::wvalue::list();
//         size_t i = 0;
//         for (const auto& p : pop_over_time) {
//             crow::json::wvalue entry;
//             entry["time"] = p.first;
//             entry["population"] = p.second;
//             arr[i++] = std::move(entry);
//         }
//         // --- Serialize JSON for Redis ---
//         std::string json_payload = arr.dump();
//         std::cout << "[viss-api][REDIS] Serialized population-over-time JSON: " << json_payload.substr(0, 256) << (json_payload.size() > 256 ? "..." : "") << std::endl;
//         // --- Push to Redis ---
//         try {
//             auto redis = Redis("tcp://redis:6379");
//             redis.set("latest_simulation", json_payload);
//             std::cout << "[viss-api][REDIS] Successfully pushed latest_simulation to Redis (" << json_payload.size() << " bytes)" << std::endl;
//         } catch (const std::exception& e) {
//             std::cout << "[viss-api][REDIS][ERROR] Failed to push to Redis: " << e.what() << std::endl;
//         }
//         crow::response res;
//         res.code = 200;
//         res.set_header("Content-Type", "application/json");
//         res.body = json_payload;
//         std::cout << "[viss-api] /population-over-time served " << pop_over_time.size() << " points\n";
//         return res;
//     });

//     // New: Serve cached population-over-time from Redis
//     CROW_ROUTE(app, "/retrieve-from-cache")([](const crow::request& req){
//         (void)req; // silence unused parameter warning
//         try {
//             auto redis = Redis("tcp://redis:6379");
//             auto val = redis.get("latest_simulation");
//             if (val) {
//                 std::cout << "[viss-api][REDIS] /retrieve-from-cache hit: " << (val->size() > 256 ? val->substr(0, 256) + "..." : *val) << std::endl;
//                 crow::response res;
//                 res.code = 200;
//                 res.set_header("Content-Type", "application/json");
//                 res.body = *val;
//                 return res;
//             } else {
//                 std::cout << "[viss-api][REDIS][MISS] /retrieve-from-cache: latest_simulation not found in Redis" << std::endl;
//                 return crow::response(404, "No cached simulation found");
//             }
//         } catch (const std::exception& e) {
//             std::cout << "[viss-api][REDIS][ERROR] /retrieve-from-cache: " << e.what() << std::endl;
//             return crow::response(500, "Redis error");
//         }
//     });
    // app.port(8000).multithreaded().run();
    // std::cout << "[viss-api] Crow REST API server stopped.\n";
    // return 0;
// }

