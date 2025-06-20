# Top-level CMakeLists.txt for tests
cmake_minimum_required(VERSION 3.10)

# Force C++17 for all targets in this subdirectory
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(SimpactTests)

# Enable testing
enable_testing()

# Download and unpack googletest at configure time if not present
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Boost REQUIRED COMPONENTS system filesystem)

# Add test source
add_executable(test_base test_base.cpp)
add_executable(test_circum test_circum.cpp)

# Set C++17 for this test target only
set_target_properties(test_base PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
set_target_properties(test_circum PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)

# Link GoogleTest and Boost
# gtest_main provides main(); Boost is needed for property_tree
# Use PRIVATE to avoid leaking Boost to dependents

target_link_libraries(test_base PRIVATE gtest_main Boost::system Boost::filesystem)
target_link_libraries(test_circum PRIVATE gtest_main Boost::system Boost::filesystem)

# Add test
# Copy required files to build dir so test can find them
configure_file(${CMAKE_SOURCE_DIR}/test_config1.txt ${CMAKE_BINARY_DIR}/test_config1.txt COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/stat_out_base_500.json ${CMAKE_BINARY_DIR}/stat_out_base_500.json COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/stat_out_circum_500.json ${CMAKE_BINARY_DIR}/stat_out_circum_500.json COPYONLY)
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/intervention DESTINATION ${CMAKE_BINARY_DIR})

add_test(NAME test_base COMMAND ${CMAKE_BINARY_DIR}/test_base)
add_test(NAME test_circum COMMAND ${CMAKE_BINARY_DIR}/test_circum)
set_tests_properties(test_base PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(test_circum PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

