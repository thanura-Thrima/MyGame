
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

add_library(GFX STATIC GfxDevice.cpp
                        TextureSampler.cpp
                        GfxTexture.cpp
        )

target_link_libraries(GFX PRIVATE vulkan log)