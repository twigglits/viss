macro(viss_setup)
	include(${PROJECT_SOURCE_DIR}/cmake/Macros.cmake)

	if (NOT UNIX OR CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "" FORCE)
	endif()
	if ("${CMAKE_CXX_FLAGS_DEBUG}" STREQUAL "")
		message("Debug flags not set, setting default")
		set(CMAKE_CXX_FLAGS_DEBUG "-g")
	endif()
	if ("${CMAKE_CXX_FLAGS_RELEASE}" STREQUAL "")
		message("Release flags not set, setting default")
		set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
	endif()

	get_install_directory(LIBRARY_INSTALL_DIR)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

	include(CheckCXXCompilerFlag)
	include(CheckCXXSourceCompiles)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if(COMPILER_SUPPORTS_CXX0X)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
		# Test if the compiler already supports C++0x
		check_cxx_source_compiles("int main(void) \n { \n char *a = R\"TST(X)TST\"; \n return 0; \n }" C0XAVAIL)
		if (NOT C0XAVAIL)
			message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++0x support.")
		endif()
	endif()

	if (WIN32)
		# Make sure Win32 flag is set and exceptions are enabled to avoid warnings 
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32 /EHsc")

		# If we're using our own precompiled gsl and tiff on windows, try to use
		# this (in the docs I've specified that you should set CMAKE_INSTALL_PREFIX
		# accordingly). This is to avoid other libraries getting mixed in there, for
		# example from Anaconda Python.
		option(USE_PRECOMPILED_GSL_TIFF "Use the package with precompiled GSL and TIFF libraries" ON)
		if (USE_PRECOMPILED_GSL_TIFF)
			set(ZLIB_FOUND 0)
			set(ZLIB_INCLUDE_DIRS "")
			set(ZLIB_LIBRARIES "")
			if (EXISTS "${CMAKE_INSTALL_PREFIX}/lib/gsl.lib")
				set(GSL_LIBRARIES "${CMAKE_INSTALL_PREFIX}/lib/gsl.lib"
					          "${CMAKE_INSTALL_PREFIX}/lib/cblas.lib")
				set(GSL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
				set(GSL_FOUND 1)
			else()
				message(FATAL_ERROR "You specified that the precompiled GSL and TIFF libraries should be used, but gsl.lib could not be found in CMAKE_INSTALL_PREFIX/lib/")
			endif()
			if (EXISTS "${CMAKE_INSTALL_PREFIX}/lib/tiff.lib")
				set(TIFF_LIBRARIES "${CMAKE_INSTALL_PREFIX}/lib/tiff.lib")
				set(TIFF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
				set(TIFF_VERSION_MAJOR 4)
				set(TIFF_FOUND 1)
			else()
				message(FATAL_ERROR "You specified that the precompiled GSL and TIFF libraries should be used, but tiff.lib could not be found in CMAKE_INSTALL_PREFIX/lib/")
			endif()

			include_directories(${GSL_INCLUDE_DIR})
			include_directories(${TIFF_INCLUDE_DIR})
		endif (USE_PRECOMPILED_GSL_TIFF)
	endif ()

	if (NOT USE_PRECOMPILED_GSL_TIFF)
		find_package(ZLIB)
		if (NOT ZLIB_FOUND)
			set(ZLIB_INCLUDE_DIRS "")
			set(ZLIB_LIBRARIES "")
		else()
			include_directories(${ZLIB_INCLUDE_DIRS})
		endif()

		set(GSL_MANUAL_SETTINGS FALSE CACHE BOOL "Set to TRUE if you want to specify the GSL libraries and include paths in the ADDITIONAL_XXX settings")
		if (NOT GSL_MANUAL_SETTINGS)
			find_package(GSL REQUIRED)
			include_directories(${GSL_INCLUDE_DIR})
		endif()

		find_package(TIFF REQUIRED)
		include_directories(${TIFF_INCLUDE_DIR})
		string(REPLACE "." ";" TIFF_VERSION_LIST "${TIFF_VERSION_STRING}")
		list(GET TIFF_VERSION_LIST 0 TIFF_VERSION_MAJOR)
	endif (NOT USE_PRECOMPILED_GSL_TIFF)

	find_package(OpenMP)
	find_package(RT)

	if (NOT RT_LIBRARIES)
		set(RT_LIBRARIES "")
	endif()

	add_additional_stuff(EXTRA_INCLUDES EXTRA_LIBS)

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

	include_directories(${PROJECT_SOURCE_DIR}/src/)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib/simpact)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib/mnrm)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib/util)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib/util/experimental)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib/core)
	include_directories(${PROJECT_SOURCE_DIR}/src/lib)
	include_directories(${PROJECT_BINARY_DIR}/)
	include_directories(${PROJECT_BINARY_DIR}/src)
	include_directories(${EXTRA_INCLUDES})

	configure_file(${PROJECT_SOURCE_DIR}/src/version.h.in ${PROJECT_BINARY_DIR}/src/version.h)

	set(SOURCES_UTIL
		${PROJECT_SOURCE_DIR}/src/lib/util/util.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/csvfile.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/mutex.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/discretedistribution.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/discretedistributionfast.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/discretedistribution2d.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/populationdistributioncsv.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/populationdistribution.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/configreader.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/configsettings.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/configdistributionhelper.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/experimental/discretefunction.cpp
		#${PROJECT_SOURCE_DIR}/src/lib/util/experimental/Faddeeva.cc
		#${PROJECT_SOURCE_DIR}/src/lib/util/experimental/inverseerfi.cpp
		#${PROJECT_SOURCE_DIR}/src/lib/util/experimental/exponentialfunction.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/hazardfunction.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/logfile.cpp 
		${PROJECT_SOURCE_DIR}/src/lib/util/tiffdensityfile.cpp 
		${PROJECT_SOURCE_DIR}/src/lib/util/configwriter.cpp 
		${PROJECT_SOURCE_DIR}/src/lib/util/piecewiselinearfunction.cpp 
		${PROJECT_SOURCE_DIR}/src/lib/util/normaldistribution.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/jsonconfig.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/binormaldistribution.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/configfunctions.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/discretedistributionwrapper.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/gridvaluescsv.cpp
		${PROJECT_SOURCE_DIR}/src/lib/util/discretedistributionwrapper2d.cpp
		)
	set(SOURCES_MRNM
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/gslrandomnumbergenerator.cpp
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/algorithm.cpp
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/simplealgorithm.cpp
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/booltype.cpp
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/eventbase.cpp
		${PROJECT_SOURCE_DIR}/src/lib/mnrm/debugtimer.cpp
		)
	set(SOURCES_CORE
		${PROJECT_SOURCE_DIR}/src/lib/core/personbase.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationalgorithmsimple.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationalgorithmadvanced.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationalgorithmtesting.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationstatesimple.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationstateadvanced.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationstatetesting.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationstatesimpleadvancedcommon.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationevent.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/personaleventlist.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/personaleventlisttesting.cpp
		${PROJECT_SOURCE_DIR}/src/lib/core/populationutil.cpp
		)

	source_group(util FILES ${SOURCES_UTIL})
	source_group(mnrm FILES ${SOURCES_MRNM})
	source_group(core FILES ${SOURCES_CORE})

	if (UNIX AND NOT CMAKE_GENERATOR STREQUAL Xcode)
		if (NOT CMAKE_BUILD_TYPE STREQUAL "")
			message(FATAL_ERROR "\n\nThe CMake variable CMAKE_BUILD_TYPE should be empty, versions with different settings will be created automatically!\n\n")
		endif()
	endif()

	set(SOURCES ${SOURCES_UTIL} ${SOURCES_CORE} ${SOURCES_MRNM})
	add_simpact_library("simpact-lib-static" ${SOURCES})

	# From: http://stackoverflow.com/questions/11813271/embed-resources-eg-shader-code-images-into-executable-library-with-cmake
	#add_executable(embedfile "${PROJECT_SOURCE_DIR}/cmake/embedfile.c")
