add_subdirectory(third_party/entt)

add_library(entt_configured INTERFACE)
add_library(entt::configured ALIAS entt_configured)

target_include_directories(entt_configured SYSTEM INTERFACE
    third_party/entt/src
)
