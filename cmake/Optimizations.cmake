include(CheckIPOSupported)

check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_CHECK_ERROR)

function(enable_ipo)
    if(NOT IPO_SUPPORTED)
        message(STATUS "IPO / LTO not supported: ${IPO_CHECK_ERROR}")
    else()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endfunction()
