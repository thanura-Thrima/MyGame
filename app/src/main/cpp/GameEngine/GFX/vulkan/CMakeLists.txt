
find_package(vulkan)

if( ${CMAKE_SYSTEM_NAME}  STREQUAL  "Android" )
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)
endif()

add_library(GfxVulkan STATIC GfxDevice.cpp
        TextureSampler.cpp
        GfxTexture.cpp
        )
target_include_directories(GfxVulkan PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(GfxVulkan PRIVATE EntityComponent vulkan Util)