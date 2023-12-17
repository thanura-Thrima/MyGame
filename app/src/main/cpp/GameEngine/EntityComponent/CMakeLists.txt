
add_library(EntityComponent STATIC Mesh.cpp
        )
target_include_directories(EntityComponent PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(EntityComponent PRIVATE vulkan log)