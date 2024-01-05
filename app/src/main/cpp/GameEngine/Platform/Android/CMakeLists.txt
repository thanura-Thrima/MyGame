
add_library(PlatformAndroid STATIC PlatformAndroid.cpp
        )
#(Platform PUBLIC ${CMAKE_SOURCE_DIR}/GameEngine/thirdparty/)
target_link_libraries(PlatformAndroid android log)