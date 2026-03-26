add_executable(maze
    gfx/Dialog.hpp gfx/Dialog.cpp
    gfx/OverviewMap.hpp gfx/OverviewMap.cpp

    Board.hpp Board.cpp
    Enemies.hpp Enemies.cpp
    Explosions.hpp Explosions.cpp
    GameStage.hpp GameStage.cpp
    Layers.hpp
    Main.cpp
    MazeAssets.hpp.in
    MazeComponents.hpp
    MazeDelegate.hpp MazeDelegate.cpp
    Pch.hpp
    Player.hpp Player.cpp
    Shots.hpp Shots.cpp
)

set(VULKAN_SHADER_SOURCES
    assets/shader/OverviewMap.frag
    assets/shader/OverviewMap.vert
)

target_sources(maze PRIVATE
    ${VULKAN_SHADER_SOURCES}
)

compile_shader_files(
    VAR COMPILED_SHADER_FILES
    SHADER_FILES ${VULKAN_SHADER_SOURCES}
)

target_include_directories(maze PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(maze PRIVATE compile_options)

target_precompile_headers(maze PRIVATE Pch.hpp)

target_link_libraries(maze PRIVATE ngn)

target_assets(maze
    NAMESPACE maze::assets
    DECLARATIONS_VAR MAZE_ASSETS_BUNDLE_DECLARATIONS
    STRIP
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    ASSET_FILES
        ${COMPILED_SHADER_FILES}
        assets/boost.ogg
        assets/enemy-shoot.ogg
        assets/explosion_one.ogg
        assets/explosion_two.ogg
        assets/laser-hit-wall.ogg
        assets/liberation-mono.ttf
        assets/shoot.ogg
        assets/textures.png
)

configure_file(MazeAssets.hpp.in MazeAssets.hpp)
