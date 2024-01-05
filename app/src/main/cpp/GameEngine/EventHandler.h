#pragma once
#include <string>
#include <game-activity/native_app_glue/android_native_app_glue.h>

class EventHandler {
public:
    enum class Event{
        Close,
        Resize,
        Touch_press,
        Touch_release,
        Key_down,
        Key_up
    };
    void onRecieve(Event e,std::string arg...);
    void handleEvents(android_app* app);
};