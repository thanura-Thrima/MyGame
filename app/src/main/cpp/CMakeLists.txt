# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("mygame")
#find_package(Vulkan)
add_subdirectory(GameEngine)

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(mygame SHARED
        main.cpp
        AndroidOut.cpp
        #VulkanRenderer.cpp
        Shader.cpp
        TextureAsset.cpp
        Utility.cpp)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

target_include_directories(mygame PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/)
# Configure libraries CMake uses to link your target library.
target_link_libraries(mygame
        # The game activity
        game-activity::game-activity
        GameEngine
        EGL
        GLESv3
        jnigraphics
        android
        log)