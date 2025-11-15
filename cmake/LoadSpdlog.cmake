set(SPDLOG_BUILD_PIC ON)

add_subdirectory(third_party/spdlog)

target_include_directories(spdlog SYSTEM BEFORE INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include>"
)

