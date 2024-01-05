add_subdirectory(vulkan)

add_library(GFX STATIC IGfxDevice.cpp
        )
target_link_libraries(GFX PRIVATE GfxVulkan log)