add_library(ngn STATIC
    gfx/Buffer.hpp gfx/Buffer.cpp
    gfx/CommandBuffer.hpp gfx/CommandBuffer.cpp
    gfx/Colors.hpp gfx/Colors.cpp
    gfx/FontCollection.hpp gfx/FontCollection.cpp
    gfx/FontMaker.hpp gfx/FontMaker.cpp
    gfx/FontRenderer.hpp gfx/FontRenderer.cpp
    gfx/GFXComponents.hpp
    gfx/Image.hpp gfx/Image.cpp
    gfx/Pipeline.hpp gfx/Pipeline.cpp
    gfx/Renderer.hpp gfx/Renderer.cpp
    gfx/SpritePipeline.cpp gfx/SpritePipeline.hpp
    gfx/SpriteRenderer.hpp gfx/SpriteRenderer.cpp
    gfx/Uniforms.hpp

    phys/Collision.hpp
    phys/DynamicTree.hpp phys/DynamicTree.cpp
    phys/Functions.hpp phys/Functions.cpp
    phys/PhysComponents.hpp
    phys/SATDetector.hpp phys/SATDetector.cpp
    phys/Shapes.hpp phys/Shapes.cpp
    phys/Solver.hpp phys/Solver.cpp
    phys/World.hpp phys/World.cpp

    CommonComponents.hpp
    Allocators.hpp Allocators.cpp
    Application.cpp Application.hpp
    Assets.hpp.in
    Input.hpp
    Logging.cpp Logging.hpp
    Macros.hpp
    Math.hpp Math.cpp
    Pch.hpp
    Timer.hpp Timer.cpp
    Types.hpp
)

set(VULKAN_SHADER_SOURCES
    assets/shader/Sprite.frag
    assets/shader/Sprite.geom
    assets/shader/Sprite.vert
)

if(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    target_compile_definitions(ngn PUBLIC NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
endif()

if(NGN_ENABLE_VISUAL_DEBUGGING)
    target_compile_definitions(ngn PUBLIC NGN_ENABLE_VISUAL_DEBUGGING)

    target_sources(ngn PRIVATE
        gfx/DebugPipeline.hpp gfx/DebugPipeline.cpp
        gfx/DebugRenderer.hpp gfx/DebugRenderer.cpp
    )

    set(VULKAN_SHADER_SOURCES
        ${VULKAN_SHADER_SOURCES}
        assets/shader/Debug.frag
        assets/shader/Debug.vert
    )
endif()

target_precompile_headers(ngn PRIVATE Pch.hpp)

target_include_directories(ngn PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(ngn PUBLIC .)

target_link_libraries(ngn PRIVATE compile_options)

target_link_libraries(ngn PUBLIC
    ext
    entt::configured
    Freetype::Freetype
    glfw::glfw
    glm::configured
    spdlog::spdlog
    vulkan::vulkan
)

target_sources(ngn PRIVATE
    ${VULKAN_SHADER_SOURCES}
)

compile_shader_files(
    VAR COMPILED_SHADER_FILES
    SHADER_FILES ${VULKAN_SHADER_SOURCES}
)

target_assets(ngn
    NAMESPACE ngn::assets
    DECLARATIONS_VAR NGN_ASSET_BUNDLE_DECLARATIONS
    STRIP
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    ASSET_FILES
        ${COMPILED_SHADER_FILES}
)

configure_file(Assets.hpp.in Assets.hpp)
