add_executable(testbed
    Main.cpp
    TestBedAssets.hpp.in
    TestBedDelegate.hpp TestBedDelegate.cpp
)

target_include_directories(testbed PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(testbed PRIVATE compile_options)

target_link_libraries(testbed PRIVATE ngn)

target_assets(testbed
    NAMESPACE testbed::assets
    DECLARATIONS_VAR TESTBED_ASSET_BUNDLE_DECLARATIONS
    STRIP
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    ASSET_FILES
        assets/player.png
)

configure_file(TestBedAssets.hpp.in TestBedAssets.hpp)
