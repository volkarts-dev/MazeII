add_executable(assetc "")

target_sources(assetc PRIVATE
    AssetCompiler.cpp
    AssetCompiler.h
    Helper.cpp
    Helper.h
    Main.cpp
)

target_link_libraries(assetc PRIVATE
    compile_options
    CLI11::CLI11
    spdlog::spdlog
)
