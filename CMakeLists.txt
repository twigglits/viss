cmake_minimum_required(VERSION 3.10)
# The NEW behavior for CMP0022 is to allow targets to work properly with INTERFACE_LINK_LIBRARIES
cmake_policy(SET CMP0022 NEW)

project(viss)
set(VERSION 1.0.0)

# Enable CTest at the top-level so all subdirectory tests are registered
enable_testing()

include(${PROJECT_SOURCE_DIR}/cmake/VissMacros.cmake)

viss_setup()

# This contains the main simpact program
add_subdirectory(src)

# Install everything from the data dir in e.g. /usr/local/share/viss/ @JEAN TODO: change this to 'viss' later.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/viss/)
install(FILES ${PROJECT_SOURCE_DIR}/python/pysimpactcyan.py DESTINATION ${CMAKE_INSTALL_PREFIX}/share/viss/python/)
