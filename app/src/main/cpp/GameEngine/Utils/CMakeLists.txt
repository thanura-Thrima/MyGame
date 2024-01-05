
add_library(Util STATIC IFileManager.cpp
                        MemoryManager.cpp
                        Definitions.h
        )
target_include_directories(Util PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(Util PRIVATE log)

if( ${CMAKE_SYSTEM_NAME}  STREQUAL  "Android" )
    message("linking Android platform specifcs")
    target_link_libraries(Util PRIVATE PlatformAndroid)
endif()

