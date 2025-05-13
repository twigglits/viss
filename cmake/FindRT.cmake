set(RT_INCLUDE_DIRS "")

find_library(RT_LIBRARY rt)
set(RT_LIBRARIES ${RT_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(RT DEFAULT_MSG RT_LIBRARIES)

