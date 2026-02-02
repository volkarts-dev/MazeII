include(CheckIPOSupported)

check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_CHECK_ERROR)

macro(enable_ipo)
    if(NOT IPO_SUPPORTED)
        message(STATUS "IPO / LTO not supported: ${IPO_CHECK_ERROR}")
    else()
        message(STATUS "IPO / LTO enabled for all builds")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endmacro()

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
endif()
