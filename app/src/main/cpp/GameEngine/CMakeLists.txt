add_subdirectory(GFX)
add_subdirectory(EntityComponent)
add_subdirectory(Utils)
add_subdirectory(Renderer)
add_subdirectory(Platform)
add_subdirectory(thirdparty/spirv_reflect)



add_library(GameEngine STATIC GameEngine.cpp EventHandler.cpp)
target_link_libraries(GameEngine PRIVATE Renderer EntityComponent spirv_reflect Util)

if( ${CMAKE_SYSTEM_NAME}  STREQUAL  "Android" )
    find_package(game-activity REQUIRED CONFIG)
    target_link_libraries(GameEngine PRIVATE game-activity::game-activity)
endif()