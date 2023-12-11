add_subdirectory(GFX)
add_library(GameEngine STATIC GameEngine.cpp)
target_link_libraries(GameEngine PRIVATE GFX spirv_reflect log)