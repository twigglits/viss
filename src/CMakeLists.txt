set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The 'program' subdirectory contains the main simpact program
add_subdirectory(program)
add_subdirectory(program-maxart)

# Some tests
add_subdirectory(tests/test1)
add_subdirectory(tests/test_mort)
add_subdirectory(tests/global)
add_subdirectory(tests/config)
add_subdirectory(tests/varia)
add_subdirectory(tests/unit)

# Build the Crow REST API server
add_executable(viss-api http_server.cpp)
target_include_directories(viss-api PRIVATE ${CMAKE_SOURCE_DIR}/external)
# Add redis-plus-plus as a subdirectory so its CMake logic is executed
add_subdirectory(../redis-plus-plus ${CMAKE_BINARY_DIR}/redis-plus-plus)

# Redis++ and hiredis include directories (default /usr/local/include for local install)
target_include_directories(viss-api PRIVATE /usr/local/include)
target_include_directories(viss-api PRIVATE /usr/include)
# Link hiredis and redis++
target_link_libraries(viss-api pthread hiredis redis++)
target_compile_definitions(viss-api PRIVATE CROW_USE_BOOST)
