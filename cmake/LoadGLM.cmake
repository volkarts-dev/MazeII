add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)

target_include_directories(glm SYSTEM BEFORE INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/glm/..>
)

add_library(glm_configured INTERFACE)
add_library(glm::configured ALIAS glm_configured)

target_compile_definitions(glm_configured INTERFACE
    GLM_FORCE_INLINE
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    GLM_FORCE_INTRINSICS
)

target_link_libraries(glm_configured INTERFACE
    glm::glm
)
