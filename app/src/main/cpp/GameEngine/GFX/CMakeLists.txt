
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)
add_library(GFX STATIC GfxDevice.cpp
                        TextureSampler.cpp
                        GfxTexture.cpp
        )
target_include_directories(GFX PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(GFX PRIVATE EntityComponent vulkan log)