endmacro()

macro(add_simpact_library LIBPREFIX MAINSOURCES)
	set(MAINSOURCES0 "${ARGV}")
	list(REMOVE_AT MAINSOURCES0 0)
	set(SOURCES ${MAINSOURCES0})
	add_simpact_executable_or_library("LIB" ${LIBPREFIX} ${SOURCES})
endmacro()

macro(add_simpact_executable EXEPREFIX MAINSOURCES)
	set(MAINSOURCES0 "${ARGV}")
	list(REMOVE_AT MAINSOURCES0 0)
	set(SOURCES ${MAINSOURCES0})
	add_simpact_executable_or_library("EXE" ${EXEPREFIX} ${SOURCES})
endmacro()

macro(install_simpact_executable EXEPREFIX)
	if (UNIX AND NOT CMAKE_GENERATOR STREQUAL Xcode)
		install(TARGETS ${EXEPREFIX}-release ${EXEPREFIX}-debug DESTINATION bin)
	endif()
endmacro()

macro(add_library_or_executable LIBOREXEFLAG LIBOREXENAME MAINSOURCES)
	#message(${LIBOREXEFLAG})
	#message(${LIBOREXENAME})
	#message(${SOURCES})
	set(MAINSOURCES0 "${ARGV}")
	list(REMOVE_AT MAINSOURCES0 0)
	list(REMOVE_AT MAINSOURCES0 0)
	set(SOURCES ${MAINSOURCES0})

	if (${LIBOREXEFLAG})
		#message("Adding library")
		add_library(${LIBOREXENAME} STATIC ${SOURCES})
	else()
		#message("Adding executable")
		add_executable(${LIBOREXENAME} ${SOURCES})
	endif()
