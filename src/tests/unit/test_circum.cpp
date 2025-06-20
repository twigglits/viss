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

TEST(Circumcision, TestCircumcision) {
    // 1. Set environment variable as in the script
    setenv("MNRM_DEBUG_SEED", "7", 1);
    // 2. Set static expected values file for comparison.
    std::string stat_out = "stat_out_circum_500.json";
    // 3. Prepare a config file with circum.enabled = true
    std::string orig_config = "test_config1.txt";
    std::string temp_config = "test_config1_circum_enabled.txt";
    std::ifstream cfg_in(orig_config);
    ASSERT_TRUE(cfg_in.is_open()) << "Could not open config file: " << orig_config;
    std::ofstream cfg_out(temp_config);
    ASSERT_TRUE(cfg_out.is_open()) << "Could not write temp config file: " << temp_config;
    std::string line;
    while (std::getline(cfg_in, line)) {
        if (line.find("circum.enabled = false") != std::string::npos) {
            cfg_out << "circum.enabled = true\n";
        } else {
            cfg_out << line << "\n";
        }
    }
    cfg_in.close();
    cfg_out.close();

    std::string exe = "./viss-release";
    std::string options = "0 opt -o";
    std::string command = exe + " " + temp_config + " " + options;
    int ret = std::system(command.c_str());
    ASSERT_EQ(ret, 0) << "Release binary failed to run";

    // Clean up temp config file
    if (fs::exists(temp_config)) {
        fs::remove(temp_config);
    }

    // 3. Load reference JSON using Boost.PropertyTree
    boost::property_tree::ptree ref_json;
    boost::property_tree::read_json(stat_out, ref_json);

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
