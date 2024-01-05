
add_library(Renderer STATIC Renderer.cpp
        )
target_include_directories(Renderer PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(Renderer PRIVATE GFX Util)