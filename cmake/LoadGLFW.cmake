set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(third_party/glfw)

target_include_directories(glfw SYSTEM BEFORE INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include>"
)

add_library(glfw::glfw ALIAS glfw)
