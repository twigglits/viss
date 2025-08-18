#define CROW_MAIN
#include "../external/crow/app.h"
#include "../external/crow/middlewares/cors.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <regex>
#include <vector>
#include <unordered_set>
#include <ctime>
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

        // --- Compute population timeline, HIV infections timeline, and HIV prevalence timeline from dev_eventlog.csv and persist to Redis ---
        std::string redis_key_used;
        std::string hiv_redis_key_used;
        std::string hiv_prevalence_redis_key_used;
        std::string hiv_incidence_redis_key_used;
        try {
            // Compute timelines
            std::ifstream evfile("dev_eventlog.csv");
            if (evfile.is_open() && start_population >= 0) {
                std::vector<std::pair<double,int>> timeline;
                std::vector<std::pair<double,int>> hiv_timeline;
                std::vector<std::pair<double,double>> hiv_prevalence_timeline;
                std::vector<std::pair<double,double>> hiv_incidence_timeline;
                int pop = start_population;
                int cumulative_hiv_infections = 0;
                int current_hiv_positive = 0;
                std::unordered_set<std::string> hiv_positive_individuals;
                
                // For incidence calculation (yearly windows)
                std::map<int, int> yearly_infections; // year -> infection count
                std::map<int, int> yearly_population; // year -> population at start of year
                std::string line;
                bool first_point_added = false;
                bool first_hiv_point_added = false;
                bool first_prevalence_point_added = false;
                
                while (std::getline(evfile, line)) {
                    if (line.empty()) continue;
                    // Fast parse: first field = time, second field = event
                    // Find first comma
                    size_t c1 = line.find(',');
                    if (c1 == std::string::npos) continue;
                    double t = 0.0;
                    try { t = std::stod(line.substr(0, c1)); } catch(...) { continue; }
                    // second field
                    size_t c2 = line.find(',', c1 + 1);
                    std::string evt = (c2 == std::string::npos) ? line.substr(c1 + 1) : line.substr(c1 + 1, c2 - (c1 + 1));
                    
                    // Parse individual IDs for HIV tracking
                    std::string individual_id;
                    std::string recipient_id;
                    if (evt == "transmission") {
                        // For transmission: time,transmission,source_id,source_num,source_gender,source_age,recipient_id,recipient_num,recipient_gender,recipient_age,originSPVL,value
                        size_t c3 = line.find(',', c2 + 1); // source_id
                        size_t c4 = line.find(',', c3 + 1); // source_num
                        size_t c5 = line.find(',', c4 + 1); // source_gender
                        size_t c6 = line.find(',', c5 + 1); // source_age
                        size_t c7 = line.find(',', c6 + 1); // recipient_id
                        if (c3 != std::string::npos && c7 != std::string::npos) {
                            individual_id = line.substr(c2 + 1, c3 - (c2 + 1)); // source (already infected)
                            size_t c8 = line.find(',', c7 + 1);
                            if (c8 != std::string::npos) {
                                recipient_id = line.substr(c7 + 1, c8 - (c7 + 1)); // newly infected
                            }
                        }
                    } else if (evt == "normalmortality" || evt == "aidsmortality") {
                        // For mortality: time,event,individual_id,num,gender,age,(none),...
                        size_t c3 = line.find(',', c2 + 1);
                        if (c3 != std::string::npos) {
                            individual_id = line.substr(c2 + 1, c3 - (c2 + 1));
                        }
                    }

                    // Track yearly population for incidence calculation
                    int year = static_cast<int>(1980 + t);
                    if (yearly_population.find(year) == yearly_population.end()) {
                        yearly_population[year] = pop;
                    }

                    // Population timeline
                    if (!first_point_added) {
                        timeline.emplace_back(0.0, pop);
                        first_point_added = true;
                    }
                    if (evt == "birth") {
                        pop += 1;
                        timeline.emplace_back(t, pop);
                    } else if (evt == "normalmortality" || evt == "aidsmortality") {
                        pop -= 1;
                        timeline.emplace_back(t, pop);
                    }
                    
                    // HIV infections timeline (cumulative)
                    if (!first_hiv_point_added) {
                        hiv_timeline.emplace_back(0.0, cumulative_hiv_infections);
                        first_hiv_point_added = true;
                    }
                    if (evt == "transmission") {
                        cumulative_hiv_infections += 1;
                        hiv_timeline.emplace_back(t, cumulative_hiv_infections);
                        // Track the newly infected individual (recipient)
                        if (!recipient_id.empty()) {
                            hiv_positive_individuals.insert(recipient_id);
                            current_hiv_positive++;
                        }
                        
                        // Track yearly infections for incidence calculation
                        yearly_infections[year]++;
                    }
                    
                    // Handle deaths of HIV-positive individuals
                    if ((evt == "normalmortality" || evt == "aidsmortality") && !individual_id.empty()) {
                        if (hiv_positive_individuals.count(individual_id)) {
                            hiv_positive_individuals.erase(individual_id);
                            current_hiv_positive--;
                        }
                    }
                    
                    // HIV prevalence timeline (percentage)
                    if (!first_prevalence_point_added) {
                        double prevalence = (pop > 0) ? (100.0 * current_hiv_positive / pop) : 0.0;
                        hiv_prevalence_timeline.emplace_back(0.0, prevalence);
                        first_prevalence_point_added = true;
                    }
                    if (evt == "transmission" || evt == "normalmortality" || evt == "aidsmortality" || evt == "birth") {
                        double prevalence = (pop > 0) ? (100.0 * current_hiv_positive / pop) : 0.0;
                        hiv_prevalence_timeline.emplace_back(t, prevalence);
                    }
                }
                evfile.close();
                
                // Calculate HIV incidence timeline (yearly percentages)
                for (const auto& year_pop : yearly_population) {
                    int year = year_pop.first;
                    int population = year_pop.second;
                    int infections = yearly_infections[year]; // defaults to 0 if not found
                    
                    if (population > 0) {
                        double incidence = (100.0 * infections) / population;
                        double year_time = year - 1980; // convert back to simulation time
                        hiv_incidence_timeline.emplace_back(year_time, incidence);
                    }
                }

                // Serialize population timeline to compact JSON array: [[t,p], ...]
                std::ostringstream json_ss;
                json_ss << "[";
                for (size_t i = 0; i < timeline.size(); ++i) {
                    if (i) json_ss << ",";
                    json_ss << "[" << timeline[i].first << "," << timeline[i].second << "]";
                }
                json_ss << "]";
                
                // Serialize HIV infections timeline to compact JSON array: [[t,infections], ...]
                std::ostringstream hiv_json_ss;
                hiv_json_ss << "[";
                for (size_t i = 0; i < hiv_timeline.size(); ++i) {
                    if (i) hiv_json_ss << ",";
                    hiv_json_ss << "[" << hiv_timeline[i].first << "," << hiv_timeline[i].second << "]";
                }
                hiv_json_ss << "]";
                
                // Serialize HIV prevalence timeline to compact JSON array: [[t,percentage], ...]
                std::ostringstream hiv_prevalence_json_ss;
                hiv_prevalence_json_ss << "[";
                for (size_t i = 0; i < hiv_prevalence_timeline.size(); ++i) {
                    if (i) hiv_prevalence_json_ss << ",";
                    hiv_prevalence_json_ss << "[" << hiv_prevalence_timeline[i].first << "," << hiv_prevalence_timeline[i].second << "]";
                }
                hiv_prevalence_json_ss << "]";
                
                // Serialize HIV incidence timeline to compact JSON array: [[t,percentage], ...]
                std::ostringstream hiv_incidence_json_ss;
                hiv_incidence_json_ss << "[";
                for (size_t i = 0; i < hiv_incidence_timeline.size(); ++i) {
                    if (i) hiv_incidence_json_ss << ",";
                    hiv_incidence_json_ss << "[" << hiv_incidence_timeline[i].first << "," << hiv_incidence_timeline[i].second << "]";
                }
                hiv_incidence_json_ss << "]";

                // Connect to Redis (try docker hostname first, then localhost)
                std::unique_ptr<Redis> redis_ptr;
                auto try_connect = [&](const std::string &uri) -> bool {
                    try {
                        redis_ptr = std::make_unique<Redis>(uri);
                        // test a command
                        redis_ptr->ping();
                        return true;
                    } catch(const std::exception &e) {
                        std::cerr << "[WARN] Redis connect failed for URI " << uri << ": " << e.what() << std::endl;
                        return false;
                    }
                };

                // Allow override from env
                const char* env_uri = std::getenv("REDIS_URI");
                bool connected = false;
                if (env_uri && std::strlen(env_uri) > 0) {
                    connected = try_connect(env_uri);
                }
                if (!connected) connected = try_connect("tcp://redis:6379");
                if (!connected) connected = try_connect("tcp://127.0.0.1:6379");

                if (connected && redis_ptr) {
                    // Key naming: population:timeline:<epoch>:seed:<seed>
                    std::time_t now = std::time(nullptr);
                    redis_key_used = "population:timeline:" + std::to_string(now);
                    hiv_redis_key_used = "hiv:infections:timeline:" + std::to_string(now);
                    hiv_prevalence_redis_key_used = "hiv:prevalence:timeline:" + std::to_string(now);
                    hiv_incidence_redis_key_used = "hiv:incidence:timeline:" + std::to_string(now);
                    if (seed != -1) {
                        redis_key_used += ":seed:" + std::to_string(seed);
                        hiv_redis_key_used += ":seed:" + std::to_string(seed);
                        hiv_prevalence_redis_key_used += ":seed:" + std::to_string(seed);
                        hiv_incidence_redis_key_used += ":seed:" + std::to_string(seed);
                    }

                    // Store JSON strings
                    try {
                        redis_ptr->set(redis_key_used, json_ss.str());
                        redis_ptr->set(hiv_redis_key_used, hiv_json_ss.str());
                        redis_ptr->set(hiv_prevalence_redis_key_used, hiv_prevalence_json_ss.str());
                        redis_ptr->set(hiv_incidence_redis_key_used, hiv_incidence_json_ss.str());
                        // Also point latest keys to these keys
                        redis_ptr->set("population:timeline:latest", redis_key_used);
                        redis_ptr->set("hiv:infections:timeline:latest", hiv_redis_key_used);
                        redis_ptr->set("hiv:prevalence:timeline:latest", hiv_prevalence_redis_key_used);
                        redis_ptr->set("hiv:incidence:timeline:latest", hiv_incidence_redis_key_used);
                    } catch(const std::exception &e) {
                        std::cerr << "[WARN] Redis set failed: " << e.what() << std::endl;
                    }
                } else {
                    std::cerr << "[WARN] Could not connect to Redis; skipping timeline persistence." << std::endl;
                }
            } else {
                std::cerr << "[INFO] dev_eventlog.csv not found or start_population unknown; skipping timeline persistence." << std::endl;
            }
        } catch(const std::exception &e) {
            std::cerr << "[WARN] Exception during timeline computation/persistence: " << e.what() << std::endl;
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
        if (!redis_key_used.empty()) {
            result["population_timeline_key"] = redis_key_used;
        }
        if (!hiv_redis_key_used.empty()) {
            result["hiv_infections_timeline_key"] = hiv_redis_key_used;
        }
        if (!hiv_prevalence_redis_key_used.empty()) {
            result["hiv_prevalence_timeline_key"] = hiv_prevalence_redis_key_used;
        }
        if (!hiv_incidence_redis_key_used.empty()) {
            result["hiv_incidence_timeline_key"] = hiv_incidence_redis_key_used;
        }

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

    // New endpoint: population_timeline/latest - fetch latest timeline JSON from Redis
    CROW_ROUTE(app, "/population_timeline/latest").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto latest_key = redis_ptr->get("population:timeline:latest");
            if (!latest_key) return crow::response(404, "No latest timeline key");
            auto val = redis_ptr->get(*latest_key);
            if (!val) return crow::response(404, "Timeline not found for latest key");

            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /population_timeline/latest: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: population_timeline/<key> - fetch specific timeline JSON from Redis
    CROW_ROUTE(app, "/population_timeline/<string>").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req, const std::string& key){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto val = redis_ptr->get(key);
            if (!val) return crow::response(404, "Timeline not found");
            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /population_timeline/<key>: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_infections_timeline/latest - fetch latest HIV infections timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_infections_timeline/latest").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto latest_key = redis_ptr->get("hiv:infections:timeline:latest");
            if (!latest_key) return crow::response(404, "No latest HIV infections timeline key");
            auto val = redis_ptr->get(*latest_key);
            if (!val) return crow::response(404, "HIV infections timeline not found for latest key");

            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_infections_timeline/latest: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_infections_timeline/<key> - fetch specific HIV infections timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_infections_timeline/<string>").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req, const std::string& key){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto val = redis_ptr->get(key);
            if (!val) return crow::response(404, "HIV infections timeline not found");
            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_infections_timeline/<key>: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_prevalence_timeline/latest - fetch latest HIV prevalence timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_prevalence_timeline/latest").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto latest_key = redis_ptr->get("hiv:prevalence:timeline:latest");
            if (!latest_key) return crow::response(404, "No latest HIV prevalence timeline key");
            auto val = redis_ptr->get(*latest_key);
            if (!val) return crow::response(404, "HIV prevalence timeline not found for latest key");

            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_prevalence_timeline/latest: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_prevalence_timeline/<key> - fetch specific HIV prevalence timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_prevalence_timeline/<string>").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req, const std::string& key){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }
        try {
            std::unique_ptr<Redis> redis_ptr;
            auto try_connect = [&](const std::string &uri) -> bool {
                try { redis_ptr = std::make_unique<Redis>(uri); redis_ptr->ping(); return true; }
                catch(const std::exception &e) { std::cerr << "[WARN] Redis connect failed: " << e.what() << std::endl; return false; }
            };
            const char* env_uri = std::getenv("REDIS_URI");
            bool connected = false;
            if (env_uri && std::strlen(env_uri) > 0) connected = try_connect(env_uri);
            if (!connected) connected = try_connect("tcp://redis:6379");
            if (!connected) connected = try_connect("tcp://127.0.0.1:6379");
            if (!connected || !redis_ptr) return crow::response(503, "Redis unavailable");

            auto val = redis_ptr->get(key);
            if (!val) return crow::response(404, "HIV prevalence timeline not found");
            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_prevalence_timeline/<key>: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_incidence_timeline/latest - fetch latest HIV incidence timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_incidence_timeline/latest").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }

        try {
            auto redis_ptr = std::make_unique<Redis>("tcp://redis:6379");
            auto val = redis_ptr->get("hiv:incidence:timeline:latest");
            if (!val) {
                // Fallback to localhost
                redis_ptr = std::make_unique<Redis>("tcp://127.0.0.1:6379");
                val = redis_ptr->get("hiv:incidence:timeline:latest");
            }
            if (!val) return crow::response(404, "No HIV incidence timeline found");

            // val is the Redis key, now fetch the actual data
            auto data = redis_ptr->get(*val);
            if (!data) return crow::response(404, "HIV incidence timeline data not found");

            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *data; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_incidence_timeline/latest: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    // New endpoint: hiv_incidence_timeline/<key> - fetch specific HIV incidence timeline JSON from Redis
    CROW_ROUTE(app, "/hiv_incidence_timeline/<string>").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req, const std::string& key){
        if (req.method == "OPTIONS"_method) {
            crow::response res; res.code = 204; return res;
        }

        try {
            auto redis_ptr = std::make_unique<Redis>("tcp://redis:6379");
            auto val = redis_ptr->get(key);
            if (!val) {
                // Fallback to localhost
                redis_ptr = std::make_unique<Redis>("tcp://127.0.0.1:6379");
                val = redis_ptr->get(key);
            }
            if (!val) return crow::response(404, "HIV incidence timeline not found");
            crow::response res; res.code = 200; res.set_header("Content-Type", "application/json"); res.body = *val; return res;
        } catch(const std::exception &e) {
            std::cerr << "[ERROR] /hiv_incidence_timeline/<key>: " << e.what() << std::endl;
            return crow::response(500, "Internal server error");
        }
    });

    app.port(8000).multithreaded().run();
    std::cout << "[viss-api] Crow REST API server stopped.\n";
    return 0;
}