#pragma once
#include <android/native_window_jni.h>

class IGfxDevice{
public:
    virtual ~IGfxDevice() = 0;
    virtual bool isInitialized() = 0;
    virtual void createSurface(ANativeWindow* window) = 0;
    virtual void reCreateSwapchain() = 0;

    virtual void init() = 0;
    virtual void deInit() = 0;
};