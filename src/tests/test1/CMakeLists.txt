set(SOURCES_TEST
	simpactpopulation.cpp
	person.cpp
	eventmortality.cpp
	eventformation.cpp
	eventdissolution.cpp
	eventdebut.cpp
	hazardutility.cpp
	main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_simpact_executable(test1 ${SOURCES_TEST})


