add_executable(maze
    Enemies.hpp Enemies.cpp
    Explosions.hpp Explosions.cpp
    GameStage.hpp GameStage.cpp
    Level.hpp Level.cpp
    Main.cpp
    MazeAssets.hpp.in
    MazeComponents.hpp
    MazeDelegate.hpp MazeDelegate.cpp
    Pch.hpp
    Shots.hpp Shots.cpp
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
        assets/boost.ogg
        assets/enemy-shoot.ogg
        assets/explode.ogg
        assets/laser-hit-wall.ogg
        assets/liberation-mono.ttf
        assets/shoot.ogg
        assets/textures.png
)

configure_file(MazeAssets.hpp.in MazeAssets.hpp)
