set(SOURCES_TEST
	simpactpopulation.cpp
	person.cpp
	eventmortality.cpp
	eventglobal.cpp
	main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_simpact_executable(testglobal ${SOURCES_TEST})


