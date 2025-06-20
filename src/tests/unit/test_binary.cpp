#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace fs = std::filesystem;

// Helper: Copy and edit config
void create_test_config(const std::string& orig, const std::string& test) {
    // Not used in this simple test
}

TEST(Binary, TestBinary) {
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
    }
    cfg.close();
    // 2. Run release binary
    std::string bin = "./viss-release";
    std::string options = "0 opt -o";
    std::string command = bin + " " + config + " " + options;
    int ret = std::system(command.c_str());
    ASSERT_EQ(ret, 0) << "Release binary failed to run";
    std::ifstream csv("dev_eventlog.csv");
    ASSERT_TRUE(csv.is_open()) << "Could not open dev_eventlog.csv";
    std::unordered_map<std::string, int> release_event_counts;
    std::string csv_line;
    while (std::getline(csv, csv_line)) {
        if (csv_line.empty()) continue;
        std::istringstream ss(csv_line);
        std::string field;
        int col = 0;
        std::string release_event;
        while (std::getline(ss, field, ',')) {
            if (col == 1) { // 2nd column
                release_event = field;
                break;
            }
            ++col;
        }
        if (!release_event.empty()) {
            ++release_event_counts[release_event];
        }
    }
    csv.close();

    // 3. Run debug binary (reuse variables)
    bin = "./viss-debug";
    command = bin + " " + config + " " + options;
    ret = std::system(command.c_str());
    ASSERT_EQ(ret, 0) << "Debug binary failed to run";
    csv.open("dev_eventlog.csv");
    ASSERT_TRUE(csv.is_open()) << "Could not open dev_eventlog.csv";
    std::unordered_map<std::string, int> debug_event_counts;
    csv_line.clear();
    while (std::getline(csv, csv_line)) {
        if (csv_line.empty()) continue;
        std::istringstream ss(csv_line);
        std::string field;
        int col = 0;
        std::string debug_event;
        while (std::getline(ss, field, ',')) {
            if (col == 1) { // 2nd column
                debug_event = field;
                break;
            }
            ++col;
        }
        if (!debug_event.empty()) {
            ++debug_event_counts[debug_event];
        }
    }
    csv.close();

    // 4. Strictly compare event sets and counts
    std::unordered_set<std::string> release_events, debug_events, all_events;
    for (const auto& p : release_event_counts) release_events.insert(p.first);
    for (const auto& p : debug_event_counts) debug_events.insert(p.first);
    for (const auto& p : release_event_counts) all_events.insert(p.first);
    for (const auto& p : debug_event_counts) all_events.insert(p.first);
    EXPECT_EQ(release_events, debug_events) << "Distinct event sets differ between release and debug runs.";

    // 5. Compare event counts for all events
    for (const auto& event : all_events) {
        int expected = release_event_counts[event];
        int actual = debug_event_counts[event];
        EXPECT_EQ(actual, expected) << "Mismatch for event '" << event << "': expected " << expected << ", got " << actual;
    }
}