endmacro()

macro(add_simpact_executable_or_library LIBOREXE EXEPREFIX MAINSOURCES)
	#message(${LIBOREXE})
	set(MAINSOURCES0 "${ARGV}")
	list(REMOVE_AT MAINSOURCES0 0)
	list(REMOVE_AT MAINSOURCES0 0)
	set(SOURCES ${MAINSOURCES0})

	string(COMPARE EQUAL ${LIBOREXE} "LIB" USELIBSETTINGS)
	#message(${USELIBSETTINGS})

	if (OPENMP_FOUND)
		set(OPENMPDEFINE "")
	else()
		set(OPENMPDEFINE "DISABLEOPENMP")
	endif()

	set(ALLLIBS ${EXTRA_LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES} ${ZLIB_LIBRARIES} ${RT_LIBRARIES} ${JTHREAD_LIBRARIES} ${MEANWALKER_LIBRARIES} ${TIFF_LIBRARIES})

	if (UNIX AND NOT CMAKE_GENERATOR STREQUAL Xcode)
		if (USELIBSETTINGS)
			#message("Using library settings")
			set(ALLLIBSRELEASE ${ALLLIBS})
			set(ALLLIBSDEBUG ${ALLLIBS})
		else()
			#message("Using exe settings")
			set(ALLLIBSRELEASE simpact-lib-static-release)
			set(ALLLIBSDEBUG simpact-lib-static-debug)
		endif()

		add_library_or_executable(${USELIBSETTINGS} ${EXEPREFIX}-release ${SOURCES})
		target_link_libraries(${EXEPREFIX}-release ${ALLLIBSRELEASE})
		set_target_properties(${EXEPREFIX}-release PROPERTIES COMPILE_DEFINITIONS "TIFFVERSION=${TIFF_VERSION_MAJOR};${OPENMPDEFINE};EVENTBASE_ALWAYS_CHECK_NANTIME")
		set_target_properties(${EXEPREFIX}-release PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
		set_target_properties(${EXEPREFIX}-release PROPERTIES LINK_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
		add_openmp_flags(${EXEPREFIX}-release) # Must be last (set_target_properties changes it again otherwise)

		add_library_or_executable(${USELIBSETTINGS} ${EXEPREFIX}-debug ${SOURCES})
		target_link_libraries(${EXEPREFIX}-debug ${ALLLIBSDEBUG})
		set_target_properties(${EXEPREFIX}-debug PROPERTIES COMPILE_DEFINITIONS "TIFFVERSION=${TIFF_VERSION_MAJOR};${OPENMPDEFINE};EVENTBASE_ALWAYS_CHECK_NANTIME")
		set_target_properties(${EXEPREFIX}-debug PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
		set_target_properties(${EXEPREFIX}-debug PROPERTIES LINK_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
		add_openmp_flags(${EXEPREFIX}-debug)
	else()

		if (NOT USELIBSETTINGS)
			set(ALLLIBS simpact-lib-static)
		endif()
		add_library_or_executable(${USELIBSETTINGS} ${EXEPREFIX} ${SOURCES})
		target_link_libraries(${EXEPREFIX} ${ALLLIBS})
		set_target_properties(${EXEPREFIX} PROPERTIES COMPILE_DEFINITIONS "TIFFVERSION=${TIFF_VERSION_MAJOR};${OPENMPDEFINE};EVENTBASE_ALWAYS_CHECK_NANTIME")
		add_openmp_flags(${EXEPREFIX}) # Must be last (set_target_properties changes it again otherwise)

		if (USELIBSETTINGS)
			set(CMAKE_RELEASE_POSTFIX "-release")
			set(CMAKE_DEBUG_POSTFIX "-debug") # Is only used for libraries
		else()
			set_property(TARGET ${EXEPREFIX} PROPERTY RELEASE_OUTPUT_NAME "${EXEPREFIX}-release")
			set_property(TARGET ${EXEPREFIX} PROPERTY DEBUG_OUTPUT_NAME "${EXEPREFIX}-debug")
		endif()
	endif()
endmacro(add_simpact_executable_or_library)
