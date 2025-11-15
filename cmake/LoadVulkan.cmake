add_library(system_lib_vulkan INTERFACE "")
add_library(vulkan::vulkan ALIAS system_lib_vulkan)

target_link_libraries(system_lib_vulkan INTERFACE
    vulkan
)

target_compile_definitions(system_lib_vulkan INTERFACE
    VULKAN_HPP_NO_CONSTRUCTORS
)
