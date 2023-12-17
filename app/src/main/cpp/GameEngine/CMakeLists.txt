add_subdirectory(GFX)
add_subdirectory(EntityComponent)

add_library(GameEngine STATIC GameEngine.cpp)
target_link_libraries(GameEngine PRIVATE GFX EntityComponent spirv_reflect log)