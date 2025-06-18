// Google Test-based test for output comparison
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace fs = std::filesystem;

// Helper: Copy and edit config
void create_test_config(const std::string& orig, const std::string& test) {
    // Not used in this simple test
}

TEST(CompareOutputs, ReleaseMatchesReference) {
    // 1. Set environment variable as in the script
    setenv("MNRM_DEBUG_SEED", "7", 1);

    // 2. Check for all files referenced in test_config1.txt
    std::string config = "test_config1.txt";
    std::ifstream cfg(config);
    ASSERT_TRUE(cfg.is_open()) << "Could not open config file: " << config;
    std::vector<std::string> missing_files;
    std::string line;
    while (std::getline(cfg, line)) {
        // Remove comments
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos) line = line.substr(0, hash_pos);
        // Find value (after =)
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        std::string value = line.substr(eq_pos + 1);
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        // Heuristics: check if value looks like a file path
        if (value.size() > 0 && (value.find(".csv") != std::string::npos || value.find("./data/") == 0 || value.find("./intervention/") == 0)) {
            // Ignore output/log file patterns
            if (value.find("${SIMPACT_OUTPUT_PREFIX}") != std::string::npos ||
                (!value.empty() && value.back() == '_') ||
                value.find('%') != std::string::npos) {
                continue;
            }
            std::string path = value;
            // Remove quotes if present
            if (!path.empty() && path.front() == '"') path = path.substr(1);
            if (!path.empty() && path.back() == '"') path.pop_back();
            // Check existence
            if (!fs::exists(path)) {
                missing_files.push_back(path);
            }
        }
    }
    cfg.close();
    if (!missing_files.empty()) {
        std::cerr << "[ERROR] Missing files referenced in config:" << std::endl;
        for (const auto& f : missing_files) std::cerr << "  " << f << std::endl;
        FAIL() << "Test aborted due to missing required files.";
    }
    // Print working directory and command for debug
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "[DEBUG] CWD: " << cwd << std::endl;
        std::cout << "[DEBUG] Files in ./data:" << std::endl;
        for (const auto& entry : fs::directory_iterator("./data")) {
            std::cout << "  " << entry.path() << std::endl;
        }
        std::cout << "[DEBUG] Files in ./intervention:" << std::endl;
        for (const auto& entry : fs::directory_iterator("./intervention")) {
            std::cout << "  " << entry.path() << std::endl;
        }
    } else {
        perror("getcwd() error");
    }
    std::string exe = "./viss-release";
    std::string options = "0 opt -o";
    std::string command = exe + " " + config + " " + options;
    std::cout << "[DEBUG] Running command: " << command << std::endl;
    int ret = std::system(command.c_str());
    ASSERT_EQ(ret, 0) << "Release binary failed to run";

    // 3. Load reference JSON using Boost.PropertyTree
    boost::property_tree::ptree ref_json;
    boost::property_tree::read_json("stat_out_500.json", ref_json);

    // 4. Parse dev_eventlog.csv and count events in column 2
    std::ifstream csv("dev_eventlog.csv");
    ASSERT_TRUE(csv.is_open()) << "Could not open dev_eventlog.csv";
    std::unordered_map<std::string, int> event_counts;
    std::string csv_line;
    while (std::getline(csv, csv_line)) {
        if (csv_line.empty()) continue;
        std::istringstream ss(csv_line);
        std::string field;
        int col = 0;
        std::string event;
        while (std::getline(ss, field, ',')) {
            if (col == 1) { // 2nd column
                event = field;
                break;
            }
            ++col;
        }
        if (!event.empty()) {
            ++event_counts[event];
        }
    }
    csv.close();

    // 5. For each event in the JSON, compare expected to actual count
    for (const auto& ref_pair : ref_json) {
        const std::string& event = ref_pair.first;
        int expected = ref_pair.second.get<int>("expected");
        int actual = event_counts[event];
        EXPECT_EQ(actual, expected) << "Mismatch for event '" << event << "': expected " << expected << ", got " << actual;
    }

}